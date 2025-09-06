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
namespace ipam {

constexpr std::string_view ETCDCTL_VERSION{"ETCDCTL_API"};
constexpr std::string_view ETCDCTL_VERSION_VALUE{"3"};

class EtcdClientShell final : public EtcdClientIf, public log::Loggable<log::Id::ipam> {
public:
  explicit EtcdClientShell(const EtcdData &etcd_data, std::unique_ptr<util::ShellIf> shell,
                           std::unique_ptr<util::EnvIf> env);
  ~EtcdClientShell();

  auto put(std::string_view key, std::string_view value) -> bool override;
  auto append(std::string_view key, std::string_view value) -> bool override;
  auto get(std::string_view key, std::string &value) -> bool override;
  auto del(std::string_view key) -> bool override;
  auto del(std::string_view key, std::string_view value) -> bool override;
  auto list(std::string_view key, std::vector<std::string> &results) -> bool override;

private:
  EtcdData etcd_data_;
  std::string command_prefix_;
  std::unique_ptr<util::ShellIf> shell_;
  std::unique_ptr<util::EnvIf> env_;
};

} // namespace ipam
} // namespace ohno
