// clang-format off
#include "kube_apiv1_nodes.h"
#include "src/common/enum_name.hpp"
// clang-format on

namespace ohno {
namespace kube {
namespace apiv1 {

void from_json(const nlohmann::json &json, KubeApiv1Nodes &nodes) {
  if (json.contains(JKEY_KUBE_NODES_ITEMS)) {
    nodes.items_ = json.at(JKEY_KUBE_NODES_ITEMS).get<std::vector<Item>>();
  }
}

void to_json(nlohmann::json &json, const KubeApiv1Nodes &nodes) {
  json = nlohmann::json{{JKEY_KUBE_NODES_ITEMS, nodes.items_}};
}

void from_json(const nlohmann::json &json, Metadata &meta) {
  if (json.contains(JKEY_KUBE_METADATA_NAME)) {
    meta.name_ = json.at(JKEY_KUBE_METADATA_NAME).get<std::string>();
  }
}

void to_json(nlohmann::json &json, const Metadata &meta) {
  json = nlohmann::json{{JKEY_KUBE_METADATA_NAME, meta.name_}};
}

void from_json(const nlohmann::json &json, Address &addr) {
  if (json.contains(JKEY_KUBE_ADDRESS_TYPE)) {
    auto type = stringEnum<Address::Type>(json.at(JKEY_KUBE_ADDRESS_TYPE).get<std::string>());
    addr.type_ = type.has_value() ? type.value() : Address::Type::Reserved;
  }
  if (json.contains(JKEY_KUBE_ADDRESS_ADDR)) {
    addr.address_ = json.at(JKEY_KUBE_ADDRESS_ADDR).get<std::string>();
  }
}

void to_json(nlohmann::json &json, const Address &addr) {
  json = nlohmann::json{
      {JKEY_KUBE_ADDRESS_TYPE, enumName(addr.type_)},
      {JKEY_KUBE_ADDRESS_ADDR, addr.address_},
  };
}

void from_json(const nlohmann::json &json, Status &status) {
  if (json.contains(JKEY_KUBE_STATUS_ADDR)) {
    status.addresses_ = json.at(JKEY_KUBE_STATUS_ADDR).get<std::vector<Address>>();
  }
}

void to_json(nlohmann::json &json, const Status &status) {
  json = nlohmann::json{{JKEY_KUBE_STATUS_ADDR, status.addresses_}};
}

void from_json(const nlohmann::json &json, Spec &spec) {
  if (json.contains(JKEY_KUBE_SPEC_PODCIDR)) {
    spec.pod_cidr_ = json.at(JKEY_KUBE_SPEC_PODCIDR).get<std::string>();
  }
}

void to_json(nlohmann::json &json, const Spec &spec) {
  json = nlohmann::json{{JKEY_KUBE_SPEC_PODCIDR, spec.pod_cidr_}};
}

void from_json(const nlohmann::json &json, Item &items) {
  if (json.contains(JKEY_KUBE_ITEM_MD)) {
    items.metadata_ = json.at(JKEY_KUBE_ITEM_MD).get<Metadata>();
  }
  if (json.contains(JKEY_KUBE_ITEM_SPEC)) {
    items.spec_ = json.at(JKEY_KUBE_ITEM_SPEC).get<Spec>();
  }
  if (json.contains(JKEY_KUBE_ITEM_STATUS)) {
    items.status_ = json.at(JKEY_KUBE_ITEM_STATUS).get<Status>();
  }
}

void to_json(nlohmann::json &json, const Item &items) {
  json = nlohmann::json{{JKEY_KUBE_ITEM_MD, items.metadata_},
                        {JKEY_KUBE_ITEM_SPEC, items.spec_},
                        {JKEY_KUBE_ITEM_STATUS, items.status_}};
}

} // namespace apiv1
} // namespace kube
} // namespace ohno
