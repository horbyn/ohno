cmake_minimum_required(VERSION 3.17)
project("ohno"
  VERSION 0.1.0
  LANGUAGES C CXX
)
message(STATUS "[版本]: ${PROJECT_NAME}_v${PROJECT_VERSION}")

# 选项
option(OHNO_STATIC_ANALYSIS "Enable static analysis" ON)
option(OHNO_MEMCHECK "Enable memory check" ON)
option(OHNO_TEST "Enable unit test" ON)
option(OHNO_BENCHMARK "Enable benchmark" ON)
option(OHNO_FLAMEGRAPH "Enable flamegraph" ON)

# 设置全局变量
set(OHNO_EXPORT_TEST_TARGET run_coverage)
set(OHNO_EXPORT_BENCHMARK_TARGET run_benchmark)
set(OHNO_EXPORT_STATIC_ANALYSIS_TARGET run_static_analysis)
set(OHNO_EXPORT_MEMCHECK_TARGET run_memcheck)
set(OHNO_EXPORT_FLAMEGRAPH_TARGET run_flamegraph)

# 编译选项
if(${CMAKE_BUILD_TYPE} MATCHES "Debug")
  message(STATUS "[编译选项]: debug")
  add_compile_options(-Wall -Wextra -O0 -g3)
else()
  message(STATUS "[编译选项]: release")
  add_compile_options(-Wall -Wextra -O2 -DNDEBUG)
endif()
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置版本
configure_file(
  ${CMAKE_SOURCE_DIR}/ohno_version.h.in
  ${CMAKE_BINARY_DIR}/ohno_version.h
  @ONLY
)

# 安装外部依赖
set(OHNO_DEPS_DIR ${CMAKE_SOURCE_DIR}/deps)
set(FETCHCONTENT_UPDATES_DISCONNECTED ON) # 避免每次 remake 时候都对依赖进行更新
include(FetchContent)
FetchContent_Declare(
  spdlog
  GIT_REPOSITORY https://github.com/gabime/spdlog.git
  GIT_TAG v1.15.1
  SOURCE_DIR   "${OHNO_DEPS_DIR}/spdlog_src"
  BINARY_DIR   "${OHNO_DEPS_DIR}/spdlog_build"
  SUBBUILD_DIR "${OHNO_DEPS_DIR}/spdlog_subbuild"
)
set(SPDLOG_INSTALL OFF CACHE BOOL "" FORCE) # 不要安装到系统目录
FetchContent_MakeAvailable(spdlog)
message(STATUS "spdlog 头文件目录: ${spdlog_SOURCE_DIR}/include")
include_directories(${spdlog_SOURCE_DIR}/include)

FetchContent_Declare(
  cli11
  GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git
  GIT_TAG v2.5.0
  SOURCE_DIR   "${OHNO_DEPS_DIR}/cli11_src"
  BINARY_DIR   "${OHNO_DEPS_DIR}/cli11_build"
  SUBBUILD_DIR "${OHNO_DEPS_DIR}/cli11_subbuild"
)
FetchContent_MakeAvailable(cli11)
message(STATUS "cli11 头文件目录: ${cli11_SOURCE_DIR}/include")
include_directories(${cli11_SOURCE_DIR}/include)

FetchContent_Declare(
  magic_enum
  GIT_REPOSITORY https://github.com/Neargye/magic_enum.git
  GIT_TAG v0.9.7
  SOURCE_DIR   "${OHNO_DEPS_DIR}/magic_enum_src"
  BINARY_DIR   "${OHNO_DEPS_DIR}/magic_enum_build"
  SUBBUILD_DIR "${OHNO_DEPS_DIR}/magic_enum_subbuild"
)
FetchContent_MakeAvailable(magic_enum)
message(STATUS "magic_enum 头文件目录: ${magic_enum_SOURCE_DIR}/include")
include_directories(${magic_enum_SOURCE_DIR}/include)

find_package(etcd-cpp-api REQUIRED)

# 引入头文件
include_directories(
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_BINARY_DIR}
)

# 单元测试
if(OHNO_TEST)
  # 下载代码覆盖率统计 cmake 模块
  set(download_file_url "https://raw.githubusercontent.com/bilke/cmake-modules/master/CodeCoverage.cmake")
  set(download_file_path "${CMAKE_SOURCE_DIR}/cmake/CodeCoverage.cmake")

  if(NOT EXISTS "${download_file_path}")
    file(DOWNLOAD ${download_file_url} ${download_file_path} SHOW_PROGRESS)
  endif()

  include(CheckCCompilerFlag)
  include(${CMAKE_SOURCE_DIR}/cmake/CodeCoverage.cmake)
  add_compile_definitions(Debug)
  APPEND_COVERAGE_COMPILER_FLAGS()

  add_subdirectory(test)
endif()

# 基准测试
if(OHNO_BENCHMARK)
  set(BENCHMARK_ENABLE_GTEST_TESTS OFF)
  add_subdirectory(benchmarks)
endif()

# 编译
add_compile_options(-Werror) # 仅作用于 ohno 项目
add_subdirectory(src)

# 接口库
add_library(${PROJECT_NAME}_core INTERFACE)
target_link_libraries(${PROJECT_NAME}_core INTERFACE
  ohno_common
  ohno_log
)
add_library(${PROJECT_NAME}::core ALIAS ${PROJECT_NAME}_core)

# 可执行文件
add_executable(${PROJECT_NAME} ohno.cc)
target_link_libraries(${PROJECT_NAME}
  PRIVATE
  ${PROJECT_NAME}::core
  etcd-cpp-api
)

