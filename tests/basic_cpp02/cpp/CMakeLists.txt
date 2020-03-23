cmake_minimum_required(VERSION 3.0)
project(basic)
add_library(basic basic.cpp)
include_directories(.)
set_property(TARGET basic PROPERTY POSITION_INDEPENDENT_CODE ON)

if(MSVC)
  set(CMAKE_EXE_LINKER_FLAGS
      "${CMAKE_EXE_LINKER_FLAGS} /NODEFAULTLIB")
endif() 