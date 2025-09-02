file(GLOB sources "*.cc")
add_library(ohno_util
  STATIC
  ${sources}
)
