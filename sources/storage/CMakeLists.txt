project(${storage_name})
cmake_minimum_required(VERSION 3.0)
include(GenerateExportHeader)

set (headers auxiliaryStorage.h export_impl.h StringParser.h CACHE STRING "")


set(sources Storage.cpp auxiliaryStorage.cpp export_impl.cpp StringParser.cpp CACHE STRING "")
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/FS)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/SMB)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/FTP)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/MSSQL)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/SQLite3)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/PostgreSQL)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/MongoDB)



add_library(${storage_name} SHARED ${sources} ${headers} ${main_include_files} ${utility_files})

set_target_properties(${storage_name} PROPERTIES
  ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
  LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)


source_group(utilities FILES  ${utility_files})
source_group("include" FILES  ${main_include_files})
source_group("storage\\FS" FILES ${FS_files})
source_group("storage\\SMB" FILES ${SMB_files})
source_group("storage\\FTP" FILES ${FTP_files})
source_group("storage\\MSSQL" FILES ${MSSQL_files})
source_group("storage\\SQLite3" FILES ${SQLite3_files})
source_group("storage\\PostgreSQL" FILES ${PostgreSQL_files})
source_group("storage\\MongoDB" FILES ${MongoDB_files})



source_group("storage\\FTP" FILES ${FTP_files})



generate_export_header(${storage_name} EXPORT_FILE_NAME storage_exports.h)

target_compile_definitions(${storage_name} PRIVATE ${compile_definitions})
target_link_libraries(${storage_name} ${libs})
target_include_directories(${storage_name} PUBLIC ${CMAKE_CURRENT_BINARY_DIR} ${include_dirs})
