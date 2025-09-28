#pragma once

// clang-format off
#include <string>
#include <string_view>
#include <vector>
#include "nlohmann/json.hpp"
// clang-format on

namespace ohno {
namespace kube {
namespace apiv1 {

constexpr std::string_view JKEY_KUBE_METADATA_NAME{"name"};
constexpr std::string_view JKEY_KUBE_ADDRESS_TYPE{"type"};
constexpr std::string_view JKEY_KUBE_ADDRESS_ADDR{"address"};
constexpr std::string_view JKEY_KUBE_STATUS_ADDR{"addresses"};
constexpr std::string_view JKEY_KUBE_SPEC_PODCIDR{"podCIDR"};
constexpr std::string_view JKEY_KUBE_ITEM_MD{"metadata"};
constexpr std::string_view JKEY_KUBE_ITEM_STATUS{"status"};
constexpr std::string_view JKEY_KUBE_ITEM_SPEC{"spec"};
constexpr std::string_view JKEY_KUBE_NODES_ITEMS{"items"};

class Item;

/**
 * @brief 对应 api/v1/nodes 返回值定义的结构
 *
 * @note 链接：
 * https://kubernetes.io/docs/reference/generated/kubernetes-api/v1.26/#node-v1-core
 */
class KubeApiv1Nodes {
public:
  friend void from_json(const nlohmann::json &json, KubeApiv1Nodes &nodes);
  friend void to_json(nlohmann::json &json, const KubeApiv1Nodes &nodes);

  std::vector<Item> items_;
};

class Metadata {
public:
  friend void from_json(const nlohmann::json &json, Metadata &meta);
  friend void to_json(nlohmann::json &json, const Metadata &meta);

  std::string name_;
};

class Address {
public:
  enum class Type : uint8_t { Reserved, InternalIP, Hostname };
  friend void from_json(const nlohmann::json &json, Address &addr);
  friend void to_json(nlohmann::json &json, const Address &addr);

  Type type_;
  std::string address_;
};

class Status {
public:
  friend void from_json(const nlohmann::json &json, Status &status);
  friend void to_json(nlohmann::json &json, const Status &status);

  std::vector<Address> addresses_;
};

class Spec {
public:
  friend void from_json(const nlohmann::json &json, Spec &spec);
  friend void to_json(nlohmann::json &json, const Spec &spec);

  std::string pod_cidr_;
};

class Item {
public:
  friend void from_json(const nlohmann::json &json, Item &items);
  friend void to_json(nlohmann::json &json, const Item &items);

  Metadata metadata_;
  Status status_;
  Spec spec_;
};

} // namespace apiv1
} // namespace kube
} // namespace ohno
