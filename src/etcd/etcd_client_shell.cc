// clang-format off
#include "etcd_client_shell.h"
#include <sstream>
#include "spdlog/fmt/fmt.h"
#include "src/common/assert.h"
#include "src/common/except.h"
#include "src/helper/string.h"
// clang-format on

namespace ohno {
namespace etcd {

EtcdClientShell::EtcdClientShell(const EtcdData &etcd_data, std::unique_ptr<util::ShellIf> shell,
                                 std::unique_ptr<util::EnvIf> env)
    : etcd_data_{etcd_data}, shell_{std::move(shell)}, env_{std::move(env)} {
  if (!env_->exist(ETCDCTL_VERSION)) {
    // TODO: 判断返回值之后可以怎么做？如果没有办法设置
    // ETCDCTL_API=3，只能使用默认值了，这时候命令会报错的
    env_->set(ETCDCTL_VERSION, ETCDCTL_VERSION_VALUE);
  }
  command_prefix_ =
      fmt::format("etcdctl --endpoints={} --cacert={} --cert={} --key={} ", etcd_data_.endpoints_,
                  etcd_data_.ca_cert_, etcd_data_.cert_, etcd_data_.key_);
}

EtcdClientShell::~EtcdClientShell() {
  if (env_->exist(ETCDCTL_VERSION)) {
    // TODO: 判断返回值之后可以怎么做？
    env_->unset(ETCDCTL_VERSION);
  }
}

/**
 * @brief 设置一个 ETCD key-value
 *
 * @param key ETCD key
 * @param value ETCD value
 * @return true 设置成功
 * @return false 设置失败
 */
auto EtcdClientShell::put(std::string_view key, std::string_view value) const -> bool {
  OHNO_ASSERT(!key.empty());
  OHNO_ASSERT(!value.empty());
  OHNO_ASSERT(shell_);

  std::string out{};
  return shell_->execute(fmt::format("{} put -- {} {}", command_prefix_, key, value), out);
}

/**
 * @brief 在原 ETCD key 基础上追加一个 value（除非原 key 不存在或 get 出错，此时行为等于 put）
 *
 * @param key ETCD key
 * @param value ETCD value
 * @return true 设置成功
 * @return false 设置失败
 */
auto EtcdClientShell::append(std::string_view key, std::string_view value) const -> bool {
  OHNO_ASSERT(!value.empty());

  std::string out{};
  if (get(key, out)) {
    out = out.empty() ? std::string{value} : fmt::format("{},{}", out, value);
  } else {
    // get() 出错
    out = value.data();
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
auto EtcdClientShell::get(std::string_view key, std::string &value) const -> bool {
  OHNO_ASSERT(!key.empty());
  OHNO_ASSERT(shell_);

  auto ret =
      shell_->execute(fmt::format("{} get {} --print-value-only", command_prefix_, key), value);
  if (ret) {
    if (!value.empty()) {
      // etcdctl get 输出会包含换行符
      value.erase(std::remove(value.begin(), value.end(), '\n'), value.end());
    }
  }
  return ret;
}

/**
 * @brief 获取所有 ETCD value
 *
 * @param key ETCD key
 * @param value ETCD value（返回值）
 * @return true 获取成功
 * @return false 获取失败
 */
auto EtcdClientShell::get(std::string_view key,
                          std::unordered_map<std::string, std::string> &value) const -> bool {
  OHNO_ASSERT(!key.empty());
  OHNO_ASSERT(shell_);

  std::string out{};
  auto ret = shell_->execute(fmt::format("{} get {} --prefix", command_prefix_, key), out);
  if (ret) {
    if (!out.empty()) {
      auto map = helper::split(out, '\n');
      OHNO_ASSERT(map.size() % 2 == 0);
      for (size_t i = 0; i < map.size(); i += 2) {
        value[map[i]] = map[i + 1];
      }
    }
  }
  return ret;
}

/**
 * @brief 删除一个 ETCD key-value
 *
 * @param key ETCD key
 * @return true 删除成功
 * @return false 删除失败
 */
auto EtcdClientShell::del(std::string_view key) const -> bool {
  OHNO_ASSERT(!key.empty());
  OHNO_ASSERT(shell_);

  std::string out{};
  return shell_->execute(fmt::format("{} del {}", command_prefix_, key), out);
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
auto EtcdClientShell::del(std::string_view key, std::string_view value) const -> bool {
  OHNO_ASSERT(!value.empty());

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
  return to_put.empty() ? del(key) : put(key, to_put);
}

/**
 * @brief 获取 ETCD value 列表，适用于 ETCD value 是以 ',' 分割的多个值的情况（如 foo -> bar,qux）
 *
 * @param key ETCD key
 * @param results ETCD value 列表（返回值）
 * @return true 获取成功
 * @return false 获取失败
 */
auto EtcdClientShell::list(std::string_view key, std::vector<std::string> &results) const -> bool {

  std::string output{};
  if (!get(key, output)) {
    return false;
  }

  results = helper::split(output, ',');
  return true;
}

/**
 * @brief 将 ETCD 信息全部输出出来
 *
 * @param key ETCD key
 * @return std::string 持久化结果，无结果为空
 */
auto EtcdClientShell::dump(std::string_view key) const -> std::string {
  OHNO_ASSERT(!key.empty());

  std::unordered_map<std::string, std::string> map{};
  if (get(key, map)) {
    if (!map.empty()) {
      std::string result{};
      for (auto &item : map) {
        result += fmt::format("{} -> {}\n", item.first, item.second);
      }
      return result;
    }
  }

  return std::string{};
}

} // namespace etcd
} // namespace ohno
