project(${test_name})
cmake_minimum_required(VERSION 2.8)

set (HEADERS ${CMAKE_SOURCE_DIR}/include/Storage.h tests.h auxiliary.h)
set(SOURCE storage_gtest.cpp tests.cpp auxiliary.cpp)

add_executable(${test_name} ${SOURCE} ${HEADERS})
set_target_properties(${test_name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/)

target_link_libraries(${test_name} ${GTEST_BOTH_LIBRARIES})