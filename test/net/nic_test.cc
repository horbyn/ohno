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
  MOCK_METHOD(bool, bridgeSetStatus,
              (std::string_view name, bool master, std::string_view bridge, std::string_view netns),
              (override));
  MOCK_METHOD(bool, addressIsExist,
              (std::string_view name, std::string_view addr, std::string_view netns), (override));
  MOCK_METHOD(bool, addressSetEntry,
              (std::string_view name, std::string_view addr, bool add, std::string_view netns),
              (override));
  MOCK_METHOD(bool, routeIsExist,
              (std::string_view dst, std::string_view via, std::string_view dev,
               std::string_view netns),
              (override));
  MOCK_METHOD(bool, routeSetEntry,
              (std::string_view dst, std::string_view via, bool add, std::string_view dev,
               std::string_view netns),
              (override));
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
  std::string netns1{"test"};
  nic_->setNetns(netns1);
  EXPECT_STREQ(nic_->getNetns().c_str(), netns1.c_str());

  std::string netns2{fmt::format("{}/{}", PATH_NAMESPACE, netns1)};
  nic_->setNetns(netns2);
  EXPECT_STREQ(nic_->getNetns().c_str(), netns1.c_str());
}
