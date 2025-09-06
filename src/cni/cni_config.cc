// clang-format off
#include "cni_config.h"
// clang-format on

namespace ohno {
namespace cni {

void from_json(const nlohmann::json &json, CniConfigPluginsIpam &ipam) {
  if (json.contains(JKEY_CNI_CCPI_TYPE)) {
    ipam.type_ = json.at(JKEY_CNI_CCPI_TYPE).get<std::string>();
  }
  if (json.contains(JKEY_CNI_CCPI_SUBNET)) {
    ipam.subnet_ = json.at(JKEY_CNI_CCPI_SUBNET).get<std::string>();
  }
  if (json.contains(JKEY_CNI_CCPI_GATEWAY)) {
    ipam.gateway_ = json.at(JKEY_CNI_CCPI_GATEWAY).get<std::string>();
  }
}

void to_json(nlohmann::json &json, const CniConfigPluginsIpam &ipam) {
  json = nlohmann::json{{JKEY_CNI_CCPI_TYPE, ipam.type_},
                        {JKEY_CNI_CCPI_SUBNET, ipam.subnet_},
                        {JKEY_CNI_CCPI_GATEWAY, ipam.gateway_}};
}

void from_json(const nlohmann::json &json, CniConfigPlugins &plugins) {
  if (json.contains(JKEY_CNI_CCP_TYPE)) {
    plugins.type_ = json.at(JKEY_CNI_CCP_TYPE).get<std::string>();
  }
  if (json.contains(JKEY_CNI_CCP_BRIDGE)) {
    plugins.bridge_ = json.at(JKEY_CNI_CCP_BRIDGE).get<std::string>();
  }
  if (json.contains(JKEY_CNI_CCP_LOG)) {
    plugins.log_ = json.at(JKEY_CNI_CCP_LOG).get<std::string>();
  }
  if (json.contains(JKEY_CNI_CCP_LOGLEVEL)) {
    plugins.loglevel_ = json.at(JKEY_CNI_CCP_LOGLEVEL).get<CniConfigPlugins::LogLevel>();
  }
  if (json.contains(JKEY_CNI_CCP_SUBNET_PREFIX)) {
    plugins.subnet_prefix_ = json.at(JKEY_CNI_CCP_SUBNET_PREFIX).get<int>();
  }
  if (json.contains(JKEY_CNI_CCP_IPAM)) {
    plugins.ipam_ = json.at(JKEY_CNI_CCP_IPAM).get<CniConfigPluginsIpam>();
  }
}

void to_json(nlohmann::json &json, const CniConfigPlugins &plugins) {
  json = nlohmann::json{{JKEY_CNI_CCP_TYPE, plugins.type_},
                        {JKEY_CNI_CCP_BRIDGE, plugins.bridge_},
                        {JKEY_CNI_CCP_LOG, plugins.log_},
                        {JKEY_CNI_CCP_LOGLEVEL, plugins.loglevel_},
                        {JKEY_CNI_CCP_SUBNET_PREFIX, plugins.subnet_prefix_},
                        {JKEY_CNI_CCP_IPAM, plugins.ipam_}};
}

void from_json(const nlohmann::json &json, CniConfig &conf) {
  if (json.contains(JKEY_CNI_CC_VERSION)) {
    conf.cni_version_ = json.at(JKEY_CNI_CC_VERSION).get<std::string>();
  }
  if (json.contains(JKEY_CNI_CC_NAME)) {
    conf.name_ = json.at(JKEY_CNI_CC_NAME).get<std::string>();
  }
  if (json.contains(JKEY_CNI_CC_PLUGINS)) {
    conf.plugins_ = json.at(JKEY_CNI_CC_PLUGINS).get<CniConfigPlugins>();
  }
}

void to_json(nlohmann::json &json, const CniConfig &conf) {
  json = nlohmann::json{{JKEY_CNI_CC_VERSION, conf.cni_version_},
                        {JKEY_CNI_CC_NAME, conf.name_},
                        {JKEY_CNI_CC_PLUGINS, conf.plugins_}};
}

} // namespace cni
} // namespace ohno
