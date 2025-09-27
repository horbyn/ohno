#pragma once

// clang-format off
#include <memory>
#include <string>
#include <string_view>
#include "macro.h"
#include "addr_if.h"
#include "route_if.h"
#include "src/net/netlink/netlink_if.h"
// clang-format on

namespace ohno {
namespace net {

class NicIf {
public:
  virtual ~NicIf() = default;
  virtual auto setup(std::weak_ptr<NetlinkIf> netlink) -> bool = 0;
  virtual auto cleanup() -> void = 0;
  virtual auto isExist() const -> bool = 0;
  virtual auto setName(std::string_view name) -> void = 0;
  virtual auto getName() const -> std::string = 0;
  virtual auto rename(std::string_view name) -> bool = 0;
  virtual auto setNetns(std::string_view netns) -> bool = 0;
  virtual auto getNetns() const -> std::string = 0;
  virtual auto setStatus(LinkStatus status) -> bool = 0;
  virtual auto getStatus() const noexcept -> bool = 0;
  virtual auto addAddr(std::unique_ptr<AddrIf> addr) -> bool = 0;
  virtual auto delAddr(std::string_view cidr) -> bool = 0;
  virtual auto getAddr(std::string_view cidr = {}) const -> const AddrIf * = 0;
  virtual auto addRoute(std::unique_ptr<RouteIf> route) -> bool = 0;
  virtual auto delRoute(std::string_view dst, std::string_view via, std::string_view dev)
      -> bool = 0;
  virtual auto getRoute(std::string_view dst, std::string_view via, std::string_view dev) const
      -> const RouteIf * = 0;
};

} // namespace net
} // namespace ohno
