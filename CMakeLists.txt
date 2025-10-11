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

#list(APPEND CMAKE_PREFIX_PATH "/home/ba466/vendors/dpdk/dpdk")
#set(PcapPlusPlus_DIR "~/Sources/PcapPlusPlus/BUILT_HERE/lib/cmake/pcapplusplus/")
#find_package(PcapPlusPlus REQUIRED)

add_subdirectory("src")

