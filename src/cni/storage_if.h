#pragma once

// clang-format off
#include <memory>
#include <string_view>
#include <string>
#include <vector>
#include "src/net/addr_if.h"
#include "src/net/route_if.h"
// clang-format on

namespace ohno {
namespace cni {

class StorageIf {
public:
  virtual ~StorageIf() = default;
  virtual auto dump() const -> std::string = 0;
  virtual auto addNetns(std::string_view node_name, std::string_view pod_name,
                        std::string_view netns_name) -> bool = 0;
  virtual auto delNetns(std::string_view node_name, std::string_view pod_name) -> bool = 0;
  virtual auto getNetns(std::string_view node_name, std::string_view pod_name) const
      -> std::string = 0;
  virtual auto addPod(std::string_view node_name, std::string_view netns_name,
                      std::string_view pod_name) -> bool = 0;
  virtual auto delPod(std::string_view node_name, std::string_view netns_name) -> bool = 0;
  virtual auto getPod(std::string_view node_name, std::string_view netns_name) -> std::string = 0;
  virtual auto getAllPods(std::string_view node_name) const -> std::vector<std::string> = 0;
  virtual auto addNic(std::string_view node_name, std::string_view pod_name,
                      std::string_view nic_name) -> bool = 0;
  virtual auto delNic(std::string_view node_name, std::string_view pod_name,
                      std::string_view nic_name) -> bool = 0;
  virtual auto getAllNic(std::string_view node_name, std::string_view pod_name) const
      -> std::vector<std::string> = 0;
  virtual auto addAddr(std::string_view node_name, std::string_view pod_name,
                       std::string_view nic_name, std::unique_ptr<net::AddrIf> addr) -> bool = 0;
  virtual auto delAddr(std::string_view node_name, std::string_view pod_name,
                       std::string_view nic_name) -> bool = 0;
  virtual auto getAllAddrs(std::string_view node_name, std::string_view pod_name,
                           std::string_view nic_name) const -> std::vector<std::string> = 0;
  virtual auto addRoute(std::string_view node_name, std::string_view pod_name,
                        std::string_view nic_name, std::unique_ptr<net::RouteIf> route) -> bool = 0;
  virtual auto delRoute(std::string_view node_name, std::string_view pod_name,
                        std::string_view nic_name) -> bool = 0;
  virtual auto getAllRoutes(std::string_view node_name, std::string_view pod_name,
                            std::string_view nic_name) const
      -> std::vector<std::unique_ptr<net::RouteIf>> = 0;
};

} // namespace cni
} // namespace ohno
