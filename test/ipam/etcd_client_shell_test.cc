// clang-format off
#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "src/etcd/etcd_client_shell.h"
#include "src/util/env_std.h"
// clang-format on

using namespace ohno::util;
using namespace ohno::etcd;

/*
 * TODO:
 * 实际上自己模拟 shell 命令输出来测试是发现不了问题的
 * 但是如果单元测试必须调用真实 shell 命令才能进行，那在 CICD 环境中就要考虑部署问题就更麻烦了
 * 这里我也不知道应该怎么模拟，暂时这样做了
 */
class MockShellSync : public ShellIf {
public:
  MOCK_METHOD(bool, execute, (std::string_view command, std::string &output), (const, override));
  MOCK_METHOD(int, execute, (std::string_view command, std::string &output, std::string &error),
              (const, override));
};

class EtcdClientShellTest : public ::testing::Test {
protected:
  void SetUp() override {
    auto mock_shell = std::make_unique<MockShellSync>();
    mock_shell_ = mock_shell.get();
    auto env = std::make_unique<EnvStd>();
    etcd_client_ =
        std::make_unique<EtcdClientShell>(EtcdData{}, std::move(mock_shell), std::move(env));
  }

  void TearDown() override {}

  std::unique_ptr<EtcdClientShell> etcd_client_;
  // 原接口是 std::unique_ptr 含义是接管 ownership，这里用裸指针保存一份期望
  MockShellSync *mock_shell_;
};

TEST_F(EtcdClientShellTest, PutOperation) {
  EXPECT_CALL(*mock_shell_, execute(testing::_, testing::_))
      .WillOnce(testing::DoAll(testing::SetArgReferee<1>(""), // 设置输出为空
                               testing::Return(true)          // 返回成功
                               ));

  bool result = etcd_client_->put("test-key", "test-value");
  EXPECT_TRUE(result);
}

TEST_F(EtcdClientShellTest, AppendOperation) {
  EXPECT_CALL(*mock_shell_, execute(testing::_, testing::_))
      .WillOnce(testing::DoAll(testing::SetArgReferee<1>("test-value"), // 设置输出
                               testing::Return(true)                    // 返回成功
                               ))
      .WillOnce(testing::DoAll(testing::SetArgReferee<1>(""), // 设置输出为空
                               testing::Return(true)          // 返回成功
                               ))
      .WillOnce(testing::DoAll(testing::SetArgReferee<1>("test-value,test-append"), // 设置输出
                               testing::Return(true)                                // 返回成功
                               ));

  bool result = etcd_client_->append("test-key", "test-append");
  EXPECT_TRUE(result);

  std::string value{};
  result = etcd_client_->get("test-key", value);
  EXPECT_TRUE(result);
  EXPECT_STREQ(value.c_str(), "test-value,test-append");
}

TEST_F(EtcdClientShellTest, GetOperation) {
  std::string value{};
  EXPECT_CALL(*mock_shell_, execute(testing::_, testing::_))
      .WillOnce(testing::DoAll(testing::SetArgReferee<1>("test-value"), // 设置输出
                               testing::Return(true)                    // 返回成功
                               ));

  bool result = etcd_client_->get("test-key", value);
  EXPECT_TRUE(result);
  EXPECT_EQ(value, "test-value");
}

TEST_F(EtcdClientShellTest, DelOperation1) {
  EXPECT_CALL(*mock_shell_, execute(testing::_, testing::_))
      .WillOnce(testing::DoAll(testing::SetArgReferee<1>(""), // 设置输出为空
                               testing::Return(true)          // 返回成功
                               ));

  bool result = etcd_client_->del("test-key");
  EXPECT_TRUE(result);
}

TEST_F(EtcdClientShellTest, DelOperation2) {
  EXPECT_CALL(*mock_shell_, execute(testing::_, testing::_))
      .WillOnce(testing::DoAll(testing::SetArgReferee<1>("test-value,test-append"), // 设置输出
                               testing::Return(true)                                // 返回成功
                               ))
      .WillOnce(testing::DoAll(testing::SetArgReferee<1>(""), // 设置输出为空
                               testing::Return(true)          // 返回成功
                               ))
      .WillOnce(testing::DoAll(testing::SetArgReferee<1>("test-append"), // 设置输出
                               testing::Return(true)                     // 返回成功
                               ));

  bool result = etcd_client_->del("test-key", "test-value");
  EXPECT_TRUE(result);

  std::string value{};
  result = etcd_client_->get("test-key", value);
  EXPECT_TRUE(result);
  EXPECT_STREQ(value.c_str(), "test-append");
}

TEST_F(EtcdClientShellTest, ListOperation) {
  std::vector<std::string> results;
  EXPECT_CALL(*mock_shell_, execute(testing::_, testing::_))
      .WillOnce(testing::DoAll(testing::SetArgReferee<1>("value1,value2,value3"), // 设置输出
                               testing::Return(true)                              // 返回成功
                               ));

  bool result = etcd_client_->list("test-key", results);
  EXPECT_TRUE(result);
  EXPECT_THAT(results, testing::ElementsAre("value1", "value2", "value3"));
}
