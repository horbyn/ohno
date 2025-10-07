#pragma once

// clang-format off
#include "mac_if.h"
#include <array>
#include <string_view>
#include "src/log/logger.h"
// clang-format on

namespace ohno {
namespace net {

constexpr size_t SZ_MACLEN_NATIVE{6};
constexpr size_t SZ_MACLEN_STRING{SZ_MACLEN_NATIVE * 3};

class Mac : public MacIf, public log::Loggable<log::Id::net> {
public:
  explicit Mac(std::string_view nic_name);

  auto getMac() -> std::string override;

private:
  using MacAddress = std::array<uint8_t, SZ_MACLEN_NATIVE>;
  auto setMacNative(std::string_view nic_name) -> bool;
  auto setMacStr(MacAddress mac) -> bool;

  MacAddress mac_native_;
  std::string mac_str_;
};

} // namespace net
} // namespace ohno
