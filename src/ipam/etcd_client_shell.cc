// clang-format off
#include "etcd_client_shell.h"
#include <sstream>
#include "src/common/except.h"
// clang-format on

namespace ohno {
namespace ipam {

EtcdClientShell::EtcdClientShell(const EtcdData &etcd_data, std::unique_ptr<util::ShellIf> shell)
    : etcd_data_{etcd_data}, shell_{std::move(shell)} {
  command_prefix_ = std::string{ETCDCTL_VERSION} + " etcdctl --endpoints=" + etcd_data_.endpoints_ +
                    " --cacert=" + etcd_data_.ca_cert_ + " --cert=" + etcd_data_.cert_ +
                    " --key=" + etcd_data_.key_ + " ";
}

/**
 * @brief 设置一个 ETCD key-value
 *
 * @param key ETCD key
 * @param value ETCD value
 * @return true 设置成功
 * @return false 设置失败
 */
auto EtcdClientShell::put(std::string_view key, std::string_view value) -> bool {
  std::string out{};
  return executeCommand(command_prefix_ + "put " + std::string{key} + " " + std::string{value},
                        out);
}

/**
 * @brief 在原 ETCD key 基础上追加一个 value（除非原 key 不存在或 get 出错，此时行为等于 put）
 *
 * @param key ETCD key
 * @param value ETCD value
 * @return true 设置成功
 * @return false 设置失败
 */
auto EtcdClientShell::append(std::string_view key, std::string_view value) -> bool {
  std::string out{};
  if (get(key, out)) {
    out = out.empty() ? std::string{value} : "," + std::string{value};
  } else {
    // get() 出错
    out = std::string{value};
  }
  return put(key, out);
}

/**
 * @brief 获取一个 ETCD value，适用于 ETCD value 是单个值的情况（如 foo -> bar）
 *
 * @param key ETCD key
 * @param value ETCD value（返回值）
 * @return true 获取成功
 * @return false 获取失败
 */
auto EtcdClientShell::get(std::string_view key, std::string &value) -> bool {
  return executeCommand(command_prefix_ + "get " + std::string{key} + " --print-value-only", value);
}

/**
 * @brief 删除一个 ETCD key-value
 *
 * @param key ETCD key
 * @return true 删除成功
 * @return false 删除失败
 */
auto EtcdClientShell::del(std::string_view key) -> bool {
  std::string out{};
  return executeCommand(command_prefix_ + "del " + std::string{key}, out);
}

/**
 * @brief 从 ETCD value 列表中删除一个值，适用于 ETCD value 是以 ',' 分割的多个值的情况（如 foo -
 * bar,baz）
 *
 * @param key ETCD key
 * @param value 待删除的 ETCD value
 * @return true 删除成功
 * @return false 删除失败
 */
auto EtcdClientShell::del(std::string_view key, std::string_view value) -> bool {
  std::vector<std::string> values{};
  if (!list(key, values)) {
    return false;
  }
  values.erase(std::remove(values.begin(), values.end(), std::string{value}), values.end());
  std::string to_put{};
  for (size_t i = 0; i < values.size(); ++i) {
    to_put += values[i];
    if (i != values.size() - 1) {
      to_put += ",";
    }
  }
  return put(key, to_put);
}

/**
 * @brief 获取 ETCD value 列表，适用于 ETCD value 是以 ',' 分割的多个值的情况（如 foo -> bar,qux）
 *
 * @param key ETCD key
 * @param results ETCD value 列表（返回值）
 * @return true 获取成功
 * @return false 获取失败
 */
auto EtcdClientShell::list(std::string_view key, std::vector<std::string> &results) -> bool {

  std::string output{};
  if (!get(key, output)) {
    return false;
  }

  results.clear();
  std::string token{};
  std::istringstream tokenStream(output);
  while (std::getline(tokenStream, token, ',')) {
    results.push_back(token);
  }
  return true;
}

/**
 * @brief 执行 etcdctl 命令
 *
 * @param command 命令
 * @param output 命令输出
 * @return true 执行成功
 * @return false 执行失败
 */
auto EtcdClientShell::executeCommand(const std::string &command, std::string &output) const
    -> bool {
  std::string err{};
  int ret = shell_->execute(command, output, err);
  if (ret != 0 && !err.empty()) {
    OHNO_LOG(error, "\"{}\" done but with stderr: {}", command, err);
    return false;
  }
  return true;
}

} // namespace ipam
} // namespace ohno
