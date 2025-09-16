#pragma once

// clang-format off
#include "storage_if.h"
#include "src/etcd/etcd_client_if.h"
#include "src/log/logger.h"
// clang-format on

namespace ohno {
namespace cni {

constexpr std::string_view ETCD_KEY_PREFIX{"/ohno/node"};
constexpr char SEPARATOR{'-'};

class Storage : public StorageIf, public log::Loggable<log::Id::cni> {
public:
  auto init(std::unique_ptr<etcd::EtcdClientIf> etcd_client) -> bool;
  auto dump() const -> std::string override;
  auto addNetns(std::string_view node_name, std::string_view pod_name, std::string_view netns_name)
      -> bool override;
  auto delNetns(std::string_view node_name, std::string_view pod_name) -> bool override;
  auto getNetns(std::string_view node_name, std::string_view pod_name) -> std::string override;
  auto addPod(std::string_view node_name, std::string_view netns_name, std::string_view pod_name)
      -> bool override;
  auto delPod(std::string_view node_name, std::string_view netns_name) -> bool override;
  auto getPod(std::string_view node_name, std::string_view netns_name) -> std::string override;
  auto getAllPods(std::string_view node_name) -> std::vector<std::string> override;
  auto addNic(std::string_view node_name, std::string_view pod_name, std::string_view nic_name)
      -> bool override;
  auto delNic(std::string_view node_name, std::string_view pod_name, std::string_view nic_name)
      -> bool override;
  auto getAllNic(std::string_view node_name, std::string_view pod_name) const
      -> std::vector<std::string> override;
  auto addAddr(std::string_view node_name, std::string_view pod_name, std::string_view nic_name,
               std::unique_ptr<net::AddrIf> addr) -> bool override;
  auto delAddr(std::string_view node_name, std::string_view pod_name, std::string_view nic_name)
      -> bool override;
  auto getAllAddrs(std::string_view node_name, std::string_view pod_name,
                   std::string_view nic_name) const -> std::vector<std::string> override;
  auto addRoute(std::string_view node_name, std::string_view pod_name, std::string_view nic_name,
                std::unique_ptr<net::RouteIf> route) -> bool override;
  auto delRoute(std::string_view node_name, std::string_view pod_name, std::string_view nic_name)
      -> bool override;
  auto getAllRoutes(std::string_view node_name, std::string_view pod_name,
                    std::string_view nic_name) const
      -> std::vector<std::unique_ptr<net::RouteIf>> override;

private:
  static auto getNetnsKey(std::string_view node_name, std::string_view pod_name) -> std::string;
  static auto getSinglePodKey(std::string_view node_name, std::string_view netns_name)
      -> std::string;
  static auto getAllPodsKey(std::string_view node_name) -> std::string;
  static auto getNicKey(std::string_view node_name, std::string_view pod_name) -> std::string;
  static auto getAddrKey(std::string_view node_name, std::string_view pod_name,
                         std::string_view nic_name) -> std::string;
  static auto getRouteKey(std::string_view node_name, std::string_view pod_name,
                          std::string_view nic_name) -> std::string;
  static auto getRouteValue(std::string_view dest, std::string_view via, std::string_view dev)
      -> std::string;

  std::unique_ptr<etcd::EtcdClientIf> etcd_client_;
};

} // namespace cni
} // namespace ohno
