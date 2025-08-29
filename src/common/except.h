#pragma once

// clang-format off
#include <stdexcept>
#include <string_view>
// clang-format on

namespace ohno {
namespace except {

class Exception : public std::exception {
public:
  explicit Exception() = default;
  virtual ~Exception() = default;
  explicit Exception(std::string_view file, int line, std::string_view msg, bool is_errno);

  auto getMsg() const -> std::string;

private:
  std::string msg_;
  int err_;
};

class Success : public Exception {
public:
  virtual ~Success() = default;
};

} // namespace except

#define EXOHNO(msg, is_errno) (except::Exception((__FILE__), (__LINE__), (msg), (is_errno)))

} // namespace ohno
