// clang-format off
#include "route.h"
// clang-format on

namespace ohno {
namespace net {

Route::Route(std::string_view dest, std::string_view via, std::string_view dev)
    : dest_{dest}, via_{via}, dev_{dev} {}

auto Route::getDest() const -> std::string { return dest_; }

auto Route::getVia() const -> std::string { return via_; }

auto Route::getDev() const -> std::string { return dev_; }

} // namespace net
} // namespace ohno
