file(GLOB sources "*.cc")
add_library(ohno_netlink
  STATIC
  ${sources}
)
