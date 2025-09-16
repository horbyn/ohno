#pragma once

// clang-format off
#include <string>
#include <string_view>
#include "src/log/logger.h"
#include "src/util/env_std.h"
// clang-format on

namespace ohno {
namespace etcd {

class EtcdData final {
public:
  explicit EtcdData() {
    util::EnvStd env{};
    endpoints_ = env.get("ETCD_ENDPOINTS");
    if (endpoints_.empty()) {
      endpoints_ = "https://127.0.0.1:2379";
      OHNO_GLOBAL_LOG(warn, "Env var ETCD_ENDPOINTS is empty, use default: {}", endpoints_);
    }
    ca_cert_ = env.get("ETCD_CACERT");
    if (ca_cert_.empty()) {
      ca_cert_ = "/etc/kubernetes/pki/etcd/ca.crt";
      OHNO_GLOBAL_LOG(warn, "Env var ETCD_CACERT is empty, use default: {}", ca_cert_);
    }
    cert_ = env.get("ETCD_CERT");
    if (cert_.empty()) {
      cert_ = "/etc/kubernetes/pki/etcd/healthcheck-client.crt";
      OHNO_GLOBAL_LOG(warn, "Env var ETCD_CERT is empty, use default: {}", cert_);
    }
    key_ = env.get("ETCD_KEY");
    if (key_.empty()) {
      key_ = "/etc/kubernetes/pki/etcd/healthcheck-client.key";
      OHNO_GLOBAL_LOG(warn, "Env var ETCD_KEY is empty, use default: {}", key_);
    }

    OHNO_GLOBAL_LOG(
        debug,
        "ETCD client initialized with endpoints: \"{}\", ca_cert: \"{}\", cert: \"{}\", key: "
        "\"{}\"",
        endpoints_, ca_cert_, cert_, key_);
  }
  explicit EtcdData(std::string_view endpoints, std::string_view ca_cert, std::string_view cert,
                    std::string_view key)
      : endpoints_{endpoints}, ca_cert_{ca_cert}, cert_{cert}, key_{key} {}

  std::string endpoints_;
  std::string ca_cert_;
  std::string cert_;
  std::string key_;
};

} // namespace etcd
} // namespace ohno
