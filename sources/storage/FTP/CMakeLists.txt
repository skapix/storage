find_package(CURL QUIET)

if(CURL_FOUND)
  set(sources ${sources} ${CMAKE_CURRENT_SOURCE_DIR}/FTPStorage.cpp CACHE STRING "" FORCE)
  set(headers ${headers} ${CMAKE_CURRENT_SOURCE_DIR}/FTPStorage.h CACHE STRING "" FORCE)
  set(include_dirs ${include_dirs} ${CURL_INCLUDE_DIRS} CACHE STRING "" FORCE)
  set(libs ${libs} ${CURL_LIBRARIES} CACHE STRING "" FORCE)
  set(compile_definitions ${compile_definitions} HAS_CURL CACHE STRING "" FORCE)
  pretty_printing("CURL (ftp)" "yes")
else(CURL_FOUND)
  pretty_printing("CURL (ftp)" "no")
endif(CURL_FOUND)

