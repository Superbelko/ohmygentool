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

include(CheckCXXSourceCompiles)
set(CMAKE_REQUIRED_FLAGS -std=c++17)
check_cxx_source_compiles(
    "
    #if __has_include(<filesystem>)
    #include <filesystem>
    #else
    #include <experimental/filesystem>
    #endif
    namespace fs = std::filesystem;
    int main(int argc, char** argv) { return 0;}
    " GENTOOL_IS_STABLE_FS
)

# Now build our tools
add_executable(gentool 
    source/main.cpp 
    source/ppcallbacks.cpp 
    source/dlang_gen.cpp 
    source/printprettyd.cpp 
    source/iohelpers.cpp
)

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


check_cxx_source_compiles(
    "
    #include <ciso646>
    #if !defined(__GLIBCXX__) && !defined(__GLIBCPP__)
    static_assert(0);
    #endif
    int main(int argc, char** argv) { return 0;}
    " GENTOOL_GNU_LIBCXX
)

check_cxx_source_compiles(
    "
    #include <ciso646>
    #ifndef (_LIBCPP_VERSION)
    static_assert(0);
    #endif
    int main(int argc, char** argv) { return 0;}
    " GENTOOL_LLVM_LIBCXX
)

if (UNIX)
    
    if (${GENTOOL_GNU_LIBCXX})
        target_link_libraries(gentool -lstdc++)
        if (NOT $GENTOOL_IS_STABLE_FS)
        target_link_libraries(gentool -lstdc++fs)
        endif ()
    endif ()

    if (${GENTOOL_USING_LIBCXX})
        target_link_libraries(gentool -lc++)
        if (NOT $GENTOOL_IS_STABLE_FS)
        target_link_libraries(gentool -lc++experimental)
        endif ()
    endif ()

    target_compile_options(gentool PRIVATE "-fno-rtti")
endif (UNIX)