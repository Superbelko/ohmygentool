cmake_minimum_required(VERSION 3.0)
project(basic)
add_library(basic basic.cpp)
include_directories(.)

if(MSVC)
  set(CMAKE_EXE_LINKER_FLAGS
      "${CMAKE_EXE_LINKER_FLAGS} /NODEFAULTLIB")
endif() 