#pragma once

// clang-format off
#include "nic_if.h"
#include <vector>
#include "src/log/logger.h"
// clang-format on

namespace ohno {
namespace net {

class Nic : public NicIf, public log::Loggable<log::Id::net> {
public:
  auto setup(std::weak_ptr<NetlinkIf> netlink) -> bool override;
  auto cleanup() -> void override;
  auto isExist() const -> bool override;
  auto setName(std::string_view name) -> void override;
  auto getName() const -> std::string override;
  auto rename(std::string_view name) -> bool override;
  auto setNetns(std::string_view netns) -> bool override;
  auto getNetns() const -> std::string override;
  auto setStatus(LinkStatus status) -> bool override;
  auto getStatus() const noexcept -> bool override;
  auto addAddr(std::unique_ptr<AddrIf> addr) -> bool override;
  auto delAddr(std::string_view cidr) -> bool override;
  auto getAddr(std::string_view cidr = {}) const -> const AddrIf * override;
  auto addRoute(std::unique_ptr<RouteIf> route, NetlinkIf::RouteNHFlags nhflags) -> bool override;
  auto delRoute(std::string_view dst, std::string_view via, std::string_view dev) -> bool override;
  auto getRoute(std::string_view dst, std::string_view via, std::string_view dev) const
      -> const RouteIf * override;
  auto addNeigh(std::unique_ptr<NeighIf> neigh) -> bool override;
  auto delNeigh(std::string_view addr, std::string_view mac, std::string_view dev) -> bool override;
  auto addFdb(std::unique_ptr<FdbIf> fdb) -> bool override;
  auto delFdb(std::string_view addr, std::string_view mac, std::string_view dev) -> bool override;

  enum class Type : uint8_t { SYS, USER };
  auto getType() const noexcept -> Type;

  static auto simpleNetns(std::string_view netns) -> std::string;

protected:
  Type type_{Type::USER};
  std::weak_ptr<NetlinkIf> netlink_; // 仅观察，不持有所有权

private:
  std::string name_;
  std::string netns_;
  LinkStatus status_;
  int index_;

  // 如果 AddrIf / RouteIf 需要反过来引用 Nic 则改为 weak_ptr

  std::vector<std::unique_ptr<AddrIf>> addrs_;
  std::vector<std::unique_ptr<RouteIf>> routes_;
  std::vector<std::unique_ptr<NeighIf>> neighs_;
  std::vector<std::unique_ptr<FdbIf>> fdbs_;
};

} // namespace net
} // namespace ohno
