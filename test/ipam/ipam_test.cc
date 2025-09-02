// clang-format off
#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "src/ipam/etcd_client_shell.h"
#include "src/ipam/ipam.h"
// clang-format on

using namespace ohno::util;
using namespace ohno::ipam;

class MockEtcdClient : public EtcdClientIf {
public:
  MOCK_METHOD(bool, put, (std::string_view key, std::string_view value), (override));
  MOCK_METHOD(bool, append, (std::string_view key, std::string_view value), (override));
  MOCK_METHOD(bool, get, (std::string_view key, std::string &value), (override));
  MOCK_METHOD(bool, del, (std::string_view key), (override));
  MOCK_METHOD(bool, del, (std::string_view key, std::string_view value), (override));
  MOCK_METHOD(bool, list, (std::string_view key, std::vector<std::string> &results), (override));
};

class IpamTest : public ::testing::Test {
protected:
  void SetUp() override {
    ipam_ = std::make_unique<Ipam>();
    auto mock_etcd_client = std::make_unique<MockEtcdClient>();
    mock_etcd_client_ = mock_etcd_client.get();
    ipam_->init(std::move(mock_etcd_client));
  }

  void TearDown() override {}

  std::unique_ptr<Ipam> ipam_;
  MockEtcdClient *mock_etcd_client_;
};

TEST_F(IpamTest, AllocateSubnet) {
  std::string subnet{};

  EXPECT_CALL(*mock_etcd_client_, get(testing::_, testing::_))
      .WillOnce(testing::Return(false)); // 第一次获取子网返回不存在
  EXPECT_CALL(*mock_etcd_client_, list(testing::_, testing::_))
      .WillRepeatedly(testing::DoAll(testing::SetArgReferee<1>(std::vector<std::string>{}),
                                     testing::Return(true))); // 总是返回空子网列表
  EXPECT_CALL(*mock_etcd_client_, append(testing::_, testing::_))
      .WillOnce(testing::Return(true)); // 追加操作成功

  bool result = ipam_->allocateSubnet("test-node", "192.168.1.0/24", 26, subnet);
  EXPECT_TRUE(result);
  EXPECT_STREQ(subnet.c_str(), "192.168.1.0/26");
}

TEST_F(IpamTest, GetSubnet) {
  std::string subnet{};

  EXPECT_CALL(*mock_etcd_client_, get(testing::_, testing::_))
      .WillOnce(testing::DoAll(testing::SetArgReferee<1>("192.168.1.0/26"),
                               testing::Return(true))); // 返回一个子网

  bool result = ipam_->getSubnet("test-node", subnet);
  EXPECT_TRUE(result);
  EXPECT_STREQ(subnet.c_str(), "192.168.1.0/26");
}

TEST_F(IpamTest, ReleaseSubnet) {
  std::string subnet{};

  EXPECT_CALL(*mock_etcd_client_, del(testing::_)).WillOnce(testing::DoAll(testing::Return(true)));
  EXPECT_CALL(*mock_etcd_client_, del(testing::_, testing::_))
      .WillOnce(testing::DoAll(testing::Return(true)));

  bool result = ipam_->releaseSubnet("test-node", "192.168.1.0/26");
  EXPECT_TRUE(result);
}

TEST_F(IpamTest, AllocateIP) {
  std::string ip{};

  EXPECT_CALL(*mock_etcd_client_, get(testing::_, testing::_))
      .WillOnce(testing::DoAll(testing::SetArgReferee<1>("192.168.1.0/26"),
                               testing::Return(true))); // 返回一个子网
  EXPECT_CALL(*mock_etcd_client_, list(testing::_, testing::_))
      .WillRepeatedly(testing::DoAll(testing::SetArgReferee<1>(std::vector<std::string>{}),
                                     testing::Return(true))); // 总是返回空 IP 列表
  EXPECT_CALL(*mock_etcd_client_, append(testing::_, testing::_))
      .WillOnce(testing::Return(true)); // 追加操作成功

  bool result = ipam_->allocateIp("test-node", ip);
  EXPECT_TRUE(result);
  EXPECT_STREQ(ip.c_str(), "192.168.1.1");
}

TEST_F(IpamTest, GetAllIp) {
  std::vector<std::string> all_ip{};

  EXPECT_CALL(*mock_etcd_client_, list(testing::_, testing::_))
      .WillOnce(testing::DoAll(
          testing::SetArgReferee<1>(std::vector<std::string>{"192.168.1.1", "192.168.1.2"}),
          testing::Return(true))); // 返回一个子网

  bool result = ipam_->getAllIp("test-node", all_ip);
  EXPECT_TRUE(result);
  EXPECT_THAT(all_ip, testing::ElementsAre("192.168.1.1", "192.168.1.2"));
}

TEST_F(IpamTest, ReleaseIp) {
  EXPECT_CALL(*mock_etcd_client_, del(testing::_, testing::_))
      .WillOnce(testing::DoAll(testing::Return(true)));

  bool result = ipam_->releaseIp("test-node", "192.168.1.1");
  EXPECT_TRUE(result);
}
