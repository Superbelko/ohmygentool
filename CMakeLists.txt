cmake_minimum_required(VERSION 3.3) # 3.10 for VS /std:c++17 switch
project(gentool)

option(GENTOOL_LIB "Build gentool as library")

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

SET(SOURCE_LIST 
    source/main.cpp 
    source/ppcallbacks.cpp 
    source/dlang_gen.cpp 
    source/printprettyd.cpp 
    source/iohelpers.cpp
    source/compiler.cpp
    source/options.cpp
    source/postedits.cpp
)

if (GENTOOL_LIB)
    add_library(gentool ${SOURCE_LIST})
    target_compile_definitions(gentool PRIVATE USE_LIB_TARGET GENTOOL_EXPORT_SYMBOLS)
    set_target_properties(gentool PROPERTIES POSITION_INDEPENDENT_CODE ON)
else()
    add_executable(gentool ${SOURCE_LIST})
endif()

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

if (UNIX AND NOT GENTOOL_LIB)
    
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
endif ()

# pick libraries 
if (LLVM_LINK_LLVM_DYLIB) 
  if ("clang-cpp" IN_LIST CLANG_EXPORTED_TARGETS)
    message(STATUS "Using dynamic (single library) LLVM and libclang ")
    # newer libs used with dylib options
    target_link_libraries( gentool 
        LLVM
        clang
        clang-cpp
    )
  else()
    message(FATAL_ERROR "Unsupported. LLVM dynamic library should be used with dynamic libclang as well")
  endif()
else()
    message(STATUS "Using LLVM and libclang components libs")
    # plain old libs
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
endif()

# Extra step when using D main(), write build script with configured environment
if (GENTOOL_LIB)
    set(DUMP_LIBS_FILENAME "${CMAKE_BINARY_DIR}/linklibs.txt")

    get_target_property(libs_list gentool LINK_LIBRARIES)
    message(STATUS "Writing libs file ${DUMP_LIBS_FILENAME}")
    file(WRITE "${DUMP_LIBS_FILENAME}" "${libs_list}")

    add_custom_command(
        TARGET gentool POST_BUILD
        COMMAND dub run -q --single "${CMAKE_CURRENT_SOURCE_DIR}/scripts/makeenv.d" -- ${DUMP_LIBS_FILENAME} ${CMAKE_CURRENT_SOURCE_DIR} ${LLVM_DIR} "$<CONFIG>"
        VERBATIM
    )
endif()
