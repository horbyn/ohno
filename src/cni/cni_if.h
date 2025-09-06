#pragma once

// clang-format off
#include <string>
#include <string_view>
// clang-format on

namespace ohno {
namespace cni {

class CniIf {
public:
  virtual ~CniIf() = default;

  /**
   * @brief CNI ADD
   *
   * @note 根据 CNI spec：
   * https://github.com/containernetworking/cni.dev/blob/main/content/docs/spec.md#add-add-container-to-network-or-apply-modifications
   *
   * 成功时返回值为 0，失败时返回非零，插件遇到错误需要向 stderr 输出一个 error 的 json 格式。
   * 插件 ADD 成功在 stdout 输出一个成功的 json 格式
   *
   * 因此，接口定义返回值为 json 格式字符串，如果接口执行出错则抛出异常，由 caller
   * 负责处理返回值以及关于 error 的输出
   *
   * @param container_id 来自环境变量 CNI_CONTAINERID
   * @param netns 来自环境变量 CNI_NETNS
   * @param nic_name 来自环境变量 CNI_IFNAME
   * @return std::string 一个 json 格式字符串
   */
  virtual auto add(std::string_view container_id, std::string_view netns, std::string_view nic_name)
      -> std::string = 0;

  /**
   * @brief CNI DEL
   *
   * @note 根据 CNI spec：
   * https://github.com/containernetworking/cni.dev/blob/main/content/docs/spec.md#del-remove-container-from-network-or-un-apply-modifications
   *
   * 插件通常应该完成 DEL 操作而不会出错，即使某些资源缺失
   *
   * 因此，接口总是成功，即使遇到出错等问题（可以记录日志），所以不能抛出异常
   *
   * @param container_id 来自环境变量 CNI_CONTAINERID
   * @param nic_name 来自环境变量 CNI_IFNAME
   */
  virtual auto del(std::string_view container_id, std::string_view nic_name) noexcept -> void = 0;

  /**
   * @brief CNI VERSION
   *
   * @note CNI spec 参考：
   * https://github.com/containernetworking/cni.dev/blob/main/content/docs/spec.md#version-probe-plugin-version-support
   *
   * @return std::string 版本号 json 字符串
   */
  virtual auto version() const -> std::string = 0;
};

} // namespace cni
} // namespace ohno
