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

    CInputOptions InputOptions::toPlainC() const
    {
        // TODO: this will leak but for now it's ok since all this junk will be removed later on
        CInputOptions res;


        res.standard = standard.data();

        res.paths = new const char*[paths.size()];
        res.pathsNum = paths.size();
        for(auto i = 0; i < paths.size(); i++)
            res.paths[i] = paths[i].data();

        res.defines = new const char*[defines.size()];
        res.definesNum = defines.size();
        for(auto i = 0; i < defines.size(); i++)
            res.defines[i] = defines[i].data();

        res.includes = new const char*[includes.size()];
        res.includesNum = includes.size();
        for(auto i = 0; i < includes.size(); i++)
            res.includes[i] = includes[i].data();

        res.systemIncludes = new const char*[systemIncludes.size()];
        res.systemIncludesNum = systemIncludes.size();
        for(auto i = 0; i < systemIncludes.size(); i++)
            res.systemIncludes[i] = systemIncludes[i].data();

        res.cflags = new const char*[cflags.size()];
        res.cflagsNum = cflags.size();
        for(auto i = 0; i < cflags.size(); i++)
            res.cflags[i] = cflags[i].data();

        return res;
    }

    OutputOptions::OutputOptions(const COutputOptions& opts)
    {
        extras.assign(opts.extras, opts.extras + opts.extrasNum);
        path = std::string(opts.path);
    }
    
    COutputOptions OutputOptions::toPlainC() const
    {
        // TODO: this will leak but for now it's ok since all this junk will be removed later on
        COutputOptions res;
        res.path = path.data();

        res.extras = new const char*[extras.size()];
        res.extrasNum = extras.size();
        for(auto i = 0; i < extras.size(); i++)
            res.extras[i] = extras[i].data();
        return res;
    }

    void cleanup(CInputOptions& opts)
    {
        if (opts.cflags)
            delete opts.cflags;
        if (opts.defines)
            delete opts.defines;
        if (opts.paths)
            delete opts.paths;
        if (opts.systemIncludes)
            delete opts.systemIncludes;
        if (opts.includes)
            delete opts.includes;
        opts = {0};
    }
    void cleanup(COutputOptions& opts)
    {
        if (opts.extras)
            delete opts.extras;
        opts = {0};
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