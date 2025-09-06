// clang-format off
#include "cni_error.h"
// clang-format on

namespace ohno {
namespace cni {

void from_json(const nlohmann::json &json, CniError &error) {
  if (json.contains(JKEY_CNI_CE_CNIVERSION)) {
    error.cniversion_ = json.at(JKEY_CNI_CE_CNIVERSION).get<std::string>();
  }
  if (json.contains(JKEY_CNI_CE_CODE)) {
    error.code_ = json.at(JKEY_CNI_CE_CODE).get<int>();
  }
  if (json.contains(JKEY_CNI_CE_MSG)) {
    error.msg_ = json.at(JKEY_CNI_CE_MSG).get<std::string>();
  }
  if (json.contains(JKEY_CNI_CE_DETAILS)) {
    error.details_ = json.at(JKEY_CNI_CE_DETAILS).get<std::string>();
  }
}

void to_json(nlohmann::json &json, const CniError &error) {
  json = nlohmann::json{{JKEY_CNI_CE_CNIVERSION, error.cniversion_},
                        {JKEY_CNI_CE_CODE, error.code_},
                        {JKEY_CNI_CE_MSG, error.msg_},
                        {JKEY_CNI_CE_DETAILS, error.details_}};
}

} // namespace cni
} // namespace ohno
