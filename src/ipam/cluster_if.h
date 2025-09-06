#pragma once

// clang-format off
#include <memory>
#include <string_view>
#include "node_if.h"
// clang-format on

namespace ohno {
namespace ipam {

class ClusterIf {
public:
  virtual ~ClusterIf() = default;
  virtual auto addNode(std::string_view node_name, std::shared_ptr<NodeIf> node) -> void = 0;
  virtual auto delNode(std::string_view node_name) -> void = 0;
  virtual auto getNode(std::string_view node_name) const -> std::shared_ptr<NodeIf> = 0;

  // 静态路由实现: cluster 要发布静态路由需要通知所有 node，所以 cluster 与 node 构成
  // subject-observe 模式。当节点间通信不需要静态路由时，这个接口可以删除
  virtual auto NotifyAdd() -> void = 0;
  virtual auto NotifyDel(std::string network, std::string via) -> void = 0;
};

} // namespace ipam
} // namespace ohno
