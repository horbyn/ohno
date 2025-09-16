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

  // 默认行为
  void setBehavior() {
    EXPECT_CALL(*this, execute(testing::_, testing::_, testing::_))
        .WillRepeatedly([](std::string_view command, std::string &output, std::string &error) {
          auto it = command_responses.find(command.data());
          if (it != command_responses.end()) {
            output = std::get<0>(it->second);
            error = std::get<1>(it->second);
            return std::get<2>(it->second);
          }

          // 命令成功执行
          output = "Mock output for: " + std::string{command};
          error = "";
          return 0;
        });

    EXPECT_CALL(*this, execute(testing::_, testing::_))
        .WillRepeatedly([](std::string_view command, std::string &output) {
          std::string error;
          int result = 0;

          auto it = command_responses.find(command.data());
          if (it != command_responses.end()) {
            output = std::get<0>(it->second);
            error = std::get<1>(it->second);
            result = std::get<2>(it->second);
          } else {
            // 命令成功执行
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

    ohno::net::NetlinkIpCmd netlink(std::move(mock_shell));
    benchmark::DoNotOptimize(netlink);
  }
}
BENCHMARK(BM_NetlinkIpCmd_Constructor);

static void BM_NetlinkIpCmd_LinkDestroy(benchmark::State &state) {
  auto mock_shell = std::make_unique<MockShell>();
  mock_shell->setBehavior();

  ohno::net::NetlinkIpCmd netlink(std::move(mock_shell));

  for (auto _ : state) {
    netlink.linkDestory("test_interface");
  }
}
BENCHMARK(BM_NetlinkIpCmd_LinkDestroy);

static void BM_NetlinkIpCmd_LinkSetStatusUp(benchmark::State &state) {
  auto mock_shell = std::make_unique<MockShell>();
  mock_shell->setBehavior();

  ohno::net::NetlinkIpCmd netlink(std::move(mock_shell));

  for (auto _ : state) {
    netlink.linkSetStatus("test_interface", ohno::net::LinkStatus::UP);
  }
}
BENCHMARK(BM_NetlinkIpCmd_LinkSetStatusUp);

static void BM_NetlinkIpCmd_LinkSetStatusDown(benchmark::State &state) {
  auto mock_shell = std::make_unique<MockShell>();
  mock_shell->setBehavior();

  ohno::net::NetlinkIpCmd netlink(std::move(mock_shell));

  for (auto _ : state) {
    netlink.linkSetStatus("test_interface", ohno::net::LinkStatus::DOWN);
  }
}
BENCHMARK(BM_NetlinkIpCmd_LinkSetStatusDown);

static void BM_NetlinkIpCmd_LinkToNetns(benchmark::State &state) {
  auto mock_shell = std::make_unique<MockShell>();
  mock_shell->setBehavior();

  ohno::net::NetlinkIpCmd netlink(std::move(mock_shell));

  for (auto _ : state) {
    netlink.linkToNetns("test_interface", "test_netns");
  }
}
BENCHMARK(BM_NetlinkIpCmd_LinkToNetns);

static void BM_NetlinkIpCmd_LinkRename(benchmark::State &state) {
  auto mock_shell = std::make_unique<MockShell>();
  mock_shell->setBehavior();

  ohno::net::NetlinkIpCmd netlink(std::move(mock_shell));

  for (auto _ : state) {
    netlink.linkRename("old_name", "new_name");
  }
}
BENCHMARK(BM_NetlinkIpCmd_LinkRename);

static void BM_NetlinkIpCmd_VethCreate(benchmark::State &state) {
  auto mock_shell = std::make_unique<MockShell>();
  mock_shell->setBehavior();

  ohno::net::NetlinkIpCmd netlink(std::move(mock_shell));

  for (auto _ : state) {
    netlink.vethCreate("veth1", "veth2");
  }
}
BENCHMARK(BM_NetlinkIpCmd_VethCreate);

static void BM_NetlinkIpCmd_BridgeCreate(benchmark::State &state) {
  auto mock_shell = std::make_unique<MockShell>();
  mock_shell->setBehavior();

  ohno::net::NetlinkIpCmd netlink(std::move(mock_shell));

  for (auto _ : state) {
    netlink.bridgeCreate("br0");
  }
}
BENCHMARK(BM_NetlinkIpCmd_BridgeCreate);

static void BM_NetlinkIpCmd_AddressSetEntryAdd(benchmark::State &state) {
  auto mock_shell = std::make_unique<MockShell>();
  mock_shell->setBehavior();

  ohno::net::NetlinkIpCmd netlink(std::move(mock_shell));

  for (auto _ : state) {
    netlink.addressSetEntry("eth0", "192.168.1.1/24", true);
  }
}
BENCHMARK(BM_NetlinkIpCmd_AddressSetEntryAdd);

static void BM_NetlinkIpCmd_AddressSetEntryDel(benchmark::State &state) {
  auto mock_shell = std::make_unique<MockShell>();
  mock_shell->setBehavior();

  ohno::net::NetlinkIpCmd netlink(std::move(mock_shell));

  for (auto _ : state) {
    netlink.addressSetEntry("eth0", "192.168.1.1/24", false);
  }
}
BENCHMARK(BM_NetlinkIpCmd_AddressSetEntryDel);

static void BM_NetlinkIpCmd_RouteIsExist_Exists(benchmark::State &state) {
  // 测试路由存在

  auto mock_shell = std::make_unique<MockShell>();
  mock_shell->setCommandBehavior("ip route show default via 192.168.1.1 dev eth0",
                                 "default via 192.168.1.1 dev eth0");
  mock_shell->setBehavior();

  ohno::net::NetlinkIpCmd netlink(std::move(mock_shell));

  for (auto _ : state) {
    bool exists = netlink.routeIsExist("", "192.168.1.1", "eth0");
    benchmark::DoNotOptimize(exists);
  }
}
BENCHMARK(BM_NetlinkIpCmd_RouteIsExist_Exists);

static void BM_NetlinkIpCmd_RouteIsExist_NotExists(benchmark::State &state) {
  // 测试路由不存在

  auto mock_shell = std::make_unique<MockShell>();
  // 设置特定命令的行为（空输出表示路由不存在）
  mock_shell->setCommandBehavior("ip route show default via 192.168.1.1 dev eth0", "");
  mock_shell->setBehavior();

  ohno::net::NetlinkIpCmd netlink(std::move(mock_shell));

  for (auto _ : state) {
    bool exists = netlink.routeIsExist("", "192.168.1.1", "eth0");
    benchmark::DoNotOptimize(exists);
  }
}
BENCHMARK(BM_NetlinkIpCmd_RouteIsExist_NotExists);

static void BM_NetlinkIpCmd_RouteSetEntryAdd(benchmark::State &state) {
  auto mock_shell = std::make_unique<MockShell>();
  mock_shell->setBehavior();

  ohno::net::NetlinkIpCmd netlink(std::move(mock_shell));

  for (auto _ : state) {
    netlink.routeSetEntry("192.168.2.0/24", "192.168.1.1", true, "eth0");
  }
}
BENCHMARK(BM_NetlinkIpCmd_RouteSetEntryAdd);

static void BM_NetlinkIpCmd_RouteSetEntryDel(benchmark::State &state) {
  auto mock_shell = std::make_unique<MockShell>();
  mock_shell->setBehavior();

  ohno::net::NetlinkIpCmd netlink(std::move(mock_shell));

  for (auto _ : state) {
    netlink.routeSetEntry("192.168.2.0/24", "192.168.1.1", false, "eth0");
  }
}
BENCHMARK(BM_NetlinkIpCmd_RouteSetEntryDel);

static void BM_NetlinkIpCmd_LinkDestroy_NameLength(benchmark::State &state) {
  // 测试不同接口名称长度的影响

  auto mock_shell = std::make_unique<MockShell>();
  mock_shell->setBehavior();

  ohno::net::NetlinkIpCmd netlink(std::move(mock_shell));

  // 生成不同长度的接口名称
  std::string interface_name(state.range(0), 'a');

  for (auto _ : state) {
    netlink.linkDestory(interface_name);
  }

  state.SetComplexityN(state.range(0));
}
BENCHMARK(BM_NetlinkIpCmd_LinkDestroy_NameLength)
    ->Range(8, 8 << 10) // 测试从8字节到8KB的接口名称
    ->Complexity(benchmark::oN);

static void BM_NetlinkIpCmd_LinkToNetns_NamespaceLength(benchmark::State &state) {
  // 测试不同网络空间名称长度的影响

  auto mock_shell = std::make_unique<MockShell>();
  mock_shell->setBehavior();

  ohno::net::NetlinkIpCmd netlink(std::move(mock_shell));

  // 生成不同长度的命名空间名称
  std::string netns_name(state.range(0), 'n');
  std::string interface_name = "eth0";

  for (auto _ : state) {
    netlink.linkToNetns(interface_name, netns_name);
  }

  state.SetComplexityN(state.range(0));
}
BENCHMARK(BM_NetlinkIpCmd_LinkToNetns_NamespaceLength)
    ->Range(8, 8 << 10) // 测试从8字节到8KB的命名空间名称
    ->Complexity(benchmark::oN);

static void BM_NetlinkIpCmd_ErrorHandling(benchmark::State &state) {
  // 测试错误处理
  auto mock_shell = std::make_unique<MockShell>();

  // 设置命令执行失败的行为
  mock_shell->setCommandBehavior("ip link del dev test_interface",
                                 "",                                       // 空输出
                                 1,                                        // 非零返回码表示失败
                                 "Cannot find device \"test_interface\""); // 错误消息
  mock_shell->setBehavior();

  ohno::net::NetlinkIpCmd netlink(std::move(mock_shell));

  for (auto _ : state) {
    bool result = netlink.linkDestory("test_interface");
    benchmark::DoNotOptimize(result);
  }
}
BENCHMARK(BM_NetlinkIpCmd_ErrorHandling);

BENCHMARK_MAIN();
