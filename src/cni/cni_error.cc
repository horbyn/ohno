// clang-format off
#include "cni_error.h"
// clang-format on

namespace ohno {
namespace cni {

CniError::CniError(int code, std::string_view details) {
  code_ = code;
  details_ = details;
  auto code_enum = static_cast<Code>(code);

  // 参考 CNI Spec
  // https://github.com/containernetworking/cni.dev/blob/main/content/docs/spec.md#error
  switch (code_enum) {
  case Code::VERSION:
    msg_ = "Incompatible CNI version";
    break;
  case Code::UNSUPPORTED_FIELD:
    msg_ = "Unsupported field in network configuration. The error message must contain the key and "
           "value of the unsupported field.";
    break;
  case Code::CONTAINER:
    msg_ = "Container unknown or does not exist. This error implies the runtime does not need to "
           "perform any container network cleanup (for example, calling the DEL action on the "
           "container).";
    break;
  case Code::ENV_VAR:
    msg_ = "Invalid necessary environment variables, like CNI_COMMAND, CNI_CONTAINERID, etc. The "
           "error message must contain the names of invalid variables.";
    break;
  case Code::IO:
    msg_ = "I/O failure. For example, failed to read network config bytes from stdin.";
    break;
  case Code::DECODE:
    msg_ = "Failed to decode content. For example, failed to unmarshal network config from bytes "
           "or failed to decode version info from string.";
    break;
  case Code::NETWORK:
    msg_ = "Invalid network config. If some validations on network configs do not pass, this error "
           "will be raised.";
    break;
  case Code::RETRY:
    msg_ = "Try again later. If the plugin detects some transient condition that should clear up, "
           "it can use this code to notify the runtime it should re-try the operation later.";
    break;
  case Code::OHNO:
    msg_ = "Kubernetes cluster environment troubles (by ohno)";
    break;
  case Code::OHNO_NOT_SUPPORTED:
    msg_ = "Not supported (by ohno)";
    break;
  default:
    msg_ = "Custom error code supported by specific vendors.";
    break;
  }
}

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
