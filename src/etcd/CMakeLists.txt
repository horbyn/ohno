file(GLOB sources "*.cc")
add_library(ohno_etcd
  STATIC
  ${sources}
)
