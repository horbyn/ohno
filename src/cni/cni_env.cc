// clang-format off
#include "cni_env.h"
// clang-format on

namespace ohno {
namespace cni {

void from_json(const nlohmann::json &json, CniEnv &env) {
  if (json.contains(JKEY_CNI_CE_COMMAND)) {
    env.command_ = json.at(JKEY_CNI_CE_COMMAND).get<std::string>();
  }
  if (json.contains(JKEY_CNI_CE_CONTAINERID)) {
    env.container_id_ = json.at(JKEY_CNI_CE_CONTAINERID).get<std::string>();
  }
  if (json.contains(JKEY_CNI_CE_NETNS)) {
    env.netns_ = json.at(JKEY_CNI_CE_NETNS).get<std::string>();
  }
  if (json.contains(JKEY_CNI_CE_IFNAME)) {
    env.ifname_ = json.at(JKEY_CNI_CE_IFNAME).get<std::string>();
  }
}

void to_json(nlohmann::json &json, const CniEnv &env) {
  json = nlohmann::json{{JKEY_CNI_CE_COMMAND, env.command_},
                        {JKEY_CNI_CE_CONTAINERID, env.container_id_},
                        {JKEY_CNI_CE_NETNS, env.netns_},
                        {JKEY_CNI_CE_IFNAME, env.ifname_}};
}

} // namespace cni
} // namespace ohno
