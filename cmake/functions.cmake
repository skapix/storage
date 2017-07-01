function(pretty_printing
 projectName
 result)
  string(LENGTH ${projectName} projectLength)
  math(EXPR NUM_SPACES "20 - ${projectLength}")
  string(RANDOM LENGTH ${NUM_SPACES} ALPHABET " " SPACES)
  message(STATUS "${projectName} ${SPACES}- ${result}")
endfunction()