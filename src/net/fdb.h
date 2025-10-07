#pragma once

// clang-format off
#include "fdb_if.h"
#include <string_view>
#include "src/log/logger.h"
// clang-format on

namespace ohno {
namespace net {

class Fdb : public FdbIf, public log::Loggable<log::Id::net> {
public:
  explicit Fdb(std::string_view mac, std::string_view underlay_addr, std::string_view dev);

  auto getMac() const -> std::string override;
  auto getUnderlayAddr() const -> std::string override;
  auto getDev() const -> std::string override;

private:
  std::string mac_;
  std::string underlay_addr_;
  std::string dev_;
};

} // namespace net
} // namespace ohno
