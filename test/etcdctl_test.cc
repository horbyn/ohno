// clang-format off
#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "src/etcd/etcd_client_shell.h"
#include "src/util/env_std.h"
#include "src/util/shell_sync.h"
// clang-format on

using namespace ohno;

// 这里测试会直接操作 ETCD 集群，可以在测试之后检查以下 key 避免污染（测试有删除逻辑但以防万一）
constexpr std::string_view KEY{"foo"};

class EtcdClientTest : public ::testing::Test {
protected:
  void SetUp() override {
    auto shell = std::make_unique<util::ShellSync>();
    auto env = std::make_unique<util::EnvStd>();
    etcd_client_ =
        std::make_unique<etcd::EtcdClientShell>(etcd::EtcdData{}, std::move(shell), std::move(env));
  }

  void TearDown() override { etcd_client_->del(KEY); }

  std::unique_ptr<etcd::EtcdClientIf> etcd_client_;
};

TEST_F(EtcdClientTest, PutOperation) {
  bool result = etcd_client_->put(KEY, "bar");
  EXPECT_TRUE(result);

  std::string value{};
  result = etcd_client_->get(KEY, value);
  EXPECT_TRUE(result);
  EXPECT_STREQ(value.c_str(), "bar");
}

TEST_F(EtcdClientTest, AppendOperation) {
  bool result = etcd_client_->put(KEY, "bar");
  EXPECT_TRUE(result);
  result = etcd_client_->append(KEY, "cat");
  EXPECT_TRUE(result);

  std::string value{};
  result = etcd_client_->get(KEY, value);
  EXPECT_TRUE(result);
  EXPECT_STREQ(value.c_str(), "bar,cat");
}

TEST_F(EtcdClientTest, DelOperation1) {
  bool result = etcd_client_->del(KEY);
  EXPECT_TRUE(result);

  std::string value{};
  result = etcd_client_->get(KEY, value);
  EXPECT_TRUE(result);
  EXPECT_TRUE(value.empty());
}

TEST_F(EtcdClientTest, DelOperation2) {
  bool result = etcd_client_->put(KEY, "bar,cat");
  EXPECT_TRUE(result);

  result = etcd_client_->del(KEY, "bar");
  EXPECT_TRUE(result);

  std::string value{};
  result = etcd_client_->get(KEY, value);
  EXPECT_TRUE(result);
  EXPECT_STREQ(value.c_str(), "cat");
}

TEST_F(EtcdClientTest, ListOperation) {
  bool result = etcd_client_->put(KEY, "cat");
  EXPECT_TRUE(result);
  result = etcd_client_->append(KEY, "bar");
  EXPECT_TRUE(result);

  std::vector<std::string> results{};
  result = etcd_client_->list(KEY, results);
  EXPECT_TRUE(result);
  EXPECT_THAT(results, testing::ElementsAre("cat", "bar"));
}
