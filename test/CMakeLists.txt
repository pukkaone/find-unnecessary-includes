# $Id$

get_target_property(
    FIND_UNNECESSARY_INCLUDES_EXE find-unnecessary-includes LOCATION)
set(TEST_COMMAND ${FIND_UNNECESSARY_INCLUDES_EXE})

macro(add_compare_test inputFile)
  add_test(
      NAME ${inputFile}
      COMMAND ${CMAKE_COMMAND}
          -D "TEST_COMMAND=$<TARGET_FILE:find-unnecessary-includes>"
          -D "TEST_INPUT=${inputFile}"
          -P compare_test.cmake
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  )
endmacro()

add_compare_test(class-unused.c)
add_compare_test(class-used.c)
add_compare_test(enum-constant-unused.cpp)
add_compare_test(enum-constant-used.cpp)
add_compare_test(enum-unused.cpp)
add_compare_test(enum-used.cpp)
add_compare_test(function-unused.cpp)
add_compare_test(function-used.cpp)
add_compare_test(macro-unused.c)
add_compare_test(macro-used.c)
add_compare_test(variable-unused.cpp)
add_compare_test(variable-used.cpp)
