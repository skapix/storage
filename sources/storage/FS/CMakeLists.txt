# no find_package required

set(sources ${sources} ${CMAKE_CURRENT_SOURCE_DIR}/FSStorage.cpp CACHE STRING "" FORCE)
set(headers ${headers} ${CMAKE_CURRENT_SOURCE_DIR}/FSStorage.h CACHE STRING "" FORCE)
set(FS_files ${CMAKE_CURRENT_SOURCE_DIR}/FSStorage.cpp ${CMAKE_CURRENT_SOURCE_DIR}/FSStorage.h CACHE STRING "" FORCE)
pretty_printing("FS" "yes")