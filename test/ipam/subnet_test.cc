// clang-format off
#include "gtest/gtest.h"
#include "src/net/subnet.h"
// clang-format on

using namespace ohno::net;

// 测试 IPv4 CIDR 初始化
TEST(SubnetTest, Init) {
  Subnet subnet;

  // condition 1: IPv4
  {
    EXPECT_NO_THROW(subnet.init("192.168.1.0/24"));
    EXPECT_EQ(subnet.getSubnet(), "192.168.1.0/24");
    EXPECT_EQ(subnet.getPrefix(), 24);
  }

  // condition 2: IPv6
  {
    EXPECT_NO_THROW(subnet.init("2001:db8::/32"));
    EXPECT_ANY_THROW(subnet.getSubnet());
    EXPECT_ANY_THROW(subnet.getPrefix());
  }

  // condition 3: 无效
  {
    EXPECT_ANY_THROW(subnet.init("invalid_cidr"));
  }
}

// 测试生成子网 CIDR
TEST(SubnetTest, GenerateCIDR) {
  Subnet subnet;
  subnet.init("192.168.1.0/24");

  EXPECT_EQ(subnet.generateCidr(26, 0), "192.168.1.0/26");
  EXPECT_EQ(subnet.generateCidr(26, 1), "192.168.1.64/26");
  EXPECT_EQ(subnet.generateCidr(26, 2), "192.168.1.128/26");
  EXPECT_EQ(subnet.generateCidr(26, 3), "192.168.1.192/26");
  EXPECT_ANY_THROW(subnet.generateCidr(26, 4));
}

// 测试生成 IP 地址
TEST(SubnetTest, GenerateIP) {
  Subnet subnet;
  subnet.init("192.168.1.0/24");

  EXPECT_EQ(subnet.generateIp(1), "192.168.1.1/24");
  EXPECT_EQ(subnet.generateIp(10), "192.168.1.10/24");
  EXPECT_EQ(subnet.generateIp(254), "192.168.1.254/24");
}

// 测试获取最大主机数
TEST(SubnetTest, GetMaxHosts) {
  Subnet subnet;
  subnet.init("192.168.1.0/24");

  EXPECT_EQ(subnet.getMaxHosts(), 256);
}

// 测试获取最大子网数
TEST(SubnetTest, GetMaxSubnetsFromCidr) {
  Subnet subnet;
  subnet.init("192.168.1.0/24");

  EXPECT_EQ(subnet.getMaxSubnetsFromCidr(26), 4);
  EXPECT_EQ(subnet.getMaxSubnetsFromCidr(28), 16);
  EXPECT_ANY_THROW(subnet.getMaxSubnetsFromCidr(22));
}
