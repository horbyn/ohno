#pragma once

// clang-format off
#include "nic_if.h"
#include <vector>
#include "src/log/logger.h"
#include "src/net/netlink/netlink_if.h"
// clang-format on

namespace ohno {
namespace net {

class Nic : public NicIf, public log::Loggable<log::Id::net> {
public:
  explicit Nic(const std::shared_ptr<NetlinkIf> &netlink);

  auto setName(std::string_view name) -> void override;
  auto getName() const -> std::string override;
  auto rename(std::string_view name) -> bool override;
  auto addNetns(std::string_view netns) -> void override;
  auto delNetns() -> void override;
  auto getNetns() const -> std::string override;
  auto setStatus(LinkStatus status) -> bool override;
  auto getStatus() const noexcept -> bool override;
  auto getIndex() const -> uint32_t override;
  auto addAddr(std::shared_ptr<AddrIf> addr) -> bool override;
  auto delAddr(std::string_view cidr) -> bool override;
  auto getAddr(std::string_view cidr = {}) const -> std::shared_ptr<AddrIf> override;
  auto addRoute(std::shared_ptr<RouteIf> route) -> bool override;
  auto delRoute(std::string_view dst, std::string_view via, std::string_view dev) -> bool override;
  auto getRoute(std::string_view dst, std::string_view via, std::string_view dev) const
      -> std::shared_ptr<RouteIf> override;
  auto cleanup() noexcept -> void override;

protected:
  std::weak_ptr<NetlinkIf> netlink_; // 仅观察，不持有所有权

private:
  std::string name_;
  std::string netns_;
  LinkStatus status_;
  int index_;

  // 如果 AddrIf / RouteIf 需要反过来引用 Nic 则改为 weak_ptr

  std::vector<std::shared_ptr<AddrIf>> addrs_;
  std::vector<std::shared_ptr<RouteIf>> routes_;
};

} // namespace net
} // namespace ohno
