project(${storage_name})
cmake_minimum_required(VERSION 2.8)

set (HEADERS ${CMAKE_SOURCE_DIR}/include/Storage.h auxiliaryStorage.h
 interface_impl.h StringParser.h registry_func.h)
set(SOURCE Storage.cpp auxiliaryStorage.cpp interface_impl.cpp registry_func.cpp
 FlatStorage/FlatStorage.cpp
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
SET_PROPERTY(TARGET libcurlT PROPERTY IMPORTED_LOCATION ${LibCurlDllPath}/libcurl.dll)#does it work?
SET_PROPERTY(TARGET libcurlT PROPERTY IMPORTED_IMPLIB ${LibCurlLibPath}/libcurl.lib)

ADD_LIBRARY (sqliteT SHARED IMPORTED)
SET_PROPERTY(TARGET sqliteT PROPERTY IMPORTED_LOCATION ${SQLiteDllPath}/sqlite3.dll)#does it work?
SET_PROPERTY(TARGET sqliteT PROPERTY IMPORTED_IMPLIB ${SQLiteLibPath}/sqlite3.lib)


ADD_LIBRARY (PostgreSQLT SHARED IMPORTED)
SET_PROPERTY(TARGET PostgreSQLT PROPERTY IMPORTED_LOCATION ${PostgreSQLDllPath}/libpq.dll)#does it work?
SET_PROPERTY(TARGET PostgreSQLT PROPERTY IMPORTED_IMPLIB ${PostgreSQLLibPath}/libpq.lib)#${PostgreSQL_LIBRARIES})


ADD_LIBRARY (BSONT SHARED IMPORTED)
SET_PROPERTY(TARGET BSONT PROPERTY IMPORTED_LOCATION ${MongoDBDllPath}/libbson-1.0.dll)#does it work?
SET_PROPERTY(TARGET BSONT PROPERTY IMPORTED_IMPLIB ${MongoDBLibPath}/bson-1.0.lib)


ADD_LIBRARY (MongoDBT SHARED IMPORTED)
SET_PROPERTY(TARGET MongoDBT PROPERTY IMPORTED_LOCATION ${MongoDBDllPath}/libmongoc-1.0.dll)#does it work?
SET_PROPERTY(TARGET MongoDBT PROPERTY IMPORTED_IMPLIB ${MongoDBLibPath}/mongoc-1.0.lib)


set (ADDITIONAL_LIBRARIES libcurlT sqliteT PostgreSQLT odbc32.lib odbccp32.lib Mpr.lib BSONT MongoDBT)

add_definitions(-D_STORAGE_WITH_EXPORT_BUILD_DLL)
add_library(${storage_name} SHARED ${SOURCE} ${HEADERS})

target_link_libraries(${storage_name} ${ADDITIONAL_LIBRARIES})
set_target_properties(${storage_name} PROPERTIES LINK_FLAGS "/DEF:\"${CMAKE_SOURCE_DIR}/sources/storage/storage.def\"") 
set_target_properties(${storage_name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/)


add_custom_command(TARGET ${storage_name} POST_BUILD
COMMAND ${CMAKE_COMMAND} -E copy_directory "${copy_dll_dir}" "${CMAKE_BINARY_DIR}/bin")