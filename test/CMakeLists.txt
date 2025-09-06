message(STATUS "[单元测试]: ON")
find_program(GCOVR_EXE "gcovr")
if(NOT GCOVR_EXE)
  message(FATAL_ERROR "找不到 gcovr 工具")
endif()

add_custom_target(testclean
  COMMAND ${CMAKE_COMMAND} -E rm -f
    ${CMAKE_BINARY_DIR}/**/*.gcda
  COMMENT "清理前次测试残留数据"
)

# 单元测试可执行程序需要以 "_test" 结尾
file(GLOB DIR RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} */)
foreach(sub ${DIR})
  if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${sub})
    file(GLOB TEST_SRC ${CMAKE_CURRENT_SOURCE_DIR}/${sub}/*_test.cc)
    foreach(test_src ${TEST_SRC})
      get_filename_component(test_src ${test_src} NAME_WE)
      list(APPEND TEST_EXE "${test_src}")
    endforeach()
  else()
    get_filename_component(test_src ${CMAKE_CURRENT_SOURCE_DIR}/${sub} NAME_WE)
    if("${test_src}" STREQUAL "etcdctl_test" AND NOT ENABLE_ETCDCTL_TEST)
    elseif("${test_src}" STREQUAL "CMakeLists")
    else()
      list(APPEND TEST_EXE "${test_src}")
    endif()
  endif()
endforeach()
message(STATUS "单元测试项目: ${TEST_EXE}")

setup_target_for_coverage_gcovr_html(
  NAME ${OHNO_EXPORT_TEST_TARGET}
  EXECUTABLE cd ${CMAKE_BINARY_DIR}/test && ctest -L Unittests && cd ${CMAKE_BINARY_DIR}
  DEPENDENCIES
    testclean
    ${TEST_EXE}
  BASE_DIRECTORY ${CMAKE_SOURCE_DIR}/src
)

enable_testing()
include(GoogleTest)

# 单元测试宏定义
macro(ohno_unit_test target_name)
  add_executable(
    ${target_name}
    ${target_name}.cc
  )
  target_link_libraries(${target_name}
    PRIVATE
    GTest::gtest_main
    GTest::gmock_main
    ${PROJECT_NAME}::core
  )
  gtest_discover_tests(${target_name}
    PROPERTIES LABELS "Unittests"
  )
endmacro()

add_subdirectory(ipam)
add_subdirectory(net)
add_subdirectory(util)
if(ENABLE_ETCDCTL_TEST)
  ohno_unit_test(etcdctl_test)
endif()
