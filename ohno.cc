// clang-format off
#include <cstdlib>
#include <exception>
#include <iostream>
#include "src/common/except.h"
#include "src/log/logger.h"
// clang-format on

auto main(int argc, char **argv) -> int {
  using namespace ohno;
  (void)argc;
  (void)argv;

  try {
    OHNO_GLOBAL_LOG(info, "启动 ohno");

  } catch (const ohno::except::Success &suc) {
    return EXIT_SUCCESS;
  } catch (const ohno::except::Exception &exc) {
    std::cout << "[error] " << exc.getMsg() << "\n";
    return EXIT_FAILURE;
  } catch (const std::exception &exc) {
    std::cout << "[error] " << exc.what() << "\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
