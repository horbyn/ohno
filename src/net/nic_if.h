#pragma once

// clang-format off
#include <memory>
#include <string>
#include <string_view>
#include "macro.h"
#include "addr_if.h"
#include "route_if.h"
// clang-format on

namespace ohno {
namespace net {

class NicIf {
public:
  virtual ~NicIf() = default;
  virtual auto setName(std::string_view name) -> void = 0;
  virtual auto getName() const -> std::string = 0;
  virtual auto rename(std::string_view name) -> bool = 0;
  virtual auto addNetns(std::string_view netns) -> void = 0;
  virtual auto delNetns() -> void = 0;
  virtual auto getNetns() const -> std::string = 0;
  virtual auto setStatus(LinkStatus status) -> bool = 0;
  virtual auto getStatus() const noexcept -> bool = 0;
  virtual auto getIndex() const -> uint32_t = 0;
  virtual auto addAddr(std::shared_ptr<AddrIf> addr) -> bool = 0;
  virtual auto delAddr(std::string_view cidr) -> bool = 0;
  virtual auto getAddr(std::string_view cidr = {}) const -> std::shared_ptr<AddrIf> = 0;
  virtual auto addRoute(std::shared_ptr<RouteIf> route) -> bool = 0;
  virtual auto delRoute(std::string_view dst, std::string_view via, std::string_view dev)
      -> bool = 0;
  virtual auto getRoute(std::string_view dst, std::string_view via, std::string_view dev) const
      -> std::shared_ptr<RouteIf> = 0;
  virtual auto cleanup() noexcept -> void = 0;
};

} // namespace net
} // namespace ohno
