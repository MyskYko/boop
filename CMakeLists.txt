cmake_minimum_required(VERSION 3.16)

project(boop LANGUAGES C CXX)

option(BOOP_BUILD_PROGRAMS "Build boop programs" ON)
option(BOOP_USE_THREADS "Build boop with thread support" OFF)
option(BOOP_USE_ABC "Build boop with ABC integration" ON)
option(BOOP_USE_MOCKTURTLE "Build boop with mockturtle integration" ON)
option(BOOP_USE_ARGPARSE "Build argv parsing support with argparse" ON)
option(BOOP_USE_CADICAL_IN_ABC "Use the Cadical copy bundled in ABC" OFF)

if(BOOP_USE_CADICAL_IN_ABC AND NOT BOOP_USE_ABC)
  message(FATAL_ERROR "BOOP_USE_CADICAL_IN_ABC requires BOOP_USE_ABC")
endif()

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
  set(ABC_USE_NAMESPACE boop_abc CACHE STRING "Namespace for ABC integration" FORCE)
  set(ABC_SKIP_TESTS ON CACHE BOOL "" FORCE)
  add_subdirectory(third_party/abc)
  get_target_property(BOOP_LIBABC_INTERFACE_OPTIONS libabc INTERFACE_COMPILE_OPTIONS)
  if(BOOP_LIBABC_INTERFACE_OPTIONS)
    list(FILTER BOOP_LIBABC_INTERFACE_OPTIONS EXCLUDE REGEX "fno-exceptions")
    set_target_properties(libabc PROPERTIES INTERFACE_COMPILE_OPTIONS "${BOOP_LIBABC_INTERFACE_OPTIONS}")
  endif()
  target_compile_definitions(boop_headers INTERFACE BOOP_USE_ABC)
  target_link_libraries(boop_headers INTERFACE
    libabc
  )
endif()

if(BOOP_USE_MOCKTURTLE)
  set(MOCKTURTLE_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  set(MOCKTURTLE_BUILD_TESTS OFF CACHE BOOL "" FORCE)
  set(MOCKTURTLE_BUILD_EXPERIMENTS OFF CACHE BOOL "" FORCE)
  add_subdirectory(third_party/mockturtle)
endif()

if(BOOP_USE_ARGPARSE)
  target_compile_definitions(boop_headers INTERFACE BOOP_USE_ARGPARSE)
  add_subdirectory(third_party/argparse)
  target_link_libraries(boop_headers INTERFACE
    argparse::argparse
  )
endif()

if(BOOP_USE_CADICAL_IN_ABC)
  target_compile_definitions(boop_headers INTERFACE BOOP_USE_CADICAL_IN_ABC)
else()
  add_subdirectory(third_party/cadical)
  target_link_libraries(boop_headers INTERFACE
    cadical
  )
endif()

add_library(boop_mockturtle_interface src/interface/mockturtle_interface.cpp)
add_library(boop::mockturtle_interface ALIAS boop_mockturtle_interface)
target_link_libraries(boop_mockturtle_interface PUBLIC
  boop::headers
)
if(BOOP_USE_MOCKTURTLE)
  target_compile_definitions(boop_mockturtle_interface PRIVATE BOOP_USE_MOCKTURTLE)
  target_link_libraries(boop_mockturtle_interface PRIVATE
    mockturtle
  )
endif()

file(GLOB BOOP_API_SOURCES CONFIGURE_DEPENDS src/api/*.cpp)
foreach(BOOP_API_SOURCE ${BOOP_API_SOURCES})
  get_filename_component(BOOP_API_NAME ${BOOP_API_SOURCE} NAME_WE)
  add_library(boop_${BOOP_API_NAME} ${BOOP_API_SOURCE})
  add_library(boop::${BOOP_API_NAME} ALIAS boop_${BOOP_API_NAME})
  target_link_libraries(boop_${BOOP_API_NAME} PUBLIC
    boop::headers
  )
endforeach()

target_link_libraries(boop_helo PUBLIC
  boop::mockturtle_interface
)

if(BOOP_BUILD_PROGRAMS)
  add_executable(helo programs/helo.cpp)
  target_link_libraries(helo PRIVATE
    boop::helo
  )
endif()
