##### other args are options to doxyfile
function( create_documentation
 TARGET_PROJECT_NAME ### if empty -> creates while building cmake project
 SOURCE_FOLDER
 OUTPUT_FOLDER )
  
  ##### preliminary settings
  set (PF86 "ProgramFiles(x86)")
  find_program(
   DOXYGEN_PATH doxygen
   PATHS
   "$ENV{ProgramFiles}/doxygen/bin"
   "$ENV{${PF86}}/doxygen/bin"
   "$ENV{ProgramW6432}/doxygen/bin")

  if(NOT DOXYGEN_PATH)
    message(
     SEND_ERROR
     "Can't find doxygen program. Install doxygen or turn off the creation of documentation.")
  endif()


  if (NOT EXISTS ${OUTPUT_FOLDER})
    #
    file(MAKE_DIRECTORY ${OUTPUT_FOLDER})
    #
  endif()

  set (DOXYGEN_CONFIG ${OUTPUT_FOLDER}/doxyfile)

  if (NOT EXISTS ${DOXYGEN_CONFIG})
    #
    # create doxy file
    execute_process(COMMAND ${DOXYGEN_PATH} -g ${DOXYGEN_CONFIG}
      RESULT_VARIABLE RESULT)
    if(NOT RESULT EQUAL 0)
      message(
        SEND_ERROR
        "Error while creating doxygen configure file")
    endif()
    
    # set params in file
    # http://stackoverflow.com/questions/11032280/specify-doxygen-parameters-through-command-line
    # doxygen uses last found options
    set (CONFIGS "RECURSIVE=YES" "INPUT=${SOURCE_FOLDER}")
    
    foreach (config IN LISTS CONFIGS)
      file(APPEND ${DOXYGEN_CONFIG} "${config}\n")
    endforeach()
    
    #add user options
    
    foreach (config IN LISTS ARGN)
      file(APPEND ${DOXYGEN_CONFIG} "${config}\n")
    endforeach()
    
  endif()


  if (NOT TARGET_PROJECT_NAME STREQUAL "")
    add_custom_target(${TARGET_PROJECT_NAME}
     COMMAND ${DOXYGEN_PATH} ${DOXYGEN_CONFIG}
     WORKING_DIRECTORY ${OUTPUT_FOLDER})
  else()
    execute_process(COMMAND ${DOXYGEN_PATH} ${DOXYGEN_CONFIG}
     WORKING_DIRECTORY ${OUTPUT_FOLDER})
  endif()
endfunction()
