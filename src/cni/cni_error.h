#pragma once

// clang-format off
#include <string>
#include <string_view>
#include "nlohmann/json.hpp"
// clang-format on

namespace ohno {
namespace cni {

constexpr std::string_view JKEY_CNI_CE_CNIVERSION{"cniVersion"};
constexpr std::string_view JKEY_CNI_CE_CODE{"code"};
constexpr std::string_view JKEY_CNI_CE_MSG{"msg"};
constexpr std::string_view JKEY_CNI_CE_DETAILS{"details"};

class CniError final {
public:
  friend void from_json(const nlohmann::json &json, CniError &error);
  friend void to_json(nlohmann::json &json, const CniError &error);

  // 这里 version 版本好像不同
  // https://github.com/containernetworking/cni.dev/blob/main/content/docs/spec.md#error
  std::string cniversion_{"1.1.0"};
  int code_;
  std::string msg_;
  std::string details_;
};

} // namespace cni
} // namespace ohno
