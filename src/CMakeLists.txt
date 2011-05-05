# $Id$

include(HandleLLVMOptions)

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
    UnnecessaryIncludeFinder.cpp
)
