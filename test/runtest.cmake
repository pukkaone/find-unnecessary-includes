# $Id$

set(TEST_EXPECTED "${TEST_INPUT}-expected")
set(TEST_ACTUAL "${TEST_INPUT}-actual")

# Run test command, capturing standard output.
execute_process(
    COMMAND ${TEST_COMMAND} ${TEST_INPUT}
    OUTPUT_FILE ${TEST_ACTUAL}
)

# Compare actual output with expected output.
execute_process(
    COMMAND ${CMAKE_COMMAND} -E compare_files ${TEST_EXPECTED} ${TEST_ACTUAL}
    RESULT_VARIABLE TEST_RESULT
)

if(TEST_RESULT)
  message(FATAL_ERROR "Failed for input ${TEST_INPUT}: did not produce expected output")
endif()
