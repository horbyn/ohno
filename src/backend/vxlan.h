#pragma once

// clang-format off
#include <unordered_map>
#include "src/backend/backend.h"
#include "src/cni/storage_if.h"
// clang-format on

namespace ohno {
namespace backend {

class Vxlan : public Backend {
public:
  auto start(std::string_view node_name) -> void override;

  auto setStorage(std::unique_ptr<cni::StorageIf> storage) -> void;

protected:
  auto eventHandler(std::string_view current_node) -> void override;

private:
  std::unordered_map<std::string, backend::NodeInfo> node_cache_;
  std::unique_ptr<cni::StorageIf> storage_;
};

} // namespace backend
} // namespace ohno
