/+ dub.sdl:
	name "makeenv"
+/

int main(string[] args)
{
    import std.stdio : writeln;
    import std.file : readText, write, setAttributes;
    import std.path : dirSeparator;
    import std.conv : octal;
    import std.array : appender;

    auto libListPath = args[1];
    auto targetPath = args[2];

    const command = readText(libListPath)
        .parseList()
        .makeEnv();

    targetPath.scriptFile.write(command);

    writeln("Written file ", targetPath.scriptFile);
    // set read-write-execute to owner, read-execute to anyone else
    version(Posix)
        targetPath.scriptFile.setAttributes(octal!755);

    return 0;
}

string scriptFile(string baseDir)
{
    import std.path : dirSeparator;

    version(Windows)
    return baseDir ~ dirSeparator ~ "dubBuild.bat";
    else
    return baseDir ~ dirSeparator ~ "dubBuild.sh";
}

string[] parseList(string rawList)
{
    import std.algorithm : splitter;
    import std.array : array;

    return rawList
        .splitter(';')
        .array();
}

string makeEnv(string[] libs)
{
    import std.conv : text;
    import std.algorithm : map, joiner;
    import std.format : format;

    auto res = libs
        .map!(s => "-L-l" ~ s)
        .joiner(" ")
        .text;

    version(Windows)
    return "set LINK=\"%s\"\ndub build".format(res);
    else
    return "export LFLAGS=\"%s\"\ndub build".format(res);
}
