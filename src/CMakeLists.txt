# $Id$

include(HandleLLVMOptions)

include_directories(
    ${LLVM_INCLUDE_DIRS}
    ${CLANG_INCLUDE_DIRS}
    ${CMAKE_BINARY_DIR}/include
)

set(LLVM_USED_LIBS
    clangFrontend
    clangSerialization
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

install(TARGETS find-unnecessary-includes
    RUNTIME DESTINATION bin)
