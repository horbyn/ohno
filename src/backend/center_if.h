#pragma once

// clang-format off
#include <string>
#include <string_view>
#include <unordered_map>
// clang-format on

namespace ohno {
namespace backend {

struct NodeInfo {
  std::string name_;
  std::string internal_ip_;
  std::string pod_cidr_;
};

constexpr std::string_view PATH_CA_POD{"/var/run/secrets/kubernetes.io/serviceaccount"};
constexpr std::string_view PATH_CA_HOST{"/etc/kubernetes/pki"};
constexpr std::string_view PATH_TOKEN_HOST{"/var/run/ohno"};

class CenterIf {
public:
  virtual ~CenterIf() = default;
  virtual auto test() const -> bool = 0;
  virtual auto getKubernetesData(std::string_view node_name) const -> NodeInfo = 0;
  virtual auto getKubernetesData() const -> std::unordered_map<std::string, NodeInfo> = 0;
};

} // namespace backend
} // namespace ohno
