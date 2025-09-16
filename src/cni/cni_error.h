#pragma once

// clang-format off
#include <string>
#include <string_view>
#include "nlohmann/json.hpp"
#include "src/common/except.h"
// clang-format on

namespace ohno {
namespace cni {

constexpr std::string_view JKEY_CNI_CE_CNIVERSION{"cniVersion"};
constexpr std::string_view JKEY_CNI_CE_CODE{"code"};
constexpr std::string_view JKEY_CNI_CE_MSG{"msg"};
constexpr std::string_view JKEY_CNI_CE_DETAILS{"details"};
constexpr int CNI_ERRCODE_OHNO{278};
constexpr int CNI_ERRCODE_NOT_SUPPORTED{287};

class CniError final : public except::Exception {
public:
  enum class Code {
    VERSION = 1,
    UNSUPPORTED_FIELD,
    CONTAINER,
    ENV_VAR,
    IO,
    DECODE,
    NETWORK,
    RETRY = 11,
    OHNO = CNI_ERRCODE_OHNO,
    OHNO_NOT_SUPPORTED = CNI_ERRCODE_NOT_SUPPORTED,
  };
  explicit CniError(int code, std::string_view details);

  friend void from_json(const nlohmann::json &json, CniError &error);
  friend void to_json(nlohmann::json &json, const CniError &error);

private:
  // 这里 version 版本好像不同
  // https://github.com/containernetworking/cni.dev/blob/main/content/docs/spec.md#error
  std::string cniversion_{"1.1.0"};
  int code_;
  std::string details_;
};

} // namespace cni

#define OHNO_CNIERR(code, msg) (cni::CniError(code, msg))

} // namespace ohno
