# $Id$

set(LLVM_CONFIG_EXE
    ${CLANG_PATH_TO_LLVM_BUILD}/bin/${CMAKE_CFG_INTDIR}/llvm-config)

execute_process(
    COMMAND ${LLVM_CONFIG_EXE} --cxxflags
    OUTPUT_VARIABLE LLVM_CONFIG_CXXFLAGS)

add_definitions(${LLVM_CONFIG_CXXFLAGS})

include_directories(
    ${CMAKE_BINARY_DIR}/llvm/include
    ${CMAKE_SOURCE_DIR}/llvm/include
    ${CMAKE_BINARY_DIR}/clang/include
    ${CMAKE_SOURCE_DIR}/clang/include
    ${CMAKE_BINARY_DIR}/include
)

set(LLVM_USED_LIBS
    clangFrontend
    clangDriver
    clangSema
    clangAnalysis
    clangAST
    clangParse
    clangLex
    clangBasic
)

add_clang_executable(find-unnecessary-includes
    main.cpp
    UnusedHeaderFinder.cpp
)

set(FUI_OPTIONS "")
get_directory_property(DEFINES COMPILE_DEFINITIONS)
foreach(d ${DEFINES})
  list(APPEND FUI_OPTIONS ${d})
endforeach()

get_directory_property(INCLUDES INCLUDE_DIRECTORIES)
foreach(d ${INCLUDES})
  list(APPEND FUI_OPTIONS "-I${d}")
endforeach()

add_custom_target(fui
    COMMAND find-unnecessary-includes ${FUI_OPTIONS} main.cpp
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
