// clang-format off
#include <cstdlib>
#include <exception>
#include <iostream>
#include <memory> ///
#include "etcd/Client.hpp"
#include "src/common/except.h"
#include "src/log/logger.h"
// clang-format on

auto main(int argc, char **argv) -> int {
  using namespace ohno;
  (void)argc;
  (void)argv;

  try {
    static std::string ca = "/etc/etcd/pki/ca.pem";
    static std::string cert = "/etc/etcd/pki/etcd.pem";
    static std::string key = "/etc/etcd/pki/etcd-key.pem";
    static const std::string etcd_url = etcdv3::detail::resolve_etcd_endpoints(
        "https://etcd1:2379,https://etcd2:2379,https://etcd3:2379");

    OHNO_GLOBAL_LOG(info, "启动 ohno");
    std::unique_ptr<etcd::Client> etcd{};
    etcd.reset(etcd::Client::WithSSL(etcd_url, ca, cert, key));
    if (etcd == nullptr) {
      throw OHNO_EXCEPT("fail to create etcd client", false);
    }

    // pplx::task 对象负责接收 HTTP 响应并解析 gRPC 响应
    pplx::task<etcd::Response> response_task = etcd->get("foo");
    etcd::Response resp = response_task.get();
    if (resp.is_ok()) {
      OHNO_GLOBAL_LOG(info, "get foo: {}", resp.value().as_string());
    } else {
      OHNO_GLOBAL_LOG(error, "fail to get foo, error code {} with {}", resp.error_code(),
                      resp.error_message());
    }

  } catch (const ohno::except::Success &suc) {
    return EXIT_SUCCESS;
  } catch (const ohno::except::Exception &exc) {
    std::cout << "[error] " << exc.getMsg() << "\n";
    return EXIT_FAILURE;
  } catch (const std::exception &exc) {
    std::cout << "[error] " << exc.what() << "\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
