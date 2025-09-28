file(GLOB sources "*.cc")
add_library(ohno_kube
  STATIC
  ${sources}
)
