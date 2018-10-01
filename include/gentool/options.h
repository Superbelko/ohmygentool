#pragma once

#include <vector>
#include <string>
#include <string_view>

namespace gentool 
{
    struct InputOptions
    {
        std::vector<std::string> paths;
        std::vector<std::string> defines;
        std::vector<std::string> includes;
        std::vector<std::string> systemIncludes;
        std::vector<std::string> cflags; // compiler flags
        std::string standard; // language standard version, like c11 or c++14
    };


    struct OutputOptions
    {
        std::vector<std::string> extras;
        std::string path;
    };
}