file(GLOB sources "*.cc")
add_executable(ohnod ${sources})
target_link_libraries(ohnod
  PRIVATE
  ${PROJECT_NAME}::core
)
