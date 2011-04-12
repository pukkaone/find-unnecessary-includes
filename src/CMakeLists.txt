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
