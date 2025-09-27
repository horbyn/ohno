#pragma once

// clang-format off
#include "center_if.h"
#include <string_view>
#include "src/log/logger.h"
#include "src/net/http_client/http_client_if.h"
#include "src/util/env_if.h"
// clang-format on

namespace ohno {
namespace backend {

constexpr std::string_view KUBE_HEALTH{"healthz"};
constexpr std::string_view KUBE_API_NODES{"api/v1/nodes"};
constexpr std::string_view HOST_FILE{"/etc/kubernetes/kubelet.conf"};

class Center : public CenterIf, public log::Loggable<log::Id::backend> {
public:
  enum class Type : uint8_t { HOST /* 宿主机环境 */, POD /* Pod 环境 */ };
  explicit Center(std::string_view api_server, bool insecure, Type type);

  auto test() const -> bool override;
  auto getKubernetesData(std::string_view node_name) const -> NodeInfo override;
  auto getKubernetesData() const -> std::unordered_map<std::string, NodeInfo> override;

  static auto getEtcdClusters() -> std::string;
  static auto getApiServer(Type type, const util::EnvIf *env) -> std::string;

private:
  auto getSslDir(Type type, bool host_ca) const -> std::string_view;
  auto getToken(Type type) const -> std::string;
  auto getCa(Type type) const -> std::string;
  auto getHttpResponse(const net::HttpClientIf *http_client, std::string_view uri,
                       std::string &response) const -> net::HttpCode;
  auto getNodes(const net::HttpClientIf *http_client, bool is_all, NodeInfo &single,
                std::unordered_map<std::string, NodeInfo> &all) const -> void;
  static auto readFile(std::string_view filename) -> std::string;
  static auto getServerUrl(std::string_view conf_path) -> std::string;

  std::string api_server_;
  bool ssl_;
  Type type_;
  std::string token_;
  std::string ca_path_;
};

} // namespace backend
} // namespace ohno
