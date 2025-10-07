// clang-format off
#include "fdb.h"
// clang-format on

namespace ohno {
namespace net {

Fdb::Fdb(std::string_view mac, std::string_view underlay_addr, std::string_view dev)
    : mac_{mac}, underlay_addr_{underlay_addr}, dev_{dev} {}

auto Fdb::getMac() const -> std::string { return mac_; }

auto Fdb::getUnderlayAddr() const -> std::string { return underlay_addr_; }

auto Fdb::getDev() const -> std::string { return dev_; }

} // namespace net
} // namespace ohno
