include(HandleLLVMOptions)

include_directories(
    ${LLVM_INCLUDE_DIRS}
    ${CLANG_INCLUDE_DIRS}
    ${CMAKE_BINARY_DIR}/include
)

add_clang_executable(find-unnecessary-includes
    main.cpp
    UnnecessaryIncludeFinder.cpp
)

target_link_libraries(find-unnecessary-includes
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

install(TARGETS find-unnecessary-includes
    RUNTIME DESTINATION bin)
