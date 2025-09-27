#pragma once

// clang-format off
#include <string>
#include <string_view>
#include "nlohmann/json.hpp"
// clang-format on

namespace ohno {
namespace cni {

constexpr std::string_view JKEY_CNI_CRIPS_ADDRESS{"address"};
constexpr std::string_view JKEY_CNI_CRIPS_GATEWAY{"gateway"};
constexpr std::string_view JKEY_CNI_CRI_NAME{"name"};
constexpr std::string_view JKEY_CNI_CRI_SANDBOX{"sandbox"};
constexpr std::string_view JKEY_CNI_CR_CNIVERSION{"cniVersion"};
constexpr std::string_view JKEY_CNI_CR_INTERFACES{"interfaces"};
constexpr std::string_view JKEY_CNI_CR_IPS{"ips"};
constexpr std::string_view JKEY_CNI_CV_CNIVERSION{"cniVersion"};
constexpr std::string_view JKEY_CNI_CV_SUPPORTED{"supportedVersions"};

class CniResultIps final {
public:
  friend void from_json(const nlohmann::json &json, CniResultIps &ips);
  friend void to_json(nlohmann::json &json, const CniResultIps &ips);

  std::string address_;
  std::string gateway_;
};

class CniResultInterfaces final {
public:
  friend void from_json(const nlohmann::json &json, CniResultInterfaces &interfance);
  friend void to_json(nlohmann::json &json, const CniResultInterfaces &interfance);

  std::string name_;
  std::string sandbox_;
};

class CniResult final {
public:
  friend void from_json(const nlohmann::json &json, CniResult &result);
  friend void to_json(nlohmann::json &json, const CniResult &result);

  std::string cniversion_;
  std::vector<CniResultIps> ips_;
  std::vector<CniResultInterfaces> interfaces_;
};

class CniVersion final {
public:
  friend void from_json(const nlohmann::json &json, CniVersion &version);
  friend void to_json(nlohmann::json &json, const CniVersion &version);

  std::string cni_version_;

  // TODO: 我也不知道这里的版本列表有什么用，先随便写点
  std::vector<std::string> supported_versions_{"0.3.0", "0.3.1", "0.4.0"};
};

} // namespace cni
} // namespace ohno
