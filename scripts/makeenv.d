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
    auto llvmDir = args[3];
    auto buildType = args[4];

    const command = readText(libListPath)
        .parseList()
        .makeEnv(llvmDir, buildType);

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

string makeEnv(string[] libs, string llvmDir = null, string buildType = null)
{
    import std.conv : text;
    import std.algorithm : map, joiner, canFind;
    import std.format : format;
    import std.path : buildNormalizedPath, dirSeparator;

    version(Windows)
        string itemSeparator = " " ;
    else
        string itemSeparator = `\" \"`; // hack for dumb tools

    version(Posix)
    {
        auto clangLibs = libs[1..$]
            .map!(s => "-L-l" ~ s)
            .joiner(itemSeparator)
            .text;
        clangLibs = "-l" ~ libs[0] ~ itemSeparator ~ clangLibs;
    }

    // folder that contains llvm cmake config (e.g. ~/llvm-build/lib/cmake/llvm)
    // we are interested in static libraries files location
    if (llvmDir)
    {
        llvmDir = buildNormalizedPath(llvmDir, "..", "..");
    }

    string dubBuildType = buildType.canFind("Debug") ? "debug" : "release";

    // windows linker flags is for LDC2, for DMD you'll need to prepend it with -L
    version(Windows)
    return "set CLANGLIBS=%s\nset LIB=%s;build\\%s\ndub build --build=%s %s"
        .format(llvmDir~dirSeparator~"*.lib", llvmDir, buildType, dubBuildType, "%*");
    else
    return "export CLANGLIBS=\"%s\"\nexport LIBRARY_PATH=%s\ndub build --build=%s \"$@\""
        .format(clangLibs, llvmDir, dubBuildType);
}
