cmake_minimum_required(VERSION 3.20)

project(res3)

enable_language(C CXX ASM)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Copies all binaries to `build/ninja`
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY 
  ${CMAKE_BINARY_DIR}
)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY 
  ${CMAKE_BINARY_DIR}
)


# Helpful utility macros
include("scripts/cmake/Utils.cmake")

find_package(PkgConfig REQUIRED)
pkg_check_modules(IBVERBS REQUIRED libibverbs)
pkg_check_modules(DPDK REQUIRED libdpdk>=25.07)

if (CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64")
  message(STATUS "Detected x86_64 architecture — enabling Intel-specific flags")
  add_compile_options(-march=native -msse3)
endif()

add_subdirectory("src")

