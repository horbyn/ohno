// clang-format off
#include <memory>
#include <cstring>
#include "gtest/gtest.h"
#include "src/util/env_std.h"
// clang-format on

using namespace ohno::util;

TEST(EnvStdTest, CommonTest) {
  EnvStd es{};
  std::string env_name{"OHNO_ENV_TEST"}, env_value_backup{};
  if (es.exist(env_name)) {
    env_value_backup = es.get(env_name);
    es.unset(env_name);
  }

  // condition 0: 存在环境变量，获取它的值
  {
    std::unique_ptr<char[]> buffer{new char[16]{}};
    std::strcpy(buffer.get(), "TEST");
    if (!es.exist(env_name)) {
      EXPECT_TRUE(es.set(env_name, buffer.get()));
    }
    std::string value = es.get(env_name);
    EXPECT_STREQ(value.c_str(), buffer.get());
  }

  // condition 1: 获取不存在环境变量
  {
    if (es.exist(env_name)) {
      es.unset(env_name);
    }
    EXPECT_TRUE(es.get(env_name).empty());
  }

  if (!env_value_backup.empty()) {
    es.set(env_name, env_value_backup);
  }
}
