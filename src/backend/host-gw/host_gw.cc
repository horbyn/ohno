// clang-format off
#include "host_gw.h"
#include "spdlog/fmt/fmt.h"
#include "src/common/assert.h"
#include "src/common/except.h"
#include "src/net/addr.h"
#include "src/net/route.h"
// clang-format on

namespace ohno {
namespace backend {

/**
 * @brief 启动后端线程
 *
 * @param node_name 当前 Kubernetes 节点名称
 */
auto HostGw::start(std::string_view node_name) -> void {
  OHNO_ASSERT(!node_name.empty());
  OHNO_ASSERT(center_);
  OHNO_ASSERT(ipam_);
  OHNO_ASSERT(nic_);

  if (running_) {
    return;
  }
  running_ = true;

  auto interval = interval_ == 0 ? DEFAULT_INTERVAL : interval_;
  monitor_ = std::thread{[&running = running_,
                          callback = std::bind(&HostGw::eventHandler, this, node_name),
                          interv = interval]() {
    try {
      pthread_setname_np(pthread_self(), "host-gw");

      while (running.load()) {
        callback();
        std::this_thread::sleep_for(std::chrono::seconds(interv));
      }
    } catch (const ohno::except::Exception &exc) {
      std::cerr << "[error] Ohnod worker thread terminated!" << exc.getMsg() << "\n";
    } catch (const std::exception &exc) {
      std::cerr << "[error] Ohnod worker thread terminated!" << exc.what() << "\n";
    }
  }};
}

/**
 * @brief 停止后端线程
 *
 */
auto HostGw::stop() -> void {
  running_ = false;
  if (monitor_.joinable()) {
    monitor_.join();
  }
}

/**
 * @brief 设置轮询时间间隔，单位秒
 *
 * @param sec 监听的时间间隔
 */
auto HostGw::setInterval(int sec) -> void { interval_ = sec; }

/**
 * @brief 设置 Center 对象
 *
 * @param center Center 对象
 */
auto HostGw::setCenter(std::unique_ptr<backend::CenterIf> center) -> void {
  center_ = std::move(center);
}

/**
 * @brief 设置持久化对象
 *
 * @param ipam 持久化对象
 */
auto HostGw::setIpam(std::unique_ptr<ipam::IpamIf> ipam) -> void { ipam_ = std::move(ipam); }

/**
 * @brief 设置网卡对象
 *
 * @param nic 网卡对象
 * @param netlink Netlink 对象
 * @return true 成功
 * @return false 失败
 */
auto HostGw::setNic(std::unique_ptr<net::NicIf> nic, std::weak_ptr<net::NetlinkIf> netlink)
    -> bool {
  nic_ = std::move(nic);
  return nic_->setup(std::move(netlink));
}

/**
 * @brief 触发事件
 *
 * @param current_node 当前 Kubernetes 节点名称
 */
auto HostGw::eventHandler(std::string_view current_node) -> void {
  // 集群是直接从 Kubernetes api server 中获取的，包含所有节点
  auto cluster = center_->getKubernetesData();

  // 持久化是从分布式缓存中获取的，仅包含创建了 CNI 插件的节点
  std::string subnet_cur{};
  std::string subnet{};
  ipam_->getSubnet(current_node, subnet_cur);

  if (!subnet_cur.empty()) {
    // 检查节点新增
    for (const auto &[name, info] : cluster) {
      if (current_node == name) {
        continue; // 跳过自己
      }
      ipam_->getSubnet(name, subnet);

      // 出现在持久化中，并且之前没有缓存过
      if (!subnet.empty() && node_cache_.find(name) == node_cache_.end()) {
        if (!nic_->addRoute(
                std::make_unique<net::Route>(info.pod_cidr_, info.internal_ip_, std::string{}))) {
          OHNO_LOG(warn, "Host-gw mode failed to create static route(dest:{}, via:{})",
                   info.pod_cidr_, info.internal_ip_);
        } else {
          node_cache_.emplace(name, info);
          OHNO_LOG(info, "Host-gw static route(dest:{}, via:{}) existed", info.pod_cidr_,
                   info.internal_ip_);
        }
      }
    }
  }

  // 检查节点删除
  for (auto it = node_cache_.begin(); it != node_cache_.end();) {
    const auto &[name, info] = *it;

    // 只有两种情况：
    // 1. 当前节点已被删除，则节点内所有静态路由都要删除
    // 2. 其他节点被删除了，则节点只需要删除对应的静态路由
    ipam_->getSubnet(name, subnet);
    if (subnet_cur.empty() || (subnet.empty() && node_cache_.find(name) != node_cache_.end())) {
      if (!nic_->delRoute(info.pod_cidr_, info.internal_ip_, {})) {
        OHNO_LOG(warn, "Host-gw mode failed to delete static route(dest:{}, via:{})",
                 info.pod_cidr_, info.internal_ip_);
      } else {
        it = node_cache_.erase(it);
        OHNO_LOG(info, "Host-gw static route(dest:{}, via:{}) has been erased", info.pod_cidr_,
                 info.internal_ip_);
        continue; // 下一轮迭代直接使用当前迭代器
      }
    }

    ++it;
  }
}

} // namespace backend
} // namespace ohno
