// clang-format off
#include "neigh.h"
// clang-format on

namespace ohno {
namespace net {

Neigh::Neigh(std::string_view addr, std::string_view mac, std::string_view dev)
    : addr_{addr}, mac_{mac}, dev_{dev} {}

auto Neigh::getAddr() const -> std::string { return addr_; }

auto Neigh::getMac() const -> std::string { return mac_; }

auto Neigh::getDev() const -> std::string { return dev_; }

} // namespace net
} // namespace ohno
