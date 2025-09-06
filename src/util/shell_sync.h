#pragma once

// clang-format off
#include "shell_if.h"
#include "src/log/logger.h"
// clang-format on

namespace ohno {
namespace util {

class ShellSync final : public ShellIf, public log::Loggable<log::Id::util> {
public:
  auto execute(std::string_view command, std::string &out) -> bool override;
  auto execute(std::string_view command, std::string &out, std::string &err) -> int override;
};

} // namespace util
} // namespace ohno
