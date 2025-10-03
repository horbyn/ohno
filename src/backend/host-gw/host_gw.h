#pragma once

// clang-format off
#include <atomic>
#include <memory>
#include <thread>
#include <unordered_map>
#include "src/backend/backend_if.h"
#include "src/backend/center_if.h"
#include "src/ipam/ipam_if.h"
#include "src/log/logger.h"
#include "src/net/nic_if.h"
#include "src/net/netlink/netlink_if.h"
// clang-format on

namespace ohno {
namespace backend {

constexpr int DEFAULT_INTERVAL{1};

class HostGw : public BackendIf, public log::Loggable<log::Id::backend> {
public:
  auto start(std::string_view node_name) -> void override;
  auto stop() -> void override;

  auto setInterval(int sec) -> void;
  auto setCenter(std::unique_ptr<backend::CenterIf> center) -> void;
  auto setIpam(std::unique_ptr<ipam::IpamIf> ipam) -> void;
  auto setNic(std::unique_ptr<net::NicIf> nic, std::weak_ptr<net::NetlinkIf> netlink) -> bool;

private:
  auto eventHandler(std::string_view current_node) -> void;

  int interval_;
  std::unique_ptr<backend::CenterIf> center_;
  std::unique_ptr<ipam::IpamIf> ipam_;
  std::unique_ptr<net::NicIf> nic_;
  std::unordered_map<std::string, backend::NodeInfo> node_cache_;
  std::atomic<bool> running_;
  std::thread monitor_;
};

} // namespace backend
} // namespace ohno
