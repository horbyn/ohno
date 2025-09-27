#pragma once

// clang-format off
#include <memory>
#include "netlink_if.h"
#include "src/log/logger.h"
#include "src/util/shell_if.h"
// clang-format on

namespace ohno {
namespace net {

class NetlinkIpCmd : public NetlinkIf, public log::Loggable<log::Id::net> {
public:
  explicit NetlinkIpCmd(std::unique_ptr<util::ShellIf> shell);

  auto linkDestory(std::string_view name, std::string_view netns = {}) -> bool override;
  auto linkExist(std::string_view name, std::string_view netns = {}) -> bool override;
  auto linkSetStatus(std::string_view name, LinkStatus status, std::string_view netns = {})
      -> bool override;
  auto linkIsInNetns(std::string_view name, std::string_view netns) -> bool override;
  auto linkToNetns(std::string_view name, std::string_view netns) -> bool override;
  auto linkRename(std::string_view name, std::string_view new_name, std::string_view netns = {})
      -> bool override;
  auto vethCreate(std::string_view name1, std::string_view name2) -> bool override;
  auto bridgeCreate(std::string_view name) -> bool override;
  auto bridgeSetStatus(std::string_view name, bool master, std::string_view bridge,
                       std::string_view netns = {}) -> bool override;
  auto addressIsExist(std::string_view name, std::string_view addr, std::string_view netns = {})
      -> bool override;
  auto addressSetEntry(std::string_view name, std::string_view addr, bool add,
                       std::string_view netns = {}) -> bool override;
  auto routeIsExist(std::string_view dst, std::string_view via, std::string_view dev = {},
                    std::string_view netns = {}) const -> bool override;
  auto routeSetEntry(std::string_view dst, std::string_view via, bool add,
                     std::string_view dev = {}, std::string_view netns = {}) const -> bool override;

private:
  static auto addNetns(std::string_view command, std::string_view netns = {}) -> std::string;
  template <typename... Args>
  auto executeCommand(std::string_view command, std::string_view error_message,
                      Args &&...args) const -> bool;

  std::unique_ptr<util::ShellIf> shell_;
};

} // namespace net
} // namespace ohno

#include "netlink_ip_cmd.tpp"
