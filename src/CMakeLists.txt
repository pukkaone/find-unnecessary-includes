# $Id$

include_directories(
    ${CMAKE_BINARY_DIR}/llvm/include
    ${CMAKE_BINARY_DIR}/clang/include
    ${CMAKE_SOURCE_DIR}/llvm/include
    ${CMAKE_SOURCE_DIR}/clang/include
)

set(LLVM_USED_LIBS
    clangAnalysis
    clangAST
    clangBasic
    clangDriver
    clangFrontend
    clangLex
    clangParse
    clangSema
)

add_clang_executable(find-unnecessary-includes
    main.cpp
    TranslationUnitReader.cpp
    UnusedHeaderFinder.cpp
)
