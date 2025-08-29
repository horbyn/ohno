file(GLOB sources "*.cc")
add_library(ohno_log
  STATIC
  ${sources}
)
target_link_libraries(
  ohno_log
  PRIVATE
  ohno_common
)
