find_package(mongocxx QUIET)

if(MONGOCXX_FOUND)
  set(sources ${sources} ${CMAKE_CURRENT_SOURCE_DIR}/MongoDBStorage.cpp CACHE STRING "" FORCE)
  set(headers ${headers} ${CMAKE_CURRENT_SOURCE_DIR}/MongoDBStorage.h CACHE STRING "" FORCE)
  set(include_dirs ${include_dirs} ${LIBMONGOCXX_INCLUDE_DIRS} CACHE STRING "" FORCE)
  set(libs ${libs} ${LIBMONGOCXX_LIBRARIES} CACHE STRING "" FORCE)
  set(compile_definitions ${compile_definitions} HAS_MONGODB CACHE STRING "" FORCE)
  pretty_printing("MongoDB" "yes")
  
else(MONGOCXX_FOUND)
  pretty_printing("MongoDB" "no")
endif(MONGOCXX_FOUND)

