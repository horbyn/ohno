file(GLOB sources "*.cc")
add_library(ohno_helper
  STATIC
  ${sources}
)
