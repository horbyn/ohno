#pragma once

// clang-format off
#include "scheduler_if.h"
#include "src/log/logger.h"
// clang-format on

namespace ohno {
namespace backend {

class Scheduler : public SchedulerIf, public log::Loggable<log::Id::backend> {
public:
  auto setStrategy(std::unique_ptr<BackendIf> backend) -> void override;
  auto start(std::string_view node_name, std::weak_ptr<ohno::net::NetlinkIf> netlink,
             const ohno::backend::BackendInfo &backend_info) -> void override;
  auto stop() -> void override;

private:
  std::unique_ptr<BackendIf> backend_;
};

} // namespace backend
} // namespace ohno
