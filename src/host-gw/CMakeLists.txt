file(GLOB sources "*.cc")
add_library(ohno_hostgw
  STATIC
  ${sources}
)
