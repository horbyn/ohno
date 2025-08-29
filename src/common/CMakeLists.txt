file(GLOB sources "*.cc")
add_library(ohno_common
  STATIC
  ${sources}
)
