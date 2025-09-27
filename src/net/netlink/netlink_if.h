#pragma once

// clang-format off
#include <string_view>
#include "src/net/macro.h"
// clang-format on

namespace ohno {
namespace net {

class NetlinkIf {
public:
  virtual ~NetlinkIf() = default;
  virtual auto linkDestory(std::string_view name, std::string_view netns = {}) -> bool = 0;
  virtual auto linkExist(std::string_view name, std::string_view netns = {}) -> bool = 0;
  virtual auto linkSetStatus(std::string_view name, LinkStatus status, std::string_view netns = {})
      -> bool = 0;
  virtual auto linkIsInNetns(std::string_view name, std::string_view netns) -> bool = 0;
  virtual auto linkToNetns(std::string_view name, std::string_view netns) -> bool = 0;
  virtual auto linkRename(std::string_view name, std::string_view new_name,
                          std::string_view netns = {}) -> bool = 0;
  virtual auto vethCreate(std::string_view name1, std::string_view name2) -> bool = 0;
  virtual auto bridgeCreate(std::string_view name) -> bool = 0;
  virtual auto bridgeSetStatus(std::string_view name, bool master, std::string_view bridge,
                               std::string_view netns = {}) -> bool = 0;
  virtual auto addressIsExist(std::string_view name, std::string_view addr,
                              std::string_view netns = {}) -> bool = 0;
  virtual auto addressSetEntry(std::string_view name, std::string_view addr, bool add,
                               std::string_view netns = {}) -> bool = 0;
  virtual auto routeIsExist(std::string_view dst, std::string_view via, std::string_view dev = {},
                            std::string_view netns = {}) const -> bool = 0;
  virtual auto routeSetEntry(std::string_view dst, std::string_view via, bool add,
                             std::string_view dev = {}, std::string_view netns = {}) const
      -> bool = 0;
};

} // namespace net
} // namespace ohno
