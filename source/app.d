module app;
import std.algorithm;
import std.array;
import std.file;
import std.json;
import std.stdio;
import std.string;

import vibe.data.json;

import gentool.options;

extern (C) int gentool_run(int compilerArgc, const(char*)* compilerArgv, CInputOptions inOpts, COutputOptions outOpts);

immutable HELP_MESSAGE = 
r"Oh My Gentool - v0.3.0
Generates D bindings from C/C++ code

USAGE:
	gentool <project.json>

	<project.json>: .json configuration file path
";



int main(string[] args)
{
    if (args.length < 2)
    {
        writeln(HELP_MESSAGE);
        return 1;
    }

    auto path = args[1];
    auto json = readJson(path);

    InputOptions input = json.readInputOpts();
    OutputOptions output = json.readOutputOpts();
    replaceEnvVars(input);
    replaceEnvVars(output);

    auto cmd = makeCommandLine(args[0], input, output)
        .map!(toStringz)
        .array;
    
    return gentool_run(cast(int)cmd.length, &cmd[0], input.toPlainC, output.toPlainC);
}


auto readJson(string path)
{
    auto json = vibe.data.json.parseJsonString(
        File(path)
        .byLineCopy()
        .map!(a => a.strip())
        .filter!(a => !a.startsWith("//"))
        .join()
    );
    return json;
}


InputOptions readInputOpts(Json json)
{
    InputOptions opts;
    opts.deserializeJson(json["input"]);
    //json["cflags"].get!(Json[]).each!(a => opts.cflags ~= a.get!string());
    return opts;
}

OutputOptions readOutputOpts(Json json)
{
    import std.algorithm : among;
    import std.exception : enforce;
    OutputOptions opts;
    auto output = json["output"];

    void addExtrasIfPresent(scope string option)
    {
        if (auto attr = option in output)
            if (attr.get!bool) 
                opts.extras ~= option;
    }

    enforce(output["target"].get!string.among("dlang", "d2", "d"), "Only D target supported at this moment");

    opts.deserializeJson(json["output"]);

    addExtrasIfPresent("attr-nogc");
    addExtrasIfPresent("no-param-refs");
    addExtrasIfPresent("skip-bodies");
    addExtrasIfPresent("mangle-all");

    return opts;
}


string[] makeCommandLine(string programPath, const InputOptions input, const OutputOptions output)
{
    string[] result;
    result ~= programPath;
    result ~= collectSources(input);
    result ~= "--";
    result ~= "-x" ~ (input.standard.startsWith("c++") ? "c++" : "c");
    result ~= "-std=" ~ input.standard;
    foreach(d; input.cflags)
        result ~= d;
    foreach(d; input.defines)
        result ~= "-D" ~ d;
    foreach(d; input.includes)
        result ~= "-I" ~ d;
    foreach(d; input.systemIncludes)
        result ~= "-I" ~ d;
    return result;
}


const(string)[] collectSources(const InputOptions input)
{
    import std.file;
    import std.path;
    import std.range : chain;
    import std.functional : pipe;
    import std.conv : to;

    auto sources = input.paths
            .filter!(isFile)
            .map!(buildNormalizedPath)
            .array()
            ;

    input.paths
        .filter!(isDir)
        .each!(d => dirEntries(d, SpanMode.depth)
                .filter!(p => [".h", ".hpp", ".hxx"].canFind(p.name.extension))
                .map!(a => a.name)
                .map!(asNormalizedPath)
                .each!(p => sources ~= p.array)
        )
        ;

    return sources;
}