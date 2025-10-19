// clang-format off
#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "spdlog/fmt/fmt.h"
#include "src/net/nic.h"
// clang-format on

using namespace ohno::net;

class MockNetlink : public NetlinkIf {
public:
  MOCK_METHOD(bool, linkDestory, (std::string_view name, std::string_view netns), (override));
  MOCK_METHOD(bool, linkExist, (std::string_view name, std::string_view netns), (override));
  MOCK_METHOD(bool, linkSetStatus,
              (std::string_view name, LinkStatus status, std::string_view netns), (override));
  MOCK_METHOD(bool, linkIsInNetns, (std::string_view name, std::string_view netns), (override));
  MOCK_METHOD(bool, linkToNetns, (std::string_view name, std::string_view netns), (override));
  MOCK_METHOD(bool, linkRename,
              (std::string_view name, std::string_view new_name, std::string_view netns),
              (override));
  MOCK_METHOD(bool, vethCreate, (std::string_view name1, std::string_view name2), (override));
  MOCK_METHOD(bool, bridgeCreate, (std::string_view name), (override));
  MOCK_METHOD(bool, vxlanCreate,
              (std::string_view name, std::string_view underlay_addr,
               std::string_view underlay_dev),
              (override));
  MOCK_METHOD(bool, vrfCreate, (std::string_view name, uint32_t table), (override));
  MOCK_METHOD(bool, bridgeSetStatus,
              (std::string_view name, bool master, std::string_view bridge, BridgeAddrGenMode mode,
               std::string_view netns),
              (override));
  MOCK_METHOD(bool, vxlanSetSlave,
              (std::string_view name, bool neigh_suppress, bool learning, std::string_view netns),
              (override));
  MOCK_METHOD(bool, addressIsExist,
              (std::string_view name, std::string_view addr, std::string_view netns), (override));
  MOCK_METHOD(bool, addressSetEntry,
              (std::string_view name, std::string_view addr, bool add, std::string_view netns),
              (override));
  MOCK_METHOD(bool, routeIsExist,
              (std::string_view dst, std::string_view via, std::string_view dev,
               std::string_view netns),
              (const, override));
  MOCK_METHOD(bool, routeSetEntry,
              (std::string_view dst, std::string_view via, bool add, std::string_view dev,
               std::string_view netns, RouteNHFlags nhflags),
              (const, override));
  MOCK_METHOD(bool, neighIsExist,
              (std::string_view addr, std::string_view dev, std::string_view netns),
              (const, override));
  MOCK_METHOD(bool, neighSetEntry,
              (std::string_view addr, std::string_view mac, bool add, std::string_view dev,
               std::string_view netns),
              (const, override));
  MOCK_METHOD(bool, fdbIsExist,
              (std::string_view mac, std::string_view underlay_addr, std::string_view dev,
               std::string_view netns),
              (const, override));
  MOCK_METHOD(bool, fdbSetEntry,
              (std::string_view mac, std::string_view underlay_addr, std::string_view dev, bool add,
               std::string_view netns),
              (const, override));
};

class NicTest : public ::testing::Test {
protected:
  void SetUp() override {
    mock_shell_ = std::make_shared<MockNetlink>();
    nic_ = std::make_unique<Nic>();
    nic_->setup(mock_shell_);
  }

  void TearDown() override {}

  std::unique_ptr<Nic> nic_;
  std::shared_ptr<MockNetlink> mock_shell_;
};

TEST_F(NicTest, AddNetns) {
  EXPECT_CALL(*mock_shell_, linkIsInNetns(testing::_, testing::_))
      .WillRepeatedly(testing::Return(true));

  std::string netns1{"test"};
  nic_->setNetns(netns1);
  EXPECT_STREQ(nic_->getNetns().c_str(), netns1.c_str());

  std::string netns2{fmt::format("{}/{}", PATH_NAMESPACE, netns1)};
  nic_->setNetns(netns2);
  EXPECT_STREQ(nic_->getNetns().c_str(), netns1.c_str());
}
