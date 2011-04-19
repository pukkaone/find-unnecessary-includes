# $Id$
project(find-unnecessary-includes)
cmake_minimum_required(VERSION 2.8)

list(APPEND CMAKE_MODULE_PATH
    ${CMAKE_SOURCE_DIR}/cmake
    ${CMAKE_SOURCE_DIR}/llvm/cmake/modules)
include(configure_files)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if(CMAKE_GENERATOR MATCHES "Visual Studio.*")
  # Disable compiler warnings.
  add_definitions("-D_SCL_SECURE_NO_WARNINGS /wd4146 /wd4244 /wd4275 /wd4345 /wd4355 /wd4800")
endif()

# For make-based builds, defines target "test".
enable_testing()

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

find_package(Git)
if(GIT_FOUND)
  # Get the version number from the git tag.
  execute_process(
      COMMAND ${GIT_EXECUTABLE} describe --abbrev=0
      OUTPUT_VARIABLE FUI_VERSION
      OUTPUT_STRIP_TRAILING_WHITESPACE)

  # Get the build number by counting commits from the first commit in the
  # repository which is tagged "build".
  execute_process(
      COMMAND ${GIT_EXECUTABLE} describe --match build
      OUTPUT_VARIABLE DESCRIBE_BUILD
      OUTPUT_STRIP_TRAILING_WHITESPACE)
  string(REGEX MATCH "[0-9]+" BUILD_NUMBER ${DESCRIBE_BUILD})
else()
  set(FUI_VERSION "0.0")
  set(BUILD_NUMBER 0)
endif()

# Write version and build number into version.h file.
configure_files(
    ${CMAKE_SOURCE_DIR}/template/include ${CMAKE_BINARY_DIR}/include)
