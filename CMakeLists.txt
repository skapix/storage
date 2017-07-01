project(large_storage)
cmake_minimum_required(VERSION 3.0)

if (NOT WIN32)
	message(FATAL_ERROR "Currently only Windows OS is supported")
endif()


include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/sources)


include(cmake/settings.cmake)
include(cmake/functions.cmake)
add_definitions(-D_CMAKE_USED)

set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)

if (${BUILD_TESTS})
  find_package(GTest REQUIRED)
endif()

find_package(Boost COMPONENTS serialization REQUIRED)
include_directories(${Boost_INCLUDE_DIR})
add_definitions( "-DHAS_BOOST" )

link_directories(${PostgreSQLLibPath})
include_directories("${CMAKE_SOURCE_DIR}/include/"  
	${SQLiteIncludePath} ${LibCurlIncludePath}
	${PostgreSQLIncludePath} ${MsSQLIncludePath} ${BSONIncludePath} ${MongoDBIncludePath}
  ${GTEST_INCLUDE_DIRS})

set(main_include_files ${CMAKE_SOURCE_DIR}/include/Storage.h ${CMAKE_SOURCE_DIR}/include/ErrorCode.h)

file(GLOB utility_files
    "${CMAKE_SOURCE_DIR}/sources/utilities/*.h"
    "${CMAKE_SOURCE_DIR}/sources/utilities/*.cpp"
)

set(storage_name storage)
add_subdirectory(sources/storage)

set(benchmark_name benchmark)
add_subdirectory(sources/benchmark)

if (${BUILD_TESTS})
  ##
  set(test_name storage_gtest)
  add_subdirectory(sources/storage_gtest)
  ##
endif()


if (${CREATE_DOC})
  ##
  include(cmake/doxygen.cmake)
  create_documentation("" "${CMAKE_SOURCE_DIR}/sources" "${CMAKE_BINARY_DIR}/doxygen" "INPUT+=${CMAKE_SOURCE_DIR}/include")
  ##
endif()
