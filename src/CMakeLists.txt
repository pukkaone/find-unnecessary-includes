# $Id$

include(HandleLLVMOptions)

include_directories(
    ${LLVM_INCLUDE_DIRS}
    ${CLANG_INCLUDE_DIRS}
    ${CMAKE_BINARY_DIR}/include
)

set_source_files_properties(
  ${CMAKE_BINARY_DIR}/clang/include/clang/Basic/DiagnosticCommonKinds.inc
  PROPERTIES HEADER_FILE_ONLY TRUE)

# HACK: we depend on some dyanamically generated headers - ensure they are build
# before our own source files our compiled.
set_source_files_properties(
  main.cpp
  OBJECT_DEPENDS ${CMAKE_BINARY_DIR}/clang/include/clang/Basic/DiagnosticCommonKinds.inc)
set_source_files_properties(
  UnnecessaryIncludeFinder.cpp
  OBJECT_DEPENDS ${CMAKE_BINARY_DIR}/clang/include/clang/Basic/DiagnosticCommonKinds.inc)

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
