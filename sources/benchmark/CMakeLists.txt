project(${benchmark_name})
cmake_minimum_required(VERSION 3.0)

set (HEADERS func.h ${CMAKE_SOURCE_DIR}/sources/storage/auxiliaryStorage.h)
set(SOURCE benchmark.cpp func.cpp ${CMAKE_SOURCE_DIR}/sources/storage/auxiliaryStorage.cpp
    ${CMAKE_SOURCE_DIR}/sources/storage/StringParser.cpp )

add_executable(${benchmark_name} ${SOURCE} ${HEADERS} ${main_include_files} ${utility_files})
target_link_libraries(${benchmark_name} ${storage_name})
set_target_properties(${benchmark_name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/)
add_dependencies(${benchmark_name} ${storage_name})

source_group(utilities FILES  ${utility_files})
source_group("include" FILES  ${main_include_files})