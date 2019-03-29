#include "gentool/options.h"

namespace gentool
{
    using namespace options_impl;

    InputOptions::InputOptions(const CInputOptions& opts)
    {
        paths.assign(opts.paths, opts.paths + opts.pathsNum);
        defines.assign(opts.defines, opts.defines + opts.definesNum);
        includes.assign(opts.includes, opts.includes + opts.includesNum);
        systemIncludes.assign(opts.systemIncludes, opts.systemIncludes + opts.systemIncludesNum);
        cflags.assign(opts.cflags, opts.cflags + opts.cflagsNum);
        standard = std::string(opts.standard);
    }
    
    options_impl::InputOptionsTmp InputOptions::toPlainC() const
    {
        InputOptionsTmp res;
        res.standard = standard.c_str();
        for(auto it : paths)
            res.paths.push_back(it.c_str());
        for(auto it : defines)
            res.defines.push_back(it.c_str());
        for(auto it : includes)
            res.includes.push_back(it.c_str());
        for(auto it : systemIncludes)
            res.systemIncludes.push_back(it.c_str());
        for(auto it : cflags)
            res.cflags.push_back(it.c_str());
        return res;
    }

    OutputOptions::OutputOptions(const COutputOptions& opts)
    {
        extras.assign(opts.extras, opts.extras + opts.extrasNum);
        path = std::string(opts.path);
    }
    
    options_impl::OutputOptionsTmp OutputOptions::toPlainC() const
    {
        OutputOptionsTmp res;
        res.path = path.c_str();
        for(auto it : extras)
            res.extras.push_back(it.c_str());
        return res;
    }


    namespace options_impl
    {
        InputOptionsTmp::operator CInputOptions() const
        {
            CInputOptions input;

            input.paths = (const char**) paths.data();
            input.pathsNum = paths.size();

            input.defines = (const char**) defines.data();
            input.definesNum = defines.size();

            input.includes = (const char**) includes.data();
            input.includesNum = includes.size();

            input.systemIncludes = (const char**) systemIncludes.data();
            input.systemIncludesNum = systemIncludes.size();

            input.cflags = (const char**) cflags.data();
            input.cflagsNum = cflags.size();
            
            input.standard = standard;
            return input;
        }

        OutputOptionsTmp::operator COutputOptions() const
        {
            COutputOptions out;

            out.extras = (const char**) extras.data();
            out.extrasNum = extras.size();

            out.path = path;
            return out;
        }
    }
}