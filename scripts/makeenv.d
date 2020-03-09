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

    const command = readText(libListPath)
        .parseList()
        .makeEnv(llvmDir);

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

string makeEnv(string[] libs, string llvmDir = null)
{
    import std.conv : text;
    import std.algorithm : map, joiner;
    import std.format : format;
    import std.path : buildNormalizedPath, dirSeparator;

    auto res = libs
        .map!(s => "-L-l" ~ s)
        .joiner(" ")
        .text;

    // folder that contains llvm cmake config (e.g. ~/llvm-build/lib/cmake/llvm)
    // we are interested in static libraries files location
    if (llvmDir)
        llvmDir = buildNormalizedPath(llvmDir, "..", "..");

    // windows linker flags is for LDC2, for DMD you'll need to prepend it with -L
    version(Windows)
    return "set LFLAGS=%s\nset LIB=%s;build\\Release\ndub build --build=release"
        .format(llvmDir~dirSeparator~"*.lib", llvmDir);
    else
    return "export LFLAGS=\"%s\"\nexport LIBRARY_PATH=%s\ndub build --build=release".format(res, llvmDir);
}
