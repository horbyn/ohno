// clang-format off
#include "cluster.h"
#include "src/common/assert.h"
// clang-format on

namespace ohno {
namespace ipam {

/**
 * @brief 向 Kubernetes 集群中增加一个节点
 *
 * @param node_name 节点名称
 * @param node 节点对象
 */
auto Cluster::addNode(std::string_view node_name, std::shared_ptr<NodeIf> node) -> void {
  nodes_.emplace(node_name, node);
}

/**
 * @brief 从 Kubernetes 集群中删除一个节点
 *
 * @param node_name 节点名称
 */
auto Cluster::delNode(std::string_view node_name) -> void {
  OHNO_ASSERT(!node_name.empty());
  nodes_.erase(node_name.data());
}

/**
 * @brief 获取 Kubernetes 集群中的节点
 *
 * @param node_name 节点名称
 * @return std::shared_ptr<NodeIf> 节点对象
 */
auto Cluster::getNode(std::string_view node_name) const -> std::shared_ptr<NodeIf> {
  OHNO_ASSERT(!node_name.empty());
  auto iter = nodes_.find(node_name.data());
  if (iter == nodes_.end()) {
    OHNO_LOG(error, "cannot find node {}", node_name);
    return nullptr;
  }
  return iter->second;
}

/**
 * @brief 通知事件，需要添加静态路由
 *
 */
auto Cluster::NotifyAdd() -> void {
  std::vector<std::shared_ptr<NodeIf>> nodes{};
  std::transform(nodes_.begin(), nodes_.end(), std::back_inserter(nodes),
                 [](const auto &pair) { return pair.second; });

  for (const auto &pair : nodes_) {
    pair.second->onStaticRouteAdd(nodes);
  }
}

/**
 * @brief 通知事件，需要删除静态路由
 *
 * @param network 已经删除的网段
 * @param via 通过该网段的网关
 */
auto Cluster::NotifyDel(std::string network, std::string via) -> void {
  std::vector<std::shared_ptr<NodeIf>> nodes{};
  std::transform(nodes_.begin(), nodes_.end(), std::back_inserter(nodes),
                 [](const auto &pair) { return pair.second; });

  for (const auto &pair : nodes_) {
    pair.second->onStaticRouteDel(nodes, network, via);
  }
}

} // namespace ipam
} // namespace ohno
