cmake_minimum_required(VERSION 3.0)
project(nstest)
add_library(nstest nstest.cpp)
include_directories(.)
set_property(TARGET nstest PROPERTY POSITION_INDEPENDENT_CODE ON)

if(MSVC)
  set(CMAKE_EXE_LINKER_FLAGS
      "${CMAKE_EXE_LINKER_FLAGS} /NODEFAULTLIB")
endif() 