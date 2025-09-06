add_subdirectory(netlink)

file(GLOB sources "*.cc")
add_library(ohno_net
  STATIC
  ${sources}
)
target_link_libraries(ohno_net
  PRIVATE
  ohno_common
  ohno_log
  ohno_netlink
)
