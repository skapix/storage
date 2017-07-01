find_package(PostgreSQL QUIET)

if(POSTGRESQL_FOUND) #PostgreSQL_VERSION_STRING
  set(sources ${sources} ${CMAKE_CURRENT_SOURCE_DIR}/PostgreSQLStorage.cpp CACHE STRING "" FORCE)
  set(headers ${headers} ${CMAKE_CURRENT_SOURCE_DIR}/PostgreSQLStorage.h CACHE STRING "" FORCE)
  set(include_dirs ${include_dirs} ${PostgreSQL_INCLUDE_DIRS} CACHE STRING "" FORCE)
  set(libs ${libs} ${PostgreSQL_LIBRARIES} CACHE STRING "" FORCE)
  set(compile_definitions ${compile_definitions} HAS_POSTGRESQL CACHE STRING "" FORCE)
  set(POSTGRESQL_files ${CMAKE_CURRENT_SOURCE_DIR}/PostgreSQLStorage.cpp ${CMAKE_CURRENT_SOURCE_DIR}/PostgreSQLStorage.h CACHE STRING "" FORCE)
  pretty_printing("PostgreSQL" "yes")
else(POSTGRESQL_FOUND)
  pretty_printing("PostgreSQL" "no")
endif(POSTGRESQL_FOUND)
