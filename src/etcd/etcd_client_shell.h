#pragma once

// clang-format off
#include <memory>
#include <vector>
#include "etcd_client_if.h"
#include "etcd_data.hpp"
#include "src/log/logger.h"
#include "src/util/env_if.h"
#include "src/util/shell_if.h"
// clang-format on

namespace ohno {
namespace etcd {

constexpr std::string_view ETCDCTL_VERSION{"ETCDCTL_API"};
constexpr std::string_view ETCDCTL_VERSION_VALUE{"3"};

class EtcdClientShell final : public EtcdClientIf, public log::Loggable<log::Id::etcd> {
public:
  explicit EtcdClientShell(const EtcdData &etcd_data, std::unique_ptr<util::ShellIf> shell,
                           std::unique_ptr<util::EnvIf> env);
  ~EtcdClientShell();

  auto test() const -> bool override;
  auto put(std::string_view key, std::string_view value) const -> bool override;
  auto append(std::string_view key, std::string_view value) const -> bool override;
  auto get(std::string_view key, std::string &value) const -> bool override;
  auto get(std::string_view key, std::unordered_map<std::string, std::string> &value) const
      -> bool override;
  auto del(std::string_view key) const -> bool override;
  auto del(std::string_view key, std::string_view value) const -> bool override;
  auto list(std::string_view key, std::vector<std::string> &results) const -> bool override;
  auto dump(std::string_view key) const -> std::string override;

private:
  EtcdData etcd_data_;
  std::string command_prefix_;
  std::unique_ptr<util::ShellIf> shell_;
  std::unique_ptr<util::EnvIf> env_;
};

} // namespace etcd
} // namespace ohno
