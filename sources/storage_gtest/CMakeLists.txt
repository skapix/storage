project(${test_name})
cmake_minimum_required(VERSION 2.8)

set (HEADERS tests.h auxiliary.h)
set(SOURCE storage_gtest.cpp tests.cpp auxiliary.cpp)

add_executable(${test_name} ${SOURCE} ${HEADERS} ${main_include_files} ${utility_files})
target_link_libraries(${test_name} ${GTEST_BOTH_LIBRARIES} ${storage_name})
set_target_properties(${test_name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/)
add_dependencies(${test_name} ${storage_name})

source_group(utilities FILES  ${utility_files})
source_group("include" FILES  ${main_include_files})