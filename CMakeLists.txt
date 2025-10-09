cmake_minimum_required(VERSION 3.20)

project(res3)

enable_language(C CXX ASM)

# Copies all binaries to `build/ninja`
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY 
  ${CMAKE_BINARY_DIR}
)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY 
  ${CMAKE_BINARY_DIR}
)

# Helpful utility macros
include("scripts/cmake/Utils.cmake")

#list(APPEND CMAKE_PREFIX_PATH "/home/ba466/vendors/dpdk/dpdk")
#set(PcapPlusPlus_DIR "~/Sources/PcapPlusPlus/BUILT_HERE/lib/cmake/pcapplusplus/")
#find_package(PcapPlusPlus REQUIRED)

add_subdirectory("src")

