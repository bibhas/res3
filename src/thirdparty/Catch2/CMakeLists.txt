cmake_minimum_required(VERSION 3.20)

# Catch2 interface library
utils_add_interface_library(Catch2
  INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/include
)
