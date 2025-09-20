#pragma once

// clang-format off
#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include "netns_if.h"
// clang-format on

namespace ohno {
namespace ipam {

class NodeIf {
public:
  virtual ~NodeIf() = default;
  virtual auto setName(std::string_view node_name) -> void = 0;
  virtual auto getName() const -> std::string = 0;
  virtual auto addNetns(std::string_view netns_name, std::shared_ptr<NetnsIf> netns) -> void = 0;
  virtual auto delNetns(std::string_view netns_name) -> void = 0;
  virtual auto getNetns(std::string_view netns_name) const -> std::shared_ptr<NetnsIf> = 0;
  virtual auto getNetnsSize() const noexcept -> size_t = 0;
  virtual auto setSubnet(std::string_view subnet) -> void = 0;
  virtual auto getSubnet() const -> std::string = 0;
  virtual auto setUnderlayAddr(std::string_view underlay_addr) -> void = 0;
  virtual auto getUnderlayAddr() const -> std::string = 0;
  virtual auto setUnderlayDev(std::string_view underlay_dev) -> void = 0;
  virtual auto getUnderlayDev() const -> std::string = 0;
};

} // namespace ipam
} // namespace ohno
