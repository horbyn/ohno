message(STATUS "[基准测试]: ON")
FetchContent_Declare(
  benchmark
  GIT_REPOSITORY https://github.com/google/benchmark.git
  GIT_TAG v1.9.2
  SOURCE_DIR   "${OHNO_DEPS_DIR}/benchmark_src"
  BINARY_DIR   "${OHNO_DEPS_DIR}/benchmark_build"
  SUBBUILD_DIR "${OHNO_DEPS_DIR}/benchmark_subbuild"
)
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "Enable testing of the benchmark library." FORCE)
FetchContent_MakeAvailable(benchmark)
message(STATUS "benchmack 头文件目录: ${benchmark_SOURCE_DIR}/include")
include_directories(${benchmark_SOURCE_DIR}/include)

# 基准测试可执行程序需要以 "_bm" 结尾
file(GLOB DIR RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} */)
foreach(sub ${DIR})
  if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${sub})
    file(GLOB DIR_L2 ${CMAKE_CURRENT_SOURCE_DIR}/${sub}/*)
    foreach(sub_l2 ${DIR_L2})
      if(IS_DIRECTORY ${sub_l2})
        file(GLOB TEST_SRC ${sub_l2}/*_bm.cc)

        foreach(test_src ${TEST_SRC})
          get_filename_component(test_name ${test_src} NAME_WE)
          list(APPEND BENCHMARK_EXE "${test_name}")
        endforeach()
      endif()
    endforeach()
  endif()
endforeach()
message(STATUS "基准测试项目: ${BENCHMARK_EXE}")

add_custom_target(${OHNO_EXPORT_BENCHMARK_TARGET}
  COMMAND cd ${CMAKE_BINARY_DIR}/benchmarks
  COMMAND ctest -L Benchmarks --verbose
  DEPENDS ${BENCHMARK_EXE}
  COMMENT "运行基准测试"
)

# 基准测试宏定义
macro(ohno_benchmark_test target_name)
  file(GLOB sources "*.cc")
  add_executable(
    ${target_name}
    ${sources}
  )
  target_link_libraries(${target_name}
    PRIVATE
    benchmark::benchmark
    GTest::gtest_main
    GTest::gmock_main
    ${PROJECT_NAME}::core
  )

  target_compile_options(${target_name}
    PRIVATE
    -O3     # 启用最大优化
  )

  add_test(
    NAME ${target_name}
    COMMAND ${target_name}
      --benchmark_time_unit=ns
      --benchmark_format=console
  )

  set_property(
    TEST ${target_name}
    APPEND PROPERTY LABELS "Benchmarks"
  )
endmacro()

enable_testing()

add_subdirectory(net)