# 静态检查
if(OHNO_STATIC_ANALYSIS)
  message(STATUS "[静态检查]: ON")
  set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE BOOL "Export compile_commands.json" FORCE)

  find_program(CLANG_TIDY_EXE "clang-tidy")
  if(NOT CLANG_TIDY_EXE)
    message(FATAL_ERROR "找不到 clang-tidy 工具")
  endif()

  file(GLOB_RECURSE ALL_SROUCES
    "${CMAKE_SOURCE_DIR}/ohno.cc"
    "${CMAKE_SOURCE_DIR}/src/*.h"
    "${CMAKE_SOURCE_DIR}/src/*.tpp"
    "${CMAKE_SOURCE_DIR}/src/*.cc"
  )

  add_custom_target(${OHNO_EXPORT_STATIC_ANALYSIS_TARGET}
    COMMAND ${CLANG_TIDY_EXE}
      -p ${CMAKE_BINARY_DIR}/compile_commands.json
      --config-file=${CMAKE_SOURCE_DIR}/.clang-tidy
      ${ALL_SROUCES}
    COMMENT "对项目进行静态检查"
  )
endif()

# 内存检查
if(OHNO_MEMCHECK)
  message(STATUS "[内存检查]: ON")

  find_program(VALGRIND_EXE "valgrind")
  if (NOT VALGRIND_EXE)
    message(FATAL_ERROR "找不到 valgrind 工具")
  endif()

  add_custom_target(${OHNO_EXPORT_MEMCHECK_TARGET}
    COMMAND ${VALGRIND_EXE}
      --leak-check=full
      --track-origins=yes
      --leak-check=full
      --show-leak-kinds=all
      ${CMAKE_BINARY_DIR}/${PROJECT_NAME}
    DEPENDS ${PROJECT_NAME}
    COMMENT "对项目进行内存检查"
  )
endif()

# 火焰图
if(OHNO_FLAMEGRAPH)
  message(STATUS "[火焰图集成]: ON")

  execute_process(
    COMMAND perf --version
    RESULT_VARIABLE PERF_RESULT
    OUTPUT_VARIABLE PERF_OUTPUT
    ERROR_VARIABLE PERF_ERR
  )
  if (PERF_RESULT EQUAL 0)
    string(REGEX MATCH "^perf version [0-9]+\\.[0-9]+\\.[0-9]+" PERF_VERSION ${PERF_OUTPUT})
    if (NOT PERF_VERSION)
      message(FATAL_ERROR "perf 命令损坏: ${PERF_OUTPUT}")
    endif()
  else()
    message(FATAL_ERROR "找不到 perf 命令: ${PERF_ERR}")
  endif()

  FetchContent_Declare(
    flamegraph
    GIT_REPOSITORY https://github.com/brendangregg/FlameGraph.git
    GIT_TAG v1.0
    SOURCE_DIR   "${OHNO_DEPS_DIR}/flamegraph_src"
    BINARY_DIR   "${OHNO_DEPS_DIR}/flamegraph_build"
    SUBBUILD_DIR "${OHNO_DEPS_DIR}/flamegraph_subbuild"
  )
  FetchContent_MakeAvailable(flamegraph)

  set(OHNO_FG_DIR ${CMAKE_BINARY_DIR}/${OHNO_EXPORT_FLAMEGRAPH_TARGET})
  if (NOT EXISTS ${OHNO_FG_DIR})
    file(MAKE_DIRECTORY ${OHNO_FG_DIR})
  endif()
  add_custom_target(${OHNO_EXPORT_FLAMEGRAPH_TARGET}
    COMMAND perf record -g ${CMAKE_BINARY_DIR}/${PROJECT_NAME}
    COMMAND perf script -i perf.data > perf.unfold
    COMMAND ${flamegraph_SOURCE_DIR}/stackcollapse-perf.pl perf.unfold > perf.folded
    COMMAND ${flamegraph_SOURCE_DIR}/flamegraph.pl perf.folded > ${PROJECT_NAME}.svg
    COMMAND ${CMAKE_COMMAND} -E echo "已生成火焰图: ${OHNO_FG_DIR}/${PROJECT_NAME}.svg"
    DEPENDS ${PROJECT_NAME}
    WORKING_DIRECTORY ${OHNO_FG_DIR}
    COMMENT "生成火焰图"
  )
endif()

message(STATUS "需要生成单元测试覆盖率报告，运行\n"
  "\tcmake --build <BUILD_DIR> --target ${OHNO_EXPORT_TEST_TARGET} -j <CORES>\n"
  "\t输出目录为 ${CMAKE_BINARY_DIR}/${OHNO_EXPORT_TEST_TARGET}"
)
message(STATUS "需要生成基准测试报告，运行\n"
  "\tcmake --build <BUILD_DIR> --target ${OHNO_EXPORT_BENCHMARK_TARGET} -j <CORES>\n"
  "\t输出目录为 ${CMAKE_BINARY_DIR}/${OHNO_EXPORT_BENCHMARK_TARGET}"
)
message(STATUS "需要进行代码静态分析，运行\n"
  "\tcmake --build <BUILD_DIR> --target ${OHNO_EXPORT_STATIC_ANALYSIS_TARGET}"
)
message(STATUS "需要进行检查内存使用情况，运行\n"
  "\tcmake --build <BUILD_DIR> --target ${OHNO_EXPORT_MEMCHECK_TARGET} -j <CORES>"
)
message(STATUS "需要生成火焰图，运行\n"
  "\tcmake --build <BUILD_DIR> --target ${OHNO_EXPORT_FLAMEGRAPH_TARGET} -j <CORES>\n"
  "\t输出目录为 ${CMAKE_BINARY_DIR}/${OHNO_EXPORT_FLAMEGRAPH_TARGET}"
)
