// clang-format off
#include "backend.h"
#include "src/common/assert.h"
#include "src/common/except.h"
// clang-format on

namespace ohno {
namespace backend {

/**
 * @brief 启动后端线程
 *
 * @param node_name 当前 Kubernetes 节点名称
 */
auto Backend::start(std::string_view node_name) -> void { startImpl(node_name, "backend"); }

/**
 * @brief 停止后端线程
 *
 */
auto Backend::stop() -> void {
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
auto Backend::setInterval(int sec) -> void { interval_ = sec; }

/**
 * @brief 设置 Center 对象
 *
 * @param center Center 对象
 */
auto Backend::setCenter(std::unique_ptr<backend::CenterIf> center) -> void {
  center_ = std::move(center);
}

/**
 * @brief 设置网卡对象
 *
 * @param nic 网卡对象
 */
auto Backend::setNic(std::unique_ptr<net::NicIf> nic) -> void { nic_ = std::move(nic); }

/**
 * @brief 启动后端线程
 *
 * @param node_name 当前 Kubernetes 节点名称
 * @param thread_name 线程名称
 */
auto Backend::startImpl(std::string_view node_name, std::string_view thread_name) -> void {
  OHNO_ASSERT(!node_name.empty());
  OHNO_ASSERT(center_ != nullptr);
  OHNO_ASSERT(nic_ != nullptr);

  if (running_) {
    return;
  }
  running_ = true;

  auto interval = interval_ == 0 ? DEFAULT_INTERVAL : interval_;
  monitor_ = std::thread{[&running = running_,
                          callback = std::bind(&Backend::eventHandler, this, node_name),
                          interv = interval, name = thread_name]() {
    try {
      pthread_setname_np(pthread_self(), name.data());

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
 * @brief 触发事件（由派生类实现）
 *
 * @param current_node 当前 Kubernetes 节点名称
 */
auto Backend::eventHandler(std::string_view current_node) -> void {
  OHNO_ASSERT(!current_node.empty());
}

} // namespace backend
} // namespace ohno
