cmake_minimum_required(VERSION 3.23)

# Define the project
project(cevf)
set(cevf_VERSION_MAJOR 1)  #version number
set(cevf_VERSION_MINOR 0)

set(LUA_LIBRARY "" CACHE STRING "lua library")
set(LUA_INCLUDE_DIR "" CACHE STRING "lua header directory")
if(NOT "${LUA_LIBRARY}" STREQUAL "")
if("${LUA_INCLUDE_DIR}" EQUAL "")
message(FATAL_ERROR "LUA_INCLUDE_DIR needed")
endif()
set(BUILD_CEVF_LUA ON)
endif()

add_subdirectory(src)
if (BUILD_EXAMPLES)
  add_subdirectory(examples/hello1)
  add_subdirectory(examples/hello2)
  add_subdirectory(examples/tcp1)
endif ()
