#pragma once

// clang-format off
#include <string_view>
#include "scheduler_if.h"
#include "src/backend/backend_info.h"
#include "src/cni/cni_config.h"
#include "src/log/logger.h"
#include "src/net/netlink/netlink_if.h"
// clang-format on

namespace ohno {
namespace backend {

class StrategyClient final : public log::Loggable<log::Id::backend> {
public:
  auto setNetlink(std::weak_ptr<net::NetlinkIf> netlink) -> bool;
  auto setBackendInfo(const BackendInfo &bkinfo) -> void;
  auto executeStrategy(std::string_view node_name) -> void;
  auto stopStrategy() -> void;

private:
  auto getStrategy(cni::CniConfigIpam::Mode mode) const -> std::unique_ptr<BackendIf>;
  auto getHostgw() const -> std::unique_ptr<BackendIf>;
  auto getVxlan() const -> std::unique_ptr<BackendIf>;

  std::unique_ptr<SchedulerIf> scheduler_;
  std::shared_ptr<net::NetlinkIf> netlink_; // TODO: 外部对象必须一直存在, 但实际可能不会
  BackendInfo bkinfo_;
};

} // namespace backend
} // namespace ohno
