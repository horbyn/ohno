#pragma once

// clang-format off
#include "cluster_if.h"
#include "src/log/logger.h"
// clang-format on

namespace ohno {
namespace ipam {

class Cluster : public ClusterIf, public log::Loggable<log::Id::ipam> {
public:
  auto addNode(std::string_view node_name, std::shared_ptr<NodeIf> node) -> void override;
  auto delNode(std::string_view node_name) -> void override;
  auto getNode(std::string_view node_name) const -> std::shared_ptr<NodeIf> override;

private:
  std::unordered_map<std::string, std::shared_ptr<NodeIf>> nodes_;
};

} // namespace ipam
} // namespace ohno
