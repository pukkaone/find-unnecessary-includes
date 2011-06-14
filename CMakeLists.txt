# $Id$
cmake_minimum_required(VERSION 2.8)
project(find-unnecessary-includes)

list(APPEND CMAKE_MODULE_PATH
    ${CMAKE_SOURCE_DIR}/cmake
    ${CMAKE_SOURCE_DIR}/llvm/cmake/modules)
include(configure_files)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# For make-based builds, defines make target named test.
# For Visual Studio builds, defines Visual Studio project named RUN_TESTS.
enable_testing()

# Visual C++ Express Editions do not support solution folders.
set_property(GLOBAL PROPERTY USE_FOLDERS OFF)

option(USE_INSTALLED_CLANG "Use an installed clang/llvm?" OFF)
if(USE_INSTALLED_CLANG)
  find_package(LLVM REQUIRED)
  include(AddLLVM)
  macro(add_clang_executable name)
    add_llvm_executable( ${name} ${ARGN} )
    set_target_properties(${name} PROPERTIES FOLDER "Clang executables")
  endmacro()
  set(CLANG_INCLUDE_DIRS)
  link_directories(${LLVM_LIBRARY_DIRS})
else()
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
  set(LLVM_INCLUDE_DIRS
      ${CMAKE_BINARY_DIR}/llvm/include
      ${CMAKE_SOURCE_DIR}/llvm/include)
  set(CLANG_INCLUDE_DIRS
      ${CMAKE_BINARY_DIR}/clang/include
      ${CMAKE_SOURCE_DIR}/clang/include)
endif()
add_subdirectory(src)
add_subdirectory(test)

find_package(Git)
if(GIT_FOUND)
  # Get the version number from the git tag.
  execute_process(
      COMMAND ${GIT_EXECUTABLE} describe --abbrev=0
      OUTPUT_VARIABLE FUI_VERSION
      OUTPUT_STRIP_TRAILING_WHITESPACE
      RESULT_VARIABLE GIT_RESULT)
  if(NOT GIT_RESULT EQUAL 0)
    set(FUI_VERSION "UNKNOWN")
  endif()

  # Get the build number by counting commits from the tag named "build".
  execute_process(
      COMMAND ${GIT_EXECUTABLE} describe --match build
      OUTPUT_VARIABLE DESCRIBE_BUILD
      RESULT_VARIABLE GIT_RESULT)
  if(GIT_RESULT EQUAL 0)
    string(REGEX MATCH "[0-9]+" BUILD_NUMBER ${DESCRIBE_BUILD})
  else()
    set(BUILD_NUMBER "UNKNOWN")
  endif()
else()
  set(FUI_VERSION "UNKNOWN")
  set(BUILD_NUMBER "UNKNOWN")
endif()

# Write version and build number into version.h file.
configure_files(
    ${CMAKE_SOURCE_DIR}/template/include ${CMAKE_BINARY_DIR}/include)
