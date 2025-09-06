#pragma once

// clang-format off
#include <string>
#include <string_view>
#include "nlohmann/json.hpp"
// clang-format on

namespace ohno {
namespace cni {

constexpr std::string_view JKEY_CNI_CCPI_TYPE{"type"};
constexpr std::string_view JKEY_CNI_CCPI_SUBNET{"subnet"};
constexpr std::string_view JKEY_CNI_CCPI_GATEWAY{"gateway"};
constexpr std::string_view JKEY_CNI_CCP_TYPE{"type"};
constexpr std::string_view JKEY_CNI_CCP_BRIDGE{"bridge"};
constexpr std::string_view JKEY_CNI_CCP_LOG{"log"};
constexpr std::string_view JKEY_CNI_CCP_LOGLEVEL{"logLevel"};
constexpr std::string_view JKEY_CNI_CCP_SUBNET_PREFIX{"subnetPrefix"};
constexpr std::string_view JKEY_CNI_CCP_IPAM{"ipam"};
constexpr std::string_view JKEY_CNI_CC_VERSION{"cniVersion"};
constexpr std::string_view JKEY_CNI_CC_NAME{"name"};
constexpr std::string_view JKEY_CNI_CC_PLUGINS{"plugins"};

class CniConfigPluginsIpam final {
public:
  friend void from_json(const nlohmann::json &json, CniConfigPluginsIpam &ipam);
  friend void to_json(nlohmann::json &json, const CniConfigPluginsIpam &ipam);

  std::string type_;
  std::string subnet_;
  std::string gateway_;
};

class CniConfigPlugins final {
public:
  enum class LogLevel : std::uint8_t { trace, debug, info, warn, error, critical, off };

  friend void from_json(const nlohmann::json &json, CniConfigPlugins &plugins);
  friend void to_json(nlohmann::json &json, const CniConfigPlugins &plugins);

  std::string type_;
  std::string bridge_;
  std::string log_;   // 没错，我非常需要日志
  LogLevel loglevel_; // 还有日志等级
  int subnet_prefix_; // 每个节点上要划分的 Pod 子网由用户定义
  CniConfigPluginsIpam ipam_;
};

// NOLINTBEGIN(readability-identifier-length)
NLOHMANN_JSON_SERIALIZE_ENUM(CniConfigPlugins::LogLevel,
                             {{CniConfigPlugins::LogLevel::trace, "trace"},
                              {CniConfigPlugins::LogLevel::debug, "debug"},
                              {CniConfigPlugins::LogLevel::info, "info"},
                              {CniConfigPlugins::LogLevel::warn, "warn"},
                              {CniConfigPlugins::LogLevel::error, "error"},
                              {CniConfigPlugins::LogLevel::critical, "critical"},
                              {CniConfigPlugins::LogLevel::off, "off"}})
// NOLINTEND(readability-identifier-length)

class CniConfig final {
public:
  friend void from_json(const nlohmann::json &json, CniConfig &conf);
  friend void to_json(nlohmann::json &json, const CniConfig &conf);

  std::string cni_version_;
  std::string name_;
  std::string type_;
  CniConfigPlugins plugins_;
};

} // namespace cni
} // namespace ohno
