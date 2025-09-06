file(GLOB sources "*.cc")
add_library(ohno_cni
  STATIC
  ${sources}
)
