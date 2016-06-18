project(${benchmark_name})
cmake_minimum_required(VERSION 2.8)

set (HEADERS ${CMAKE_SOURCE_DIR}/include/Storage.h func.h ${CMAKE_SOURCE_DIR}/sources/storage/auxiliaryStorage.h)
set(SOURCE benchmark.cpp func.cpp ${CMAKE_SOURCE_DIR}/sources/storage/auxiliaryStorage.cpp
    ${CMAKE_SOURCE_DIR}/sources/storage/StringParser.cpp)

add_executable(${benchmark_name} ${SOURCE} ${HEADERS})
set_target_properties(${benchmark_name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/)
add_dependencies(${benchmark_name} ${storage_name})