#pragma once

// clang-format off
#include <mutex>
#include "env_if.h"
#include "src/log/logger.h"
// clang-format on

namespace ohno {
namespace util {

class EnvStd final : public EnvIf, public log::Loggable<log::Id::util> {
public:
  auto get(std::string_view env) const noexcept -> std::string override;
  auto exist(std::string_view env) const noexcept -> bool override;
  auto set(std::string_view env, std::string_view value) const noexcept -> bool override;
  auto unset(std::string_view env) const noexcept -> bool override;

private:
  mutable std::mutex mtx_;
};

} // namespace util
} // namespace ohno
