project(large_storage)
cmake_minimum_required(VERSION 3.0)

if (NOT WIN32)
  ##
	message(FATAL_ERROR "While only Windows OS is supported")
  ##
endif()


include_directories(${CMAKE_SOURCE_DIR}/sources)
include_directories(${CMAKE_SOURCE_DIR}/include)

include(settings.cmake)
add_definitions(-D_CMAKE_USED)

set(LibCurlPath ${CMAKE_SOURCE_DIR}/libs/curl-7.42.1)
set(PostgreSQLPath ${CMAKE_SOURCE_DIR}/libs/pgsql)
set(SQLitePath ${CMAKE_SOURCE_DIR}/libs/SQLite)
set(BSONPath ${CMAKE_SOURCE_DIR}/libs/mongo-c-driver)
set(MongoDBPath ${CMAKE_SOURCE_DIR}/libs/mongo-c-driver)

set(LibCurlLibPath ${LibCurlPath}/curl_DLL_ONLY/nossl) #libcurl.lib
set(PostgreSQLLibPath ${PostgreSQLPath}/lib) #libpq.lib
set(SQLiteLibPath ${SQLitePath}/sqlite-dll-win32-x86-3080701) #sqlite3.lib
set(BSONLibPath ${BSONPath}/lib) #bson-1.0.lib
set(MongoDBLibPath ${MongoDBPath}/lib) #mongoc-1.0.lib
set(GTestLibPathDebug ${GTestPath}/msvc/gtest-md/Debug) #gtest*.lib

set(LibCurlDllPath ${LibCurlPath}/curl_DLL_ONLY/nossl) #libcurl.dll
set(PostgreSQLDllPath ${PostgreSQLPath}/lib) #libpq.dll
set(SQLiteDllPath ${SQLitePath}/sqlite-dll-win32-x86-3080701) #sqlite3.dll
set(BSONDllPath ${BSONPath}/bin) #libbson-1.0.dll
set(MongoDBDllPath ${MongoDBPath}/bin) #libmongoc-1.0.dll

set(LibCurlIncludePath ${LibCurlPath}/include) #curl/curl.h
set(PostgreSQLIncludePath ${PostgreSQLPath}/include) #libpq-fe.h
set(SQLiteIncludePath ${SQLitePath}/sqlite-dll-win32-x86-3080701) #sqlite3.h
set(BSONIncludePath ${BSONPath}/include/libbson-1.0) #bcon.h
set(MongoDBIncludePath ${MongoDBPath}/include/libmongoc-1.0) #mongoc.h

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
set(copy_dll_dir ${CMAKE_SOURCE_DIR}/bin) # directory with dll files
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
  include(doxygen.cmake)
  create_documentation("" "${CMAKE_SOURCE_DIR}/sources" "${CMAKE_BINARY_DIR}/doxygen" "INPUT+=${CMAKE_SOURCE_DIR}/include")
  ##
endif()
