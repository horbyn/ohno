#pragma once

// clang-format off
#include <unordered_map>
#include "src/backend/backend.h"
#include "src/net/vrf.h"
#include "src/net/vxlan.h"
// clang-format on

namespace ohno {
namespace backend {

class Evpn : public Backend {
public:
  auto start(std::string_view node_name) -> void override;
  auto stop() -> void override;

  auto setVrf(std::unique_ptr<net::Vrf> vrf) -> void;
  auto setl3Bridge(std::unique_ptr<net::Bridge> br) -> void;
  auto setl2Bridge(std::unique_ptr<net::Bridge> br) -> void;
  auto setVxlan(std::unique_ptr<net::Vxlan> vtep) -> void;

private:
  auto envSetup() const -> void;
  auto envCleanup() const noexcept -> void;

  std::unordered_map<std::string, backend::NodeInfo> node_cache_;

  // BackendIf::nic_ 是用来添加宿主机路由、ARP 缓存或 FDB 表项的，EVPN 不需要

  std::unique_ptr<net::Vrf> vrf_;
  std::unique_ptr<net::Bridge> bridge_l3_;
  std::unique_ptr<net::Bridge> bridge_l2_;
  std::unique_ptr<net::Vxlan> vxlan_;
};

} // namespace backend
} // namespace ohno
