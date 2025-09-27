#pragma once

// clang-format off
#include "backend_if.h"
#include <atomic>
#include <thread>
#include "src/log/logger.h"
// clang-format on

namespace ohno {
namespace backend {

class Backend : public BackendIf, public log::Loggable<log::Id::backend> {
public:
  auto start(cni::CniConfigIpam::Mode mode, std::string_view node_name,
             std::weak_ptr<net::NetlinkIf> netlink, const BackendInfo &backend_info)
      -> void override;
  auto stop() -> void override;

private:
  std::atomic<bool> running_;
  std::thread monitor_;
};

} // namespace backend
} // namespace ohno
