// clang-format off
#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "src/etcd/etcd_client_shell.h"
#include "src/ipam/ipam.h"
// clang-format on

using namespace ohno::etcd;
using namespace ohno::ipam;
using namespace ohno::util;

class MockEtcdClient : public EtcdClientIf {
public:
  MOCK_METHOD(bool, test, (), (const, override));
  MOCK_METHOD(bool, put, (std::string_view key, std::string_view value), (const, override));
  MOCK_METHOD(bool, append, (std::string_view key, std::string_view value), (const, override));
  MOCK_METHOD(bool, get, (std::string_view key, std::string &value), (const, override));
  MOCK_METHOD(bool, get,
              (std::string_view key, (std::unordered_map<std::string, std::string> & value)),
              (const, override));
  MOCK_METHOD(bool, del, (std::string_view key), (const, override));
  MOCK_METHOD(bool, del, (std::string_view key, std::string_view value), (const, override));
  MOCK_METHOD(bool, list, (std::string_view key, std::vector<std::string> &results),
              (const, override));
  MOCK_METHOD(std::string, dump, (std::string_view key), (const, override));
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

  EXPECT_CALL(*mock_etcd_client_, get(testing::_, testing::Matcher<std::string &>(testing::_)))
      .WillOnce(testing::DoAll(testing::SetArgReferee<1>("192.168.1.0/26"),
                               testing::Return(true))); // 返回一个子网
  EXPECT_CALL(*mock_etcd_client_, list(testing::_, testing::_))
      .WillRepeatedly(testing::DoAll(testing::SetArgReferee<1>(std::vector<std::string>{}),
                                     testing::Return(true))); // 总是返回空 IP 列表
  EXPECT_CALL(*mock_etcd_client_, append(testing::_, testing::_))
      .WillOnce(testing::Return(true)); // 追加操作成功

  bool result = ipam_->allocateIp("test-node", ip);
  EXPECT_TRUE(result);
  EXPECT_STREQ(ip.c_str(), "192.168.1.1/26");
}

TEST_F(IpamTest, ReleaseIp) {
  EXPECT_CALL(*mock_etcd_client_, del(testing::_, testing::_))
      .WillOnce(testing::DoAll(testing::Return(true)));

  bool result = ipam_->releaseIp("test-node", "192.168.1.1");
  EXPECT_TRUE(result);
}
