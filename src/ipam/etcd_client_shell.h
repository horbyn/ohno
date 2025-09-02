#pragma once

// clang-format off
#include <memory>
#include <vector>
#include "etcd_client_if.h"
#include "etcd_data.hpp"
#include "src/log/logger.h"
#include "src/util/shell_if.h"
// clang-format on

namespace ohno {
namespace ipam {

constexpr std::string_view ETCDCTL_VERSION{"ETCDCTL_API=3"};

class EtcdClientShell final : public EtcdClientIf, public log::Loggable<log::Id::etcdcli> {
public:
  explicit EtcdClientShell(const EtcdData &etcd_data, std::unique_ptr<util::ShellIf> shell);
  auto put(std::string_view key, std::string_view value) -> bool override;
  auto append(std::string_view key, std::string_view value) -> bool override;
  auto get(std::string_view key, std::string &value) -> bool override;
  auto del(std::string_view key) -> bool override;
  auto del(std::string_view key, std::string_view value) -> bool override;
  auto list(std::string_view key, std::vector<std::string> &results) -> bool override;

private:
  auto executeCommand(const std::string &command, std::string &output) const -> bool;

  EtcdData etcd_data_;
  std::string command_prefix_;
  std::unique_ptr<util::ShellIf> shell_;
};

} // namespace ipam
} // namespace ohno
