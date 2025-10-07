#pragma once

// clang-format off
#include "backend_if.h"
#include <atomic>
#include <thread>
#include "src/log/logger.h"
// clang-format on

namespace ohno {
namespace backend {

constexpr int DEFAULT_INTERVAL{1};

class Backend : public BackendIf, public log::Loggable<log::Id::backend> {
public:
  auto start(std::string_view node_name) -> void override;
  auto setInterval(int sec) -> void override;
  auto setCenter(std::unique_ptr<backend::CenterIf> center) -> void override;
  auto setNic(std::unique_ptr<net::NicIf> nic) -> void override;
  auto stop() -> void override;

protected:
  auto startImpl(std::string_view node_name, std::string_view thread_name) -> void;
  virtual auto eventHandler(std::string_view current_node) -> void;

  std::atomic<bool> running_;
  std::thread monitor_;
};

} // namespace backend
} // namespace ohno
