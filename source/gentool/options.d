module gentool.options;

import std.stdio;
import std.string : toStringz;

import vibe.data.serialization;


//---------- options.h

struct CInputOptions
{
    int pathsNum;
    const(char*)* paths;

    int definesNum;
    const(char*)* defines;

    int includesNum;
    const(char*)* includes;

    int systemIncludesNum;
    const(char*)* systemIncludes;

    int cflagsNum;
    const(char*)* cflags; // compiler flags

    const(char)* standard;            // language standard version, like c11 or c++14
}

struct COutputOptions
{
    int extrasNum;
    const(char*)* extras;

    const(char)* path;
}

//----------


struct InputOptions
{
    string[] paths;
    string[] defines;
    string[] includes;
    @name("system") string[] systemIncludes;
    string[] cflags; // compiler flags
    @name("std") string standard; // language standard version, like c11 or c++14
}


struct OutputOptions
{
    @ignore string[] extras;
    string path;
}

private const(char*)[] copyptrs(string NAMEOF, TO, FROM)(ref TO to, FROM from)
{
    import std.algorithm;
    
    import std.array;
    mixin(`typeof(return) _dst = from.`~NAMEOF~`.map!(toStringz).array;`);
    

    //mixin("from."~NAMEOF~".map!(a => _dst ~= a.ptr);");
    mixin("to." ~NAMEOF~ " = _dst.ptr;");
    mixin("to." ~NAMEOF~ "Num = cast(int) from." ~NAMEOF~ ".length;");
    return _dst;
}

CInputOptions toPlainC(InputOptions opts)
{
    import std.algorithm;
    CInputOptions res;
    //const(char*)[] _cflags;
    //opts.cflags.map!(a => _cflags ~= a.ptr);
    //res.cflags = _cflags.ptr;
    //res.cflagsNum = cast(int) opts.cflags.length;
    copyptrs!(__traits(identifier, res.cflags))(res, opts);
    copyptrs!(__traits(identifier, res.includes))(res, opts);
    copyptrs!(__traits(identifier, res.defines))(res, opts);
    copyptrs!(__traits(identifier, res.systemIncludes))(res, opts);
    copyptrs!(__traits(identifier, res.paths))(res, opts);
    res.standard =opts.standard?  toStringz(opts.standard) : null;
    return res;
}

COutputOptions toPlainC(OutputOptions opts)
{
    import std.algorithm;
    COutputOptions res;
    copyptrs!(__traits(identifier, res.extras))(res, opts);
    res.path = opts.path? toStringz(opts.path) : null;
    return res;
}

void replaceEnvVars(T)(T options)
{
    import std.array;
    enum members = [__traits(allMembers, T)];
    static foreach(m; members)
    {
        mixin(`replaceEnvVars(options.`~m~`);`);
    }
}


private void replaceEnvVars(string[] arr)
{
    foreach(s; arr)
        replaceEnvVars(s);
}

private void replaceEnvVars(ref string str)
{
    import std.process;
    import std.array;
    Pair p;
    while(findEnvVar(str, p))
    {
        auto envVar = str[p.s..p.e];
        if (auto expansion = environment.get(envVar))
        {
            str = str.replace(envVar, expansion);
        }
    }
}

private struct Pair 
{
    int s;
    int e;
}

// Find substring with special pattern in form $(VARIABLE_NAME)
private bool findEnvVar(string str, out Pair pair)
{
    import std.regex;
    import std.algorithm;
    immutable pattern = `(\$\(\w+\))`;
    auto match = str.matchFirst(pattern);
    if (!match.empty) {
        auto pos = str.countUntil(match[0]);
        pair = Pair(cast(int) pos, cast(int) (pos + match[0].length));
    }
    return !match.empty;
}