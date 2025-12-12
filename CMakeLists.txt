cmake_minimum_required(VERSION 3.13)

# Define the project
project(cevf)
set(cevf_VERSION_MAJOR 1)  #version number
set(cevf_VERSION_MINOR 0)

add_subdirectory(src)
if (BUILD_EXAMPLES)
  add_subdirectory(examples/hello1)
endif ()
