#pragma once

// clang-format off
#include "route_if.h"
#include "src/log/logger.h"
// clang-format on

namespace ohno {
namespace net {

class Route : public RouteIf, public log::Loggable<log::Id::net> {
public:
  explicit Route(std::string_view dest, std::string_view via, std::string_view dev);

  auto getDest() const -> std::string override;
  auto getVia() const -> std::string override;
  auto getDev() const -> std::string override;

  std::string dest_;
  std::string via_;
  std::string dev_;
};

} // namespace net
} // namespace ohno
