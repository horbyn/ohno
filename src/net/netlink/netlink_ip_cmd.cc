// clang-format off
#include "netlink_ip_cmd.h"
#include "spdlog/fmt/fmt.h"
#include "src/common/assert.h"
#include "src/common/enum_name.hpp"
// clang-format on

namespace ohno {
namespace net {

NetlinkIpCmd::NetlinkIpCmd(std::unique_ptr<util::ShellIf> shell) : shell_{std::move(shell)} {}

/**
 * @brief 删除网络接口
 *
 * @param name 网络接口名称
 * @param netns 网络空间名称（可以为空）
 * @return true 删除成功
 * @return false 删除失败
 */
auto NetlinkIpCmd::linkDestory(std::string_view name, std::string_view netns) -> bool {
  OHNO_ASSERT(!name.empty());
  std::string cmd = addNetns(fmt::format("ip link del dev {}", name), netns);
  return executeCommand(cmd, "Failed to destory link {}", name);
}

/**
 * @brief 判断网卡是否存在
 *
 * @param name 网卡名称
 * @param netns 网络空间名称（可以为空）
 * @return true 存在
 * @return false 不存在
 */
auto NetlinkIpCmd::linkExist(std::string_view name, std::string_view netns) -> bool {
  OHNO_ASSERT(!name.empty());
  std::string cmd = addNetns(fmt::format("ip link show {}", name), netns);
  return executeCommand(cmd, "Link {} is not exist", name);
}

/**
 * @brief 设置网络接口开启或关闭
 *
 * @param name 网络接口名称
 * @param status 网络接口状态
 * @param netns 网络空间名称（可以为空）
 * @return true 设置成功
 * @return false 设置失败
 */
auto NetlinkIpCmd::linkSetStatus(std::string_view name, LinkStatus status, std::string_view netns)
    -> bool {
  OHNO_ASSERT(!name.empty());
  std::string action = status == LinkStatus::UP ? " up" : " down";
  std::string cmd = addNetns(fmt::format("ip link set dev {} {}", name, action), netns);
  return executeCommand(cmd, "Failed to set status {}", name);
}

/**
 * @brief 判断 namespace 中是否存在网卡
 *
 * @param name 网卡名称
 * @param netns 网络空间名称
 * @return true 存在
 * @return false 不存在
 */
auto NetlinkIpCmd::linkIsInNetns(std::string_view name, std::string_view netns) -> bool {
  OHNO_ASSERT(!name.empty());
  OHNO_ASSERT(!netns.empty());
  std::string cmd = addNetns(fmt::format("ip link show dev {}", name), netns);
  std::string output{};
  if (shell_->execute(cmd, output)) {
    return output.find(name) != std::string::npos;
  }
  return false;
}

/**
 * @brief 将网络接口移动到指定网络空间
 *
 * @param name 网络接口名称
 * @param netns 网络空间名称
 * @return true 移动成功
 * @return false 移动失败
 */
auto NetlinkIpCmd::linkToNetns(std::string_view name, std::string_view netns) -> bool {
  OHNO_ASSERT(!name.empty());
  OHNO_ASSERT(!netns.empty());
  std::string cmd = fmt::format("ip link set dev {} netns {}", name, netns);
  return executeCommand(cmd, "Failed to move {} to namespace {}", name, netns);
}

/**
 * @brief 网络接口重命名
 *
 * @param name 网络接口
 * @param new_name 新名称
 * @param netns 网络空间名称（可以为空）
 * @return true 重命名成功
 * @return false 重命名失败
 */
auto NetlinkIpCmd::linkRename(std::string_view name, std::string_view new_name,
                              std::string_view netns) -> bool {
  OHNO_ASSERT(!name.empty());
  OHNO_ASSERT(!new_name.empty());
  std::string cmd = addNetns(fmt::format("ip link set dev {} name {}", name, new_name), netns);
  return executeCommand(cmd, "Failed to rename {} to {}", name, new_name);
}

/**
 * @brief 创建 veth pair
 *
 * @param name1 veth pair 一端名称
 * @param name2 veth pair 另一端名称
 * @return true 创建成功
 * @return false 创建失败
 */
auto NetlinkIpCmd::vethCreate(std::string_view name1, std::string_view name2) -> bool {
  OHNO_ASSERT(!name1.empty());
  OHNO_ASSERT(!name2.empty());
  std::string cmd = fmt::format("ip link add dev {} type veth peer {}", name1, name2);
  return executeCommand(cmd, "Failed to create veth({},{})", name1, name2);
}

/**
 * @brief 创建 bridge
 *
 * @param name bridge 名称
 * @return true 创建成功
 * @return false 创建失败
 */
auto NetlinkIpCmd::bridgeCreate(std::string_view name) -> bool {
  OHNO_ASSERT(!name.empty());
  std::string cmd = fmt::format("ip link add dev {} type bridge", name);
  return executeCommand(cmd, "Failed to create bridge {}", name);
}

/**
 * @brief 创建 vxlan
 *
 * @param name 网卡名称
 * @param underlay_addr 底层网卡地址
 * @param underlay_dev 底层网卡
 * @return true 成功
 * @return false 失败
 */
auto NetlinkIpCmd::vxlanCreate(std::string_view name, std::string_view underlay_addr,
                               std::string_view underlay_dev) -> bool {
  OHNO_ASSERT(!name.empty());
  OHNO_ASSERT(!underlay_addr.empty());
  OHNO_ASSERT(!underlay_dev.empty());
  std::string cmd =
      fmt::format("ip link add dev {} type vxlan id {} dstport {} local {} dev {} nolearning proxy",
                  name, VXLAN_VNI, PORT_VXLAN, underlay_addr, underlay_dev);
  return executeCommand(cmd, "Failed to create vxlan {}, local {}, dev {}", name, underlay_addr,
                        underlay_dev);
}

/**
 * @brief 创建 vrf
 *
 * @param name 网卡名称
 * @param table 要绑定的路由表
 * @return true 成功
 * @return false 失败
 */
auto NetlinkIpCmd::vrfCreate(std::string_view name, uint32_t table) -> bool {
  OHNO_ASSERT(!name.empty());
  std::string cmd = fmt::format("ip link add name {} type vrf table {}", name, table);
  return executeCommand(cmd, "Failed to create vrf {}", name);
}

/**
 * @brief 设置 Linux bridge 接口
 *
 * @param name 需要处理的网络接口
 * @param master 插入 bridge（true），从 bridge 拔出（false）
 * @param bridge Linux bridge 接口
 * @param mode 地址生成模式
 * @param netns 网络空间名称（可以为空）
 * @return true 设置成功
 * @return false 设置失败
 */
auto NetlinkIpCmd::bridgeSetStatus(std::string_view name, bool master, std::string_view bridge,
                                   BridgeAddrGenMode mode, std::string_view netns) -> bool {
  OHNO_ASSERT(!name.empty());
  OHNO_ASSERT(!bridge.empty());
  std::string action = master ? "master" : "nomaster";
  std::string modo_str = mode == BridgeAddrGenMode::reserved
                             ? std::string{}
                             : fmt::format("addrgenmode {}", enumName(mode));
  std::string cmd =
      addNetns(fmt::format("ip link set dev {} {} {} {}", name, action, bridge, modo_str), netns);
  return executeCommand(cmd, "Failed to set {} of link({}) to bridge({})", action, name, bridge);
}

/**
 * @brief 设置 VTEP 桥接从接口属性
 *
 * @param name VTEP 设备名称
 * @param neigh_suppress 启用（true）/ 禁用（false）邻居表抑制
 * @param learning 启用（true）/ 禁用（false）地址学习
 * @param netns 网络空间名称（可以为空）
 * @return true 成功
 * @return false 失败
 */
auto NetlinkIpCmd::vxlanSetSlave(std::string_view name, bool neigh_suppress, bool learning,
                                 std::string_view netns) -> bool {
  OHNO_ASSERT(!name.empty());
  std::string arp_suppress = neigh_suppress ? "on" : "off";
  std::string learn = learning ? "on" : "off";
  std::string cmd =
      addNetns(fmt::format("ip link set {} type bridge_slave neigh_suppress {} learning {}", name,
                           arp_suppress, learn),
               netns);
  return executeCommand(cmd, "Failed to set vxlan slave {}", name);
}

/**
 * @brief 判断 IP 地址是否存在
 *
 * @param name 网卡名称
 * @param addr IP 地址
 * @param netns 网络空间（可以为空）
 * @return true 地址存在
 * @return false 地址不存在
 */
auto NetlinkIpCmd::addressIsExist(std::string_view name, std::string_view addr,
                                  std::string_view netns) -> bool {
  OHNO_ASSERT(!name.empty());
  OHNO_ASSERT(!addr.empty());
  std::string cmd = addNetns(fmt::format("ip addr show dev {}", name), netns);
  std::string output{};
  if (shell_->execute(cmd, output)) {
    return output.find(addr) != std::string::npos;
  }
  OHNO_LOG(warn, "Failed to execute command: {}", cmd);
  return false;
}

/**
 * @brief 设置 IP 地址
 *
 * @param name 网络接口名称
 * @param addr 地址
 * @param add 增加地址（true），删除地址（false）
 * @param netns 网络空间名称（可以为空）
 * @return true 设置成功
 * @return false 设置失败
 */
auto NetlinkIpCmd::addressSetEntry(std::string_view name, std::string_view addr, bool add,
                                   std::string_view netns) -> bool {
  OHNO_ASSERT(!name.empty());
  OHNO_ASSERT(!addr.empty());
  std::string action = add ? "add" : "del";
  std::string cmd = addNetns(fmt::format("ip addr {} {} dev {}", action, addr, name), netns);
  return executeCommand(cmd, "Failed to {} address({},{})", action, name, addr);
}

/**
 * @brief 检查路由是否存在
 *
 * @param dst 目的网段（可以为空）
 * @param via 下一跳地址
 * @param dev 经过设备（可以为空）
 * @param netns 网络空间（可以为空）
 * @return true 路由存在
 * @return false 路由不存在
 */
auto NetlinkIpCmd::routeIsExist(std::string_view dst, std::string_view via, std::string_view dev,
                                std::string_view netns) const -> bool {
  OHNO_ASSERT(!via.empty());
  std::string dest = dst.empty() ? "default" : std::string{dst};
  std::string device = dev.empty() ? std::string{} : fmt::format("dev {}", dev);
  std::string cmd = addNetns(fmt::format("ip route show {} via {} {}", dest, via, device), netns);
  std::string output{};
  std::string error{};
  if (shell_->execute(cmd, output, error) == 0) {
    return !output.empty(); // 输出为空则路由不存在；存在输出则路由存在
  }
  OHNO_LOG(warn, "Failed to execute command: {}", cmd);
  return false;
}

/**
 * @brief 设置路由表条目
 *
 * @param dst 目的网络（可以为空，为空则设置为 default）
 * @param via 下一跳地址
 * @param add 增加路由（true），删除路由（false）
 * @param dev 网络接口名称（可以为空）
 * @param netns 网络空间名称（可以为空）
 * @param nhflags NH 标识（可以为空）
 * @return true 设置成功
 * @return false 设置失败
 */
auto NetlinkIpCmd::routeSetEntry(std::string_view dst, std::string_view via, bool add,
                                 std::string_view dev, std::string_view netns,
                                 RouteNHFlags nhflags) const -> bool {
  OHNO_ASSERT(!via.empty());
  std::string action = add ? "add" : "del";
  std::string dest = dst.empty() ? "default" : std::string{dst};
  std::string device = dev.empty() ? std::string{} : fmt::format("dev {}", dev);
  std::string nhflags_str =
      nhflags == RouteNHFlags::NONE ? std::string{} : std::string{enumName(nhflags)};
  std::string cmd = addNetns(
      fmt::format("ip route {} {} via {} {} {}", action, dest, via, device, nhflags_str), netns);
  return executeCommand(cmd, "Failed to {} route({} via {}) {}", action, dest, via, nhflags_str);
}

/**
 * @brief 检查 ARP 缓存是否存在
 *
 * @param addr 三层地址
 * @param dev 经过设备（可以为空）
 * @param netns 网络空间（可以为空）
 * @return true 存在
 * @return false 不存在
 */
auto NetlinkIpCmd::neighIsExist(std::string_view addr, std::string_view dev,
                                std::string_view netns) const -> bool {
  OHNO_ASSERT(!addr.empty());
  std::string device = dev.empty() ? std::string{} : fmt::format("dev {}", dev);
  std::string cmd = addNetns(fmt::format("ip neigh show {} {}", addr, device), netns);
  std::string output{};
  std::string error{};
  if (shell_->execute(cmd, output, error) == 0) {
    return !output.empty(); // 输出为空则不存在；存在输出则存在
  }
  OHNO_LOG(warn, "Failed to execute command: {}", cmd);
  return false;
}

/**
 * @brief 设置 ARP 缓存条目
 *
 * @param addr 三层地址
 * @param mac MAC 地址
 * @param add 增加路由（true），删除路由（false）
 * @param dev 网络接口名称（可以为空）
 * @param netns 网络空间名称（可以为空）
 * @return true 设置成功
 * @return false 设置失败
 */
auto NetlinkIpCmd::neighSetEntry(std::string_view addr, std::string_view mac, bool add,
                                 std::string_view dev, std::string_view netns) const -> bool {
  OHNO_ASSERT(!addr.empty());
  OHNO_ASSERT(!mac.empty());
  std::string action = add ? "add" : "del";
  std::string device = dev.empty() ? std::string{} : fmt::format("dev {}", dev);
  std::string cmd =
      addNetns(fmt::format("ip neigh {} {} lladdr {} {}", action, addr, mac, device), netns);
  return executeCommand(cmd, "Failed to {} ARP cache({} lladr {})", action, addr, mac);
}

/**
 * @brief 检查 FDB 表项是否存在
 *
 * @param mac MAC 地址
 * @param underlay_addr 底层地址
 * @param dev 所在设备
 * @param netns 网络空间（可以为空）
 * @return true 存在
 * @return false 不存在
 */
auto NetlinkIpCmd::fdbIsExist(std::string_view mac, std::string_view underlay_addr,
                              std::string_view dev, std::string_view netns) const -> bool {
  OHNO_ASSERT(!mac.empty());
  OHNO_ASSERT(!underlay_addr.empty());
  OHNO_ASSERT(!dev.empty());
  std::string cmd = addNetns(
      fmt::format("bridge fdb show dev {} | grep {} | grep {}", dev, mac, underlay_addr), netns);
  std::string output{};
  std::string error{};
  if (shell_->execute(cmd, output, error) == 0) {
    return !output.empty(); // 输出为空则不存在；存在输出则存在
  }
  OHNO_LOG(warn, "Failed to execute command: {}", cmd);
  return false;
}

/**
 * @brief 设置 FDB 条目
 *
 * @param mac MAC 地址
 * @param underlay_addr 底层地址
 * @param dev 网络接口名称
 * @param add 增加路由（true），删除路由（false）
 * @param netns 网络空间名称（可以为空）
 * @return true 设置成功
 * @return false 设置失败
 */
auto NetlinkIpCmd::fdbSetEntry(std::string_view mac, std::string_view underlay_addr,
                               std::string_view dev, bool add, std::string_view netns) const
    -> bool {
  OHNO_ASSERT(!mac.empty());
  OHNO_ASSERT(!underlay_addr.empty());
  OHNO_ASSERT(!dev.empty());
  std::string action = add ? "add" : "del";
  std::string cmd = addNetns(
      fmt::format("bridge fdb {} {} dev {} dst {}", action, mac, dev, underlay_addr), netns);
  return executeCommand(cmd, "Failed to {} FDB entry({} dev {} dst {})", action, mac, dev,
                        underlay_addr);
}

/**
 * @brief 将 ip 命令加上 Linux namespace 前缀
 *
 * @param command ip 命令
 * @param netns Linux namespace 名称（可以为空）
 * @return std::string 如果 Linux namespace 为空则返回原命令，否则原命令加上网络空间前缀
 */
auto NetlinkIpCmd::addNetns(std::string_view command, std::string_view netns) -> std::string {
  OHNO_ASSERT(!command.empty());
  if (netns.empty()) {
    return std::string{command};
  }
  return fmt::format("ip netns exec {} {}", netns, command);
}

} // namespace net
} // namespace ohno
