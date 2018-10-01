cmake_minimum_required(VERSION 3.0) # 3.10 for VS /std:c++17 switch
project(gentool)

find_package(LLVM REQUIRED CONFIG)
find_package(Clang REQUIRED CONFIG HINTS "${LLVM_DIR}/../clang") # assuming LLVM_DIR is llvm-build/lib/cmake/llvm

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

include_directories(${CLANG_INCLUDE_DIRS})
add_definitions(${CLANG_DEFINITION})

include_directories("include" "source")


# Now build our tools
add_executable(gentool source/main.cpp source/clangparser.cpp source/dlang_gen.cpp source/iohelpers.cpp)

# Find the libraries that correspond to the LLVM components
# that we wish to use
llvm_map_components_to_libnames(llvm_libs support core irreader)

# Link against LLVM libraries
target_link_libraries( gentool 
    ${llvm_libs} 
    clangAST
    clangBasic
    clangFrontend
    clangLex
    clangParse
    clangSema
    clangTooling
)

set_property(TARGET gentool PROPERTY CXX_STANDARD 17)
set_property(TARGET gentool PROPERTY CXX_STANDARD_REQUIRED ON)