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
    endpoints_ = env.get("ETCDCTL_ENDPOINTS");
    if (endpoints_.empty()) {
      endpoints_ = "https://127.0.0.1:2379";
    }
    ca_cert_ = env.get("ETCDCTL_CACERT");
    if (ca_cert_.empty()) {
      ca_cert_ = "/etc/kubernetes/pki/etcd/ca.crt";
    }
    cert_ = env.get("ETCDCTL_CERT");
    if (cert_.empty()) {
      cert_ = "/etc/kubernetes/pki/etcd/healthcheck-client.crt";
    }
    key_ = env.get("ETCDCTL_KEY");
    if (key_.empty()) {
      key_ = "/etc/kubernetes/pki/etcd/healthcheck-client.key";
    }
  }
  explicit EtcdData(std::string_view endpoints) : EtcdData{} { endpoints_ = endpoints; }

  std::string endpoints_;
  std::string ca_cert_;
  std::string cert_;
  std::string key_;
};

} // namespace etcd
} // namespace ohno
