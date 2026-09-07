get_filename_component(OUTPUT_DIR "${OUTPUT_PATH}" DIRECTORY)
file(MAKE_DIRECTORY "${OUTPUT_DIR}")

execute_process(
  COMMAND "${FAST_EXECUTABLE}" "${INPUT_PATH}" "${OUTPUT_PATH}"
  RESULT_VARIABLE FAST_RESULT
)
if(NOT FAST_RESULT EQUAL 0)
  message(FATAL_ERROR "${TEST_NAME}: fast failed with exit code ${FAST_RESULT}")
endif()

if(DEFINED ABC_EXECUTABLE)
  execute_process(
    COMMAND "${ABC_EXECUTABLE}" -c "cec -n ${INPUT_PATH} ${OUTPUT_PATH}"
    RESULT_VARIABLE CEC_RESULT
    OUTPUT_VARIABLE CEC_OUTPUT
    ERROR_VARIABLE CEC_ERROR
  )
  if(NOT CEC_RESULT EQUAL 0)
    message(FATAL_ERROR "${TEST_NAME}: ABC cec failed with exit code ${CEC_RESULT}\n${CEC_OUTPUT}\n${CEC_ERROR}")
  endif()
  if(NOT CEC_OUTPUT MATCHES "Networks are equivalent\\.")
    message(FATAL_ERROR "${TEST_NAME}: ABC cec did not prove equivalence\n${CEC_OUTPUT}\n${CEC_ERROR}")
  endif()
endif()
