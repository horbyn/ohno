file(GLOB sources "*.cc")
add_library(ohno_ipam
  STATIC
  ${sources}
)
target_link_libraries(ohno_ipam
  PRIVATE
  etcd-cpp-api
)
