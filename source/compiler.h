#pragma once
#include "gentool/defines.h"
// contains helper function to init clang compiler to be used with AST visitors

struct Compiler;
struct SourceMan;
struct ASTConsumer;



//extern"C" {

API_EXPORT Compiler* createCompiler(const char* file);
API_EXPORT void freeCompiler(Compiler* compiler);
API_EXPORT void compilerAddMacro(Compiler* compiler, const char* macroDef);
API_EXPORT void compilerAddHeaderPath(Compiler* compiler, const char* includePath);

API_EXPORT void parseAST(Compiler* compiler, ASTConsumer* consumer);

//}


