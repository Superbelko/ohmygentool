cmake_minimum_required(VERSION 3.0)
project(optest)
add_library(optest optest.cpp)
include_directories(.)

if(MSVC)
  set(CMAKE_EXE_LINKER_FLAGS
      "${CMAKE_EXE_LINKER_FLAGS} /NODEFAULTLIB")
endif() 