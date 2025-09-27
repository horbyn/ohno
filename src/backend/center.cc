// clang-format off
#include "center.h"
#include <filesystem>
#include <fstream>
#include "nlohmann/json.hpp"
#include "backend_info.h"
#include "src/cni/cni_config.h"
#include "src/common/assert.h"
#include "src/common/except.h"
#include "src/net/http_client/http_client.h"
// clang-format on

namespace ohno {
namespace backend {

Center::Center(std::string_view api_server, bool insecure, Type type)
    : api_server_{api_server}, ssl_{insecure}, type_{type} {
  if (ssl_) {
    ca_path_ = getCa(type_);
  }
  if (ssl_) {
    token_ = getToken(type_);
  }
}

/**
 * @brief 测试 api server 是否健康
 *
 * @return true 健康
 * @return false 不健康
 */
auto Center::test() const -> bool {
  net::HttpCode code{net::HttpCode::Bad_Request};
  try {
    if (ssl_ && (ca_path_.empty() || token_.empty())) {
      // CA 或者 token 随便一个为空
      throw OHNO_EXCEPT(
          fmt::format(
              "Fails to access api server because ca:\"{}\" or token \"{}\" from \"{}\" invalid",
              ca_path_, token_, fmt::format("{}/token", getSslDir(type_, false))),
          false);
    }

    net::HttpClient client{};
    std::string response{};
    code = getHttpResponse(&client, fmt::format("{}/{}", api_server_, KUBE_HEALTH), response);
    if (code != net::HttpCode::Ok) {
      throw OHNO_EXCEPT(fmt::format("Fails to access api server, getting code:{} with "
                                    "response:\"{}\", using token \"{}\" from \"{}\"",
                                    static_cast<long>(code), response, token_,
                                    fmt::format("{}/token", getSslDir(type_, false))),
                        false);
    }
  } catch (const ohno::except::Exception &exc) {
    OHNO_LOG(warn, "{}, api server \"{}\", ca path \"{}\", and ssl \"{}\"", exc.getMsg(),
             api_server_, ca_path_, (ssl_ ? "enable" : "disable"));
    return false;
  } catch (const std::exception &exc) {
    OHNO_LOG(warn, "Center test with: {}, api server \"{}\", ca path \"{}\" and ssl \"{}\"",
             exc.what(), api_server_, ca_path_, (ssl_ ? "enable" : "disable"));
    return false;
  }

  OHNO_LOG(info, "Successfully accesses api server: {}, ca_path: {}, token_path: {}, ssl: {}",
           api_server_, ca_path_, fmt::format("{}/token", getSslDir(type_, false)),
           (ssl_ ? "enable" : "disable"));
  return true;
}

/**
 * @brief 获取单个 Kubernetes 节点信息
 *
 * @param node_name 节点名称
 * @return NodeInfo 节点信息
 */
auto Center::getKubernetesData(std::string_view node_name) const -> NodeInfo {
  NodeInfo info{};
  info.name_ = node_name;
  std::unordered_map<std::string, NodeInfo> unused{};
  net::HttpClient client{};
  getNodes(&client, false, info, unused);
  return info;
}

/**
 * @brief 获取所有 Kubernetes 节点信息
 *
 * @return std::unordered_map<std::string, NodeInfo> 以节点名称为 key，以节点信息为 value 的哈希表
 */
auto Center::getKubernetesData() const -> std::unordered_map<std::string, NodeInfo> {
  std::unordered_map<std::string, NodeInfo> ret{};
  NodeInfo unused{};
  net::HttpClient client{};
  getNodes(&client, true, unused, ret);
  return ret;
}

/**
 * @brief 从 kubelet 配置中获取 ETCD 集群地址
 *
 * @return std::string 返回地址字符串，如果没有配置则返回空
 */
auto Center::getEtcdClusters() -> std::string {
  std::string etcd_cluster{};

  try {
    std::string uri = Center::getServerUrl(HOST_FILE);
    size_t startPos = uri.find("://");
    OHNO_ASSERT(startPos != std::string::npos);
    startPos += 3; // "://" 之后就是主机名部分

    // 寻找主机部分的结束位置，即冒号（:）或者末尾
    size_t endPos = uri.find(":", startPos);
    if (endPos == std::string::npos) {
      endPos = uri.length(); // 如果没有找到冒号，表示没有端口号，主机地址到末尾
    }
    etcd_cluster = fmt::format("https://{}:2379", uri.substr(startPos, endPos - startPos));
  } catch (const except::Exception &exc) {
    OHNO_GLOBAL_LOG(warn, "Fails to get ETCD cluster: {}", exc.getMsg());
    etcd_cluster.clear();
  } catch (const std::exception &exc) {
    OHNO_GLOBAL_LOG(warn, "Fails to get ETCD cluster: {}", exc.what());
    etcd_cluster.clear();
  }

  return etcd_cluster;
}

/**
 * @brief 获取 Api server 地址
 *
 * @param type 执行环境类型
 * @param env Env 对象
 * @return std::string Api server 地址，获取失败返回空字符串
 */
auto Center::getApiServer(Type type, const util::EnvIf *env) -> std::string {
  std::string api_server{};

  try {
    if (type == Type::HOST) {
      // 主机环境
      api_server = Center::getServerUrl(HOST_FILE);
    } else {
      OHNO_ASSERT(env != nullptr);

      // Pod 环境
      constexpr std::string_view KUBERNETES_SERVICE_HOST{"KUBERNETES_SERVICE_HOST"};
      constexpr std::string_view KUBERNETES_SERVICE_PORT{"KUBERNETES_SERVICE_PORT"};
      auto host = env->get(KUBERNETES_SERVICE_HOST);
      auto port = env->get(KUBERNETES_SERVICE_PORT);

      if (!host.empty()) {
        api_server = fmt::format("https://{}:{}", host, port.empty() ? "443" : port);
      } else {
        constexpr std::string_view POD_API_SERVER{"https://kubernetes.default.svc.cluster.local"};
        api_server = std::string{POD_API_SERVER};
      }
    }
  } catch (const except::Exception &exc) {
    OHNO_GLOBAL_LOG(warn, "Fails to get api server: {}", exc.getMsg());
    api_server.clear();
  } catch (const std::exception &exc) {
    OHNO_GLOBAL_LOG(warn, "Fails to get api server: {}", exc.what());
    api_server.clear();
  }

  return api_server;
}

/**
 * @brief 获取 ssl 目录
 *
 * @param type 执行环境类型
 * @param host_ca HOST 环境下 CA 目录（true）还是 token 目录（false），如果是 POD 环境则忽略这个参数
 * @return std::string_view ssl 目录
 */
auto Center::getSslDir(Type type, bool host_ca) const -> std::string_view {
  if (type == Type::POD) {
    return PATH_CA_POD;
  } else {
    return host_ca ? PATH_CA_HOST : PATH_TOKEN_HOST;
  }
}

/**
 * @brief 获取 token
 *
 * @param type 执行环境类型
 * @return std::string token，文件不存在返回空字符串
 */
auto Center::getToken(Type type) const -> std::string {
  std::string token{};
  try {
    token = Center::readFile(
        fmt::format("{}/token", getSslDir(type, false /* 当 type 是 POD 时该参数无效 */)));

    if (type == Type::POD) {
      // 运行在 Pod 环境的 Daemon Set 需要维护 token，向记录到宿主机
      std::ofstream file{fmt::format("{}/token", getSslDir(Type::HOST, false))};
      if (!file.is_open()) {
        OHNO_LOG(warn, "Failed to write token to host: {}", strerror(errno));
      }
      file << token;
    }
  } catch (const except::Exception &exc) {
    OHNO_LOG(warn, "Failed to get token to access api server: {}", exc.getMsg());
    token.clear();
  } catch (const std::exception &exc) {
    OHNO_LOG(warn, "Failed to get token to access api server: {}", exc.what());
    token.clear();
  }
  return token;
}

/**
 * @brief 获取 CA 证书路径
 *
 * @param type 执行环境类型
 * @return std::string CA 证书路径，文件不存在返回空
 */
auto Center::getCa(Center::Type type) const -> std::string {
  auto pathname = fmt::format("{}/ca.crt", getSslDir(type, true));
  namespace fs = std::filesystem;
  fs::path path{pathname};
  if (!fs::exists(path)) {
    OHNO_LOG(warn, "Failed to get CA to access api server: \"{}\" is not exist", pathname);
    pathname.clear();
  }
  return pathname;
}

/**
 * @brief 发起 HTTP Get 请求
 *
 * @param http_client HTTP client
 * @param uri uri
 * @param response 响应
 * @return net::HttpCode 返回码
 */
auto Center::getHttpResponse(const net::HttpClientIf *http_client, std::string_view uri,
                             std::string &response) const -> net::HttpCode {
  OHNO_ASSERT(http_client != nullptr);
  OHNO_ASSERT(!uri.empty());
  OHNO_ASSERT(!api_server_.empty());
  OHNO_ASSERT(!token_.empty());
  OHNO_ASSERT(ssl_ && !ca_path_.empty());

  std::string ca_path = ssl_ ? ca_path_ : std::string{};
  auto code = http_client->httpRequest(net::HttpMethod::GET, uri, response, {}, token_, ca_path);
  return code;
}

/**
 * @brief 调用接口获取 api server 数据
 *
 * @param http_client http client
 * @param is_all 获取所有节点（true）
 * @param single 单个节点（返回值）
 * @param all 所有节点（返回值）
 * @return std::string 数据
 */
auto Center::getNodes(const net::HttpClientIf *http_client, bool is_all, NodeInfo &single,
                      std::unordered_map<std::string, NodeInfo> &all) const -> void {
  OHNO_ASSERT(http_client != nullptr);
  OHNO_ASSERT(is_all ||
              (!is_all && !single.name_.empty())); // 要么获取全部，要么获取单个（并且给出名称）
  OHNO_ASSERT(!api_server_.empty());
  OHNO_ASSERT(!token_.empty());

  std::string response{};
  try {
    auto code =
        getHttpResponse(http_client, fmt::format("{}/{}", api_server_, KUBE_API_NODES), response);
    if (code != net::HttpCode::Ok) {
      throw OHNO_EXCEPT(fmt::format("HTTP req failed with code: {} and \"{}\"",
                                    static_cast<long>(code), response),
                        false);
    }
    auto nodes = nlohmann::json::parse(response);

    if (nodes.contains("items")) {
      NodeInfo info{};
      for (const auto &item : nodes["items"]) {
        if (item.contains("metadata") && item["metadata"].contains("name")) {
          info.name_ = item["metadata"]["name"];
        }
        if (!is_all && info.name_ != single.name_) {
          continue;
        }

        if (item.contains("status") && item["status"].contains("addresses")) {
          for (const auto &addr : item["status"]["addresses"]) {
            if (addr.contains("type") && addr["type"] == "InternalIP") {
              info.internal_ip_ = addr["address"];
              break;
            }
          }
        }

        if (item.contains("spec") && item["spec"].contains("podCIDR")) {
          info.pod_cidr_ = item["spec"]["podCIDR"];
        }

        if (!is_all) {
          single = info;
          break;
        } else {
          all[info.name_] = info;
        }
      }
    }
  } catch (const ohno::except::Exception &exc) {
    OHNO_LOG(
        warn,
        "Get Kubernetes nodes from api server failed, because the Kubernetes API format error: {}",
        exc.getMsg());
  } catch (const std::exception &exc) {
    OHNO_LOG(
        warn,
        "Get Kubernetes nodes from api server failed, because the Kubernetes API format error: {}",
        exc.what());
  }
}

/**
 * @brief 读取文件
 *
 * @param filename 文件名
 * @return std::string 文件内容
 */
auto Center::readFile(std::string_view filename) -> std::string {
  OHNO_ASSERT(!filename.empty());
  std::ifstream file{std::string{filename}};
  if (!file.is_open()) {
    throw OHNO_EXCEPT(fmt::format("File:\"{}\" is not exist", filename), false);
  }
  std::string content{(std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>()};
  content.erase(std::remove(content.begin(), content.end(), '\n'),
                content.end()); // 移除文件结尾的换行符
  return content;
}

/**
 * @brief 从配置文件中读取 api server 地址
 *
 * @param conf_path 配置文件
 * @return std::string 地址，读取文件失败时返回空字符串
 */
auto Center::getServerUrl(std::string_view conf_path) -> std::string {
  auto yaml = Center::readFile(conf_path);
  if (yaml.empty()) {
    return {};
  }

  // TODO: 改为用 yaml 第三方库解析

  // 查找 "clusters:" 位置
  size_t clusters_pos = yaml.find("clusters:");
  if (clusters_pos == std::string::npos) {
    return ""; // 找不到 clusters 字段
  }

  // 查找 clusters[0].cluster.server 字段的 "server:" 位置
  constexpr std::string_view SERVER{"server:"};
  size_t server_pos = yaml.find(SERVER, clusters_pos);
  if (server_pos == std::string::npos) {
    return {};
  }

  // 找到 "server:" 后，跳过字段名的长度 (7 + 1 为空格)
  server_pos += SERVER.size() + 1;

  // 查找当前行结束（空格）
  size_t end_pos = yaml.find(" ", server_pos);
  if (end_pos == std::string::npos) {
    end_pos = yaml.length(); // 如果没有换行符，则直到字符串结尾
  }

  // 提取 server URL
  return yaml.substr(server_pos, end_pos - server_pos);
}

} // namespace backend
} // namespace ohno
