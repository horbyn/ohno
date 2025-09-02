// clang-format off
#include "gtest/gtest.h"
#include "src/util/shell_sync.h"
// clang-format on

using namespace ohno::util;

TEST(ShellSynTest, CommonTest) {
  ShellSync ss{};

  // condition 0: 输出 stdout
  {
    std::string ping{"ping -c5 127.0.0.1"}, output{}, error{};
    EXPECT_EQ(ss.execute(ping, output, error), 0);
    EXPECT_TRUE(error.empty());
  }

  // condition 1: 输出 stderr
  {
    std::string ls{"ls /invalid_dir/"}, output{}, error{};
    EXPECT_TRUE(ss.execute(ls, output, error) != 0);
    EXPECT_FALSE(error.empty());
  }

  // condition 2: 空命令
  {
    std::string empty{""}, output{}, error{};
    EXPECT_EQ(ss.execute(empty, output, error), -1);
  }

  // condition 3: 非法命令
  {
    std::string invalid{"invalid"}, output{}, error{};
    EXPECT_TRUE(ss.execute(invalid, output, error) == -1);
  }
}
