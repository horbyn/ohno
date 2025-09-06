#pragma once

// clang-format off
#include <string>
#include <string_view>
#include "nlohmann/json.hpp"
// clang-format on

namespace ohno {
namespace cni {

/* ↓ 既作为 json key，也作为环境变量的名称 ↓ */

constexpr std::string_view JKEY_CNI_CE_COMMAND{"CNI_COMMAND"};
constexpr std::string_view JKEY_CNI_CE_CONTAINERID{"CNI_CONTAINERID"};
constexpr std::string_view JKEY_CNI_CE_NETNS{"CNI_NETNS"};
constexpr std::string_view JKEY_CNI_CE_IFNAME{"CNI_IFNAME"};

class CniEnv final {
public:
  friend void from_json(const nlohmann::json &json, CniEnv &env);
  friend void to_json(nlohmann::json &json, const CniEnv &env);

  std::string command_;
  std::string container_id_;
  std::string netns_;
  std::string ifname_;
};

} // namespace cni
} // namespace ohno
