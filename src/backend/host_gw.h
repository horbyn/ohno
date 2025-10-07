#pragma once

// clang-format off
#include <unordered_map>
#include "src/backend/backend.h"
#include "src/ipam/ipam_if.h"
// clang-format on

namespace ohno {
namespace backend {

class HostGw : public Backend {
public:
  auto start(std::string_view node_name) -> void override;

  auto setIpam(std::unique_ptr<ipam::IpamIf> ipam) -> void;

protected:
  auto eventHandler(std::string_view current_node) -> void override;

private:
  std::unordered_map<std::string, backend::NodeInfo> node_cache_;
  std::unique_ptr<ipam::IpamIf> ipam_;
};

} // namespace backend
} // namespace ohno
