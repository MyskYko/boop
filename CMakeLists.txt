cmake_minimum_required(VERSION 3.16)

project(boop LANGUAGES C CXX)

option(BOOP_BUILD_CLI "Build the boop command line executable" ON)
option(BOOP_USE_THREADS "Build boop with thread support" OFF)
option(BOOP_USE_ABC "Build boop with ABC integration" OFF)

set(BOOP_ABC_LIBRARIES "" CACHE STRING "Libraries needed for ABC integration")

add_library(boop_headers INTERFACE)
add_library(boop::headers ALIAS boop_headers)
target_compile_features(boop_headers INTERFACE cxx_std_20)
target_include_directories(boop_headers INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

if(BOOP_USE_THREADS)
  find_package(Threads REQUIRED)
  target_compile_definitions(boop_headers INTERFACE BOOP_USE_THREADS)
  target_link_libraries(boop_headers INTERFACE Threads::Threads)
endif()

if(BOOP_USE_ABC)
  target_compile_definitions(boop_headers INTERFACE BOOP_USE_ABC)
endif()

set(ARGPARSE_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ARGPARSE_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
set(ARGPARSE_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory(third_party/argparse)

add_subdirectory(third_party/cadical)

if(BOOP_BUILD_CLI)
  add_executable(boop src/main.cpp)
  target_link_libraries(boop PRIVATE
    boop::headers
    argparse::argparse
    cadical
  )
  if(BOOP_USE_ABC)
    target_link_libraries(boop PRIVATE ${BOOP_ABC_LIBRARIES})
  endif()
endif()
