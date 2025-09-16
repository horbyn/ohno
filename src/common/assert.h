#pragma once

// clang-format off
#include <cassert>
#include <cstdlib>
#include <iostream>
// clang-format on

namespace ohno {

// ohno 假设失效时行为: Release 直接输出 stderr, Debug 立即终止以便 coredump
#ifdef NDEBUG
#define OHNO_ACTION()                                                                              \
  std::cerr << "ohno assertion failed in " << (__FILE__) << ":" << (__LINE__) << "\n"
#else
#define OHNO_ACTION() std::abort()
#endif

// ohno 运行时假设性检查
#define OHNO_ASSERT(expr)                                                                          \
  do {                                                                                             \
    assert((expr));                                                                                \
    if (!(expr)) {                                                                                 \
      OHNO_ACTION();                                                                               \
    }                                                                                              \
  } while (0)

// ohno 编译时假设性检查
#define OHNO_STATIC_ASSERT(expr, comment) static_assert((expr), comment)

} // namespace ohno
