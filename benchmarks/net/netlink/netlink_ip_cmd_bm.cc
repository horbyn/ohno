// clang-format off
#include <map>
#include <string>
#include <string_view>
#include <tuple>
#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "benchmark/benchmark.h"
#include "src/net/netlink/netlink_ip_cmd.h"
// clang-format on

using namespace ohno::net;
using namespace ohno::util;

namespace {
std::map<std::string, std::tuple<std::string, std::string, int>> command_responses{};
}

class MockShell : public ShellIf {
public:
  MOCK_METHOD(bool, execute, (std::string_view command, std::string &output), (override));
  MOCK_METHOD(int, execute, (std::string_view command, std::string &output, std::string &error),
              (override));

  void setDefaultBehavior() {
    // 设置默认行为：所有命令都成功执行
    ON_CALL(*this, execute(testing::_, testing::_, testing::_))
        .WillByDefault([](std::string_view command, std::string &output, std::string &error) {
          auto it = command_responses.find(command.data());
          if (it != command_responses.end()) {
            output = std::get<0>(it->second);
            error = std::get<1>(it->second);
            return std::get<2>(it->second);
          }

          // 默认行为：命令成功执行
          output = "Mock output for: " + std::string{command};
          error = "";
          return 0;
        });

    ON_CALL(*this, execute(testing::_, testing::_))
        .WillByDefault([](std::string_view command, std::string &output) {
          std::string error;
          int result = 0;

          auto it = command_responses.find(command.data());
          if (it != command_responses.end()) {
            output = std::get<0>(it->second);
            error = std::get<1>(it->second);
            result = std::get<2>(it->second);
          } else {
            // 默认行为：命令成功执行
            output = "Mock output for: " + std::string{command};
          }

          return result == 0;
        });
  }

  // 设置特定命令的行为
  void setCommandBehavior(std::string_view command, std::string_view output, int return_code = 0,
                          std::string_view error = "") {
    command_responses[std::string{command}] = std::make_tuple(output, error, return_code);
  }
};

static void BM_NetlinkIpCmd_Constructor(benchmark::State &state) {
  for (auto _ : state) {
    auto mock_shell = std::make_unique<MockShell>();
    mock_shell->setDefaultBehavior();

    ohno::net::NetlinkIpCmd netlink(std::move(mock_shell));
    benchmark::DoNotOptimize(netlink);
  }
}
BENCHMARK(BM_NetlinkIpCmd_Constructor);

BENCHMARK_MAIN();
