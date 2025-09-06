// clang-format off
#include "netns.h"
#include "src/common/assert.h"
// clang-format on

namespace ohno {
namespace ipam {

/**
 * @brief 向网络空间中添加网络接口
 *
 * @param nic 网络接口对象
 */
auto Netns::addNic(std::shared_ptr<net::NicIf> nic) -> void { nic_.emplace(nic->getName(), nic); }

/**
 * @brief 从网络空间中删除网络接口
 *
 * @param nic_name 网络接口名称，不能为空
 */
auto Netns::delNic(std::string_view nic_name) -> void {
  OHNO_ASSERT(!nic_name.empty());

  auto iter = nic_.find(nic_name.data());
  if (iter != nic_.end()) {
    nic_.erase(iter);
  }
}

/**
 * @brief 根据网络接口名称获取网络接口对象
 *
 * @param nic_name 网络接口名称
 * @return std::shared_ptr<net::NicIf> 网络接口对象，不存在返回空指针
 */
auto Netns::getNic(std::string_view nic_name) const -> std::shared_ptr<net::NicIf> {
  OHNO_ASSERT(!nic_name.empty());

  auto iter = nic_.find(nic_name.data());
  if (iter == nic_.end()) {
    OHNO_LOG(error, "NIC {} not found", nic_name);
    return nullptr;
  }
  return iter->second;
}

} // namespace ipam
} // namespace ohno
