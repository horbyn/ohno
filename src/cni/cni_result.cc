// clang-format off
#include "cni_result.h"
// clang-format on

namespace ohno {
namespace cni {

void from_json(const nlohmann::json &json, CniResultIps &ips) {
  if (json.contains(JKEY_CNI_CRIPS_ADDRESS)) {
    ips.address_ = json.at(JKEY_CNI_CRIPS_ADDRESS).get<std::string>();
  }
  if (json.contains(JKEY_CNI_CRIPS_GATEWAY)) {
    ips.gateway_ = json.at(JKEY_CNI_CRIPS_GATEWAY).get<std::string>();
  }
  if (json.contains(JKEY_CNI_CRIPS_INTERFACE)) {
    ips.interface_ = json.at(JKEY_CNI_CRIPS_INTERFACE).get<uint32_t>();
  }
}

void to_json(nlohmann::json &json, const CniResultIps &ips) {
  json = nlohmann::json{{JKEY_CNI_CRIPS_ADDRESS, ips.address_},
                        {JKEY_CNI_CRIPS_GATEWAY, ips.gateway_},
                        {JKEY_CNI_CRIPS_INTERFACE, ips.interface_}};
}

void from_json(const nlohmann::json &json, CniResultInterfaces &interfance) {
  if (json.contains(JKEY_CNI_CRI_NAME)) {
    interfance.name_ = json.at(JKEY_CNI_CRI_NAME).get<std::string>();
  }
  if (json.contains(JKEY_CNI_CRI_SANDBOX)) {
    interfance.sandbox_ = json.at(JKEY_CNI_CRI_SANDBOX).get<std::string>();
  }
}

void to_json(nlohmann::json &json, const CniResultInterfaces &interfance) {
  json = nlohmann::json{{JKEY_CNI_CRI_NAME, interfance.name_},
                        {JKEY_CNI_CRI_SANDBOX, interfance.sandbox_}};
}

void from_json(const nlohmann::json &json, CniResult &result) {
  if (json.contains(JKEY_CNI_CR_CNIVERSION)) {
    result.cniversion_ = json.at(JKEY_CNI_CR_CNIVERSION).get<std::string>();
  }
  if (json.contains(JKEY_CNI_CR_INTERFACES)) {
    result.interfaces_ = json.at(JKEY_CNI_CR_INTERFACES).get<std::vector<CniResultInterfaces>>();
  }
  if (json.contains(JKEY_CNI_CR_IPS)) {
    result.ips_ = json.at(JKEY_CNI_CR_IPS).get<std::vector<CniResultIps>>();
  }
}

void to_json(nlohmann::json &json, const CniResult &result) {
  json = nlohmann::json{{JKEY_CNI_CR_CNIVERSION, result.cniversion_},
                        {JKEY_CNI_CR_INTERFACES, result.interfaces_},
                        {JKEY_CNI_CR_IPS, result.ips_}};
}

void from_json(const nlohmann::json &json, CniVersion &version) {
  if (json.contains(JKEY_CNI_CV_CNIVERSION)) {
    version.cni_version_ = json.at(JKEY_CNI_CV_CNIVERSION).get<std::string>();
  }
  if (json.contains(JKEY_CNI_CV_SUPPORTED)) {
    version.supported_versions_ = json.at(JKEY_CNI_CV_SUPPORTED).get<std::vector<std::string>>();
  }
}

void to_json(nlohmann::json &json, const CniVersion &version) {
  json = nlohmann::json{{JKEY_CNI_CV_CNIVERSION, version.cni_version_},
                        {JKEY_CNI_CV_SUPPORTED, version.supported_versions_}};
}

} // namespace cni
} // namespace ohno
