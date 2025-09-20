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
};

} // namespace ipam
} // namespace ohno
