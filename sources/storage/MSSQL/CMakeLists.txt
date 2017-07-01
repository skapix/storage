find_package(mssql QUIET)

if(MSSQL_FOUND AND WIN32)
  set(sources ${sources} ${CMAKE_CURRENT_SOURCE_DIR}/MSSQLStorage.cpp CACHE STRING "" FORCE)
  set(headers ${headers} ${CMAKE_CURRENT_SOURCE_DIR}/MSSQLStorage.h CACHE STRING "" FORCE)
  set(include_dirs ${include_dirs} ${MSSQL_INCLUDE_DIRS} CACHE STRING "" FORCE)
  set(libs ${libs} ${MSSQL_LIBRARIES} CACHE STRING "" FORCE)
  set(compile_definitions ${compile_definitions} HAS_MSSQL CACHE STRING "" FORCE)
  set(MSSQL_files ${CMAKE_CURRENT_SOURCE_DIR}/MSSQLStorage.cpp ${CMAKE_CURRENT_SOURCE_DIR}/MSSQLStorage.h CACHE STRING "" FORCE)
  pretty_printing("MS SQL" "yes")
else(MSSQL_FOUND AND WIN32)
  pretty_printing("MS SQL" "no")
endif(MSSQL_FOUND AND WIN32)

