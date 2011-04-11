# $Id$
project(find-unnecessary-includes)
cmake_minimum_required(VERSION 2.8)
enable_testing()

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Visual C++ 2008 Express Edition does not support solution folders.
set_property(GLOBAL PROPERTY USE_FOLDERS OFF)

# Patch LLVM CMake script to set USE_FOLDERS property to OFF.
configure_file(
    patch/llvm/CMakeLists.txt
    ${CMAKE_SOURCE_DIR}/llvm/CMakeLists.txt
    COPYONLY)
add_subdirectory(llvm)

set(CLANG_BUILD_STANDALONE 1
    CACHE INTERNAL "Build clang as standalone.")
set(CLANG_PATH_TO_LLVM_SOURCE "${CMAKE_SOURCE_DIR}/llvm"
    CACHE INTERNAL "Path to LLVM source directory.")
set(CLANG_PATH_TO_LLVM_BUILD "${CMAKE_BINARY_DIR}/llvm"
    CACHE INTERNAL "Path to LLVM build directory.")

# Patch clang CMake script to build it standalone.
configure_file(
    patch/clang/CMakeLists.txt
    ${CMAKE_SOURCE_DIR}/clang/CMakeLists.txt
    COPYONLY)
add_subdirectory(clang)

add_subdirectory(src)
add_subdirectory(test)
