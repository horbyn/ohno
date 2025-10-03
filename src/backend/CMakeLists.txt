add_subdirectory(host-gw)

file(GLOB sources "*.cc")
add_library(ohno_backend
  STATIC
  ${sources}
)
