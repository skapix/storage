project(${storage_name})
cmake_minimum_required(VERSION 3.0)
include(GenerateExportHeader)

set (HEADERS auxiliaryStorage.h interface_impl.h StringParser.h)

 
set(SOURCE Storage.cpp auxiliaryStorage.cpp interface_impl.cpp
 FSStorage/FSStorage.cpp
 StringParser.cpp
 SQLiteStorage/SQLiteStorage.cpp
 FTPStorage/FTPStorage.cpp
 PostgreSQL/PostgreSQL.cpp
 SMBStorage/SMBStorage.cpp
 MsSQLStorage/MsSQLStorage.cpp
 MongoDB/MongoDB.cpp)

#set(PostgreSQL_LIBRARIES "${PostgreSQLLibPath}/libeay32.lib" "${PostgreSQLLibPath}/libecpg.lib"
#"${PostgreSQLLibPath}/libecpg_compat.lib" "${PostgreSQLLibPath}/libpgcommon.lib"
#"${PostgreSQLLibPath}/libpgport.lib" "${PostgreSQLLibPath}/libpgtypes.lib" "${PostgreSQLLibPath}/libpq.lib")
#install

ADD_LIBRARY (libcurlT SHARED IMPORTED)
SET_PROPERTY(TARGET libcurlT PROPERTY IMPORTED_LOCATION ${LibCurlDllPath}/libcurl.dll) # TODO: it works?
SET_PROPERTY(TARGET libcurlT PROPERTY IMPORTED_IMPLIB ${LibCurlLibPath}/libcurl.lib)

ADD_LIBRARY (sqliteT SHARED IMPORTED)
SET_PROPERTY(TARGET sqliteT PROPERTY IMPORTED_LOCATION ${SQLiteDllPath}/sqlite3.dll)
SET_PROPERTY(TARGET sqliteT PROPERTY IMPORTED_IMPLIB ${SQLiteLibPath}/sqlite3.lib)


ADD_LIBRARY (PostgreSQLT SHARED IMPORTED)
SET_PROPERTY(TARGET PostgreSQLT PROPERTY IMPORTED_LOCATION ${PostgreSQLDllPath}/libpq.dll)
SET_PROPERTY(TARGET PostgreSQLT PROPERTY IMPORTED_IMPLIB ${PostgreSQLLibPath}/libpq.lib)#${PostgreSQL_LIBRARIES})


ADD_LIBRARY (BSONT SHARED IMPORTED)
SET_PROPERTY(TARGET BSONT PROPERTY IMPORTED_LOCATION ${MongoDBDllPath}/libbson-1.0.dll)
SET_PROPERTY(TARGET BSONT PROPERTY IMPORTED_IMPLIB ${MongoDBLibPath}/bson-1.0.lib)


ADD_LIBRARY (MongoDBT SHARED IMPORTED)
SET_PROPERTY(TARGET MongoDBT PROPERTY IMPORTED_LOCATION ${MongoDBDllPath}/libmongoc-1.0.dll)
SET_PROPERTY(TARGET MongoDBT PROPERTY IMPORTED_IMPLIB ${MongoDBLibPath}/mongoc-1.0.lib)


set (ADDITIONAL_LIBRARIES libcurlT sqliteT PostgreSQLT odbc32.lib odbccp32.lib Mpr.lib BSONT MongoDBT)

add_library(${storage_name} SHARED ${SOURCE} ${HEADERS} ${main_include_files} ${utility_files})


target_link_libraries(${storage_name} ${ADDITIONAL_LIBRARIES})
set_target_properties(${storage_name} PROPERTIES
  ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
  LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)


source_group(utilities FILES  ${utility_files})
source_group("include" FILES  ${main_include_files})

generate_export_header(${storage_name} EXPORT_FILE_NAME storage_exports.h)
target_include_directories(${storage_name} PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

add_custom_command(TARGET ${storage_name} POST_BUILD
COMMAND ${CMAKE_COMMAND} -E copy_directory "${copy_dll_dir}" "${CMAKE_BINARY_DIR}/bin")