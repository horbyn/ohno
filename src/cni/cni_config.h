#pragma once

// clang-format off
#include <string>
#include <string_view>
#include <vector>
#include "nlohmann/json.hpp"
#include "src/log/logger.h"
// clang-format on

namespace ohno {
namespace cni {

constexpr std::string_view JKEY_CNI_CCI_SUBNET{"subnet"};
constexpr std::string_view JKEY_CNI_CCI_GATEWAY{"gateway"};
constexpr std::string_view JKEY_CNI_CC_VERSION{"cniVersion"};
constexpr std::string_view JKEY_CNI_CC_NAME{"name"};
constexpr std::string_view JKEY_CNI_CC_TYPE{"type"};
constexpr std::string_view JKEY_CNI_CC_BRIDGE{"bridge"};
constexpr std::string_view JKEY_CNI_CC_LOG{"log"};
constexpr std::string_view JKEY_CNI_CC_LOGLEVEL{"logLevel"};
constexpr std::string_view JKEY_CNI_CC_SUBNET_PREFIX{"subnetPrefix"};
constexpr std::string_view JKEY_CNI_CC_IPAM{"ipam"};

constexpr std::string_view DEFAULT_CONF_VERSION{"0.3.1"};
constexpr std::string_view DEFAULT_CONF_NETNAME{"mynet"};
constexpr std::string_view DEFAULT_CONF_PLUGINS_NAME{"ohno"};
constexpr std::string_view DEFAULT_CONF_PLUGINS_BRIDGE{"ohnobr"};
constexpr std::string_view DEFAULT_CONF_IPAM_SUBNET{"10.244.0.0/16"};
constexpr std::string_view DEFAULT_CONF_IPAM_GATEWAY{"10.244.0.1"};
constexpr int DEFAULT_CONF_PLUGINS_SUBNET_PREFIX{24};

class CniConfigIpam final {
public:
  friend void from_json(const nlohmann::json &json, CniConfigIpam &ipam);
  friend void to_json(nlohmann::json &json, const CniConfigIpam &ipam);

  std::string subnet_{std::string{DEFAULT_CONF_IPAM_SUBNET}};
  std::string gateway_{std::string{DEFAULT_CONF_IPAM_GATEWAY}};
};

class CniConfig final {
public:
  friend void from_json(const nlohmann::json &json, CniConfig &conf);
  friend void to_json(nlohmann::json &json, const CniConfig &conf);

  std::string cni_version_{std::string{DEFAULT_CONF_VERSION}};
  std::string name_{std::string{DEFAULT_CONF_NETNAME}};
  std::string type_{std::string{DEFAULT_CONF_PLUGINS_NAME}};
  std::string bridge_{std::string{DEFAULT_CONF_PLUGINS_BRIDGE}};
  std::string log_{log::LOGFILE_DEFAULT};                 // 没错，我非常需要日志
  log::Level loglevel_{log::Level::info};                 // 还有日志等级
  int subnet_prefix_{DEFAULT_CONF_PLUGINS_SUBNET_PREFIX}; // 每个节点上要划分的 Pod 子网由用户定义
  CniConfigIpam ipam_;
};

} // namespace cni
} // namespace ohno
