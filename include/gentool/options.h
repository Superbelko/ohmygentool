#pragma once


struct CInputOptions
{
    int pathsNum;
    const char** paths;

    int definesNum;
    const char** defines;

    int includesNum;
    const char** includes;

    int systemIncludesNum;
    const char** systemIncludes;

    int cflagsNum;
    const char** cflags; // compiler flags

    const char* standard;            // language standard version, like c11 or c++14
};

struct COutputOptions
{
    int extrasNum;
    const char** extras;

    const char* path;
};


#ifdef __cplusplus
#include <vector>
#include <string>
#include <string_view>

namespace gentool 
{
    namespace options_impl 
    {
        struct OutputOptionsTmp;
        struct InputOptionsTmp;
    }

    struct InputOptions
    {
        std::vector<std::string> paths;
        std::vector<std::string> defines;
        std::vector<std::string> includes;
        std::vector<std::string> systemIncludes;
        std::vector<std::string> cflags; // compiler flags
        std::string standard; // language standard version, like c11 or c++14

        InputOptions() = default;
        InputOptions(const CInputOptions& opts);
        // makes shallow copy for use within C
        CInputOptions toPlainC() const;
    };


    struct OutputOptions
    {
        std::vector<std::string> extras;
        std::string path;

        OutputOptions() = default;
        OutputOptions(const COutputOptions& opts);
        // makes shallow copy for use within C
        COutputOptions toPlainC() const;
    };

    /// frees up memory allocated using C++ new operator
    void cleanup(CInputOptions& opts);
    /// ditto
    void cleanup(COutputOptions& opts);

    namespace options_impl 
    {
        struct InputOptionsTmp
        {
            std::vector<const char*> paths;
            std::vector<const char*> defines;
            std::vector<const char*> includes;
            std::vector<const char*> systemIncludes;
            std::vector<const char*> cflags;
            const char* standard;

            operator CInputOptions() const;
        };
        struct OutputOptionsTmp
        {
            std::vector<const char*> extras;
            const char* path;

            operator COutputOptions() const;
        };
    }
    
}


#endif
