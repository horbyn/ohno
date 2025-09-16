// clang-format off
#include "cni_config.h"
#include "src/common/enum_name.hpp"
// clang-format on

namespace ohno {
namespace cni {

void from_json(const nlohmann::json &json, CniConfigIpam &ipam) {
  if (json.contains(JKEY_CNI_CCI_SUBNET)) {
    ipam.subnet_ = json.at(JKEY_CNI_CCI_SUBNET).get<std::string>();
  }
  if (json.contains(JKEY_CNI_CCI_GATEWAY)) {
    ipam.gateway_ = json.at(JKEY_CNI_CCI_GATEWAY).get<std::string>();
  }
}

void to_json(nlohmann::json &json, const CniConfigIpam &ipam) {
  json = nlohmann::json{{JKEY_CNI_CCI_SUBNET, ipam.subnet_}, {JKEY_CNI_CCI_GATEWAY, ipam.gateway_}};
}

void from_json(const nlohmann::json &json, CniConfig &conf) {
  if (json.contains(JKEY_CNI_CC_VERSION)) {
    conf.cni_version_ = json.at(JKEY_CNI_CC_VERSION).get<std::string>();
  }
  if (json.contains(JKEY_CNI_CC_NAME)) {
    conf.name_ = json.at(JKEY_CNI_CC_NAME).get<std::string>();
  }
  if (json.contains(JKEY_CNI_CC_TYPE)) {
    conf.type_ = json.at(JKEY_CNI_CC_TYPE).get<std::string>();
  }
  if (json.contains(JKEY_CNI_CC_BRIDGE)) {
    conf.bridge_ = json.at(JKEY_CNI_CC_BRIDGE).get<std::string>();
  }
  if (json.contains(JKEY_CNI_CC_LOG)) {
    conf.log_ = json.at(JKEY_CNI_CC_LOG).get<std::string>();
  }
  if (json.contains(JKEY_CNI_CC_LOGLEVEL)) {
    auto level_opt = stringEnum<log::Level>(json.at(JKEY_CNI_CC_LOGLEVEL).get<std::string>());
    conf.loglevel_ = level_opt.has_value() ? level_opt.value() : log::Level::info;
  }
  if (json.contains(JKEY_CNI_CC_SUBNET_PREFIX)) {
    conf.subnet_prefix_ = json.at(JKEY_CNI_CC_SUBNET_PREFIX).get<int>();
  }
  if (json.contains(JKEY_CNI_CC_IPAM)) {
    conf.ipam_ = json.at(JKEY_CNI_CC_IPAM).get<CniConfigIpam>();
  }
}

void to_json(nlohmann::json &json, const CniConfig &conf) {
  json = nlohmann::json{{JKEY_CNI_CC_VERSION, conf.cni_version_},
                        {JKEY_CNI_CC_NAME, conf.name_},
                        {JKEY_CNI_CC_TYPE, conf.type_},
                        {JKEY_CNI_CC_BRIDGE, conf.bridge_},
                        {JKEY_CNI_CC_LOG, conf.log_},
                        {JKEY_CNI_CC_LOGLEVEL, enumName(conf.loglevel_)},
                        {JKEY_CNI_CC_SUBNET_PREFIX, conf.subnet_prefix_},
                        {JKEY_CNI_CC_IPAM, conf.ipam_}};
}

} // namespace cni
} // namespace ohno
