file(GLOB sources "*.cc")
add_library(ohno_cni
  STATIC
  ${sources}
)
target_link_libraries(ohno_cni
  PRIVATE
  ohno_helper
  ohno_net
)
