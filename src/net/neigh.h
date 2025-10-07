#pragma once

// clang-format off
#include "neigh_if.h"
#include <string_view>
#include "src/log/logger.h"
// clang-format on

namespace ohno {
namespace net {

class Neigh : public NeighIf, public log::Loggable<log::Id::net> {
public:
  explicit Neigh(std::string_view addr, std::string_view mac, std::string_view dev);

  auto getAddr() const -> std::string override;
  auto getMac() const -> std::string override;
  auto getDev() const -> std::string override;

private:
  std::string addr_;
  std::string mac_;
  std::string dev_;
};

} // namespace net
} // namespace ohno
