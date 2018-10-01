#pragma once

#include "clang/AST/AST.h"
#include "clang/Frontend/CompilerInstance.h"

#include "gentool/options.h"

namespace clang
{
    class CodeGenAction;
    class LangOptions;
    class PrintingPolicy;

    namespace ast_matchers
    {
        class MatchFinder;
    }
}

///--------------------
// http://my-classes.com/2014/07/01/working-with-ast-matcher-without-clangtool-compilation-database/

class ClangParser
{
private:
	clang::CompilerInstance m_CompilerInstance;

	void prepareCompilerforFile(const char* sourcePath);

public:

	static thread_local clang::LangOptions const* g_langOptions;
	static thread_local clang::PrintingPolicy* g_printPolicy;

	ClangParser(const gentool::InputOptions* ins = nullptr, const gentool::OutputOptions* out = nullptr);

	bool parseAST(const char* sourcePath, clang::ast_matchers::MatchFinder* finder);

	// Executes CodeGenAction and returns the pointer (caller should own and delete it)
	// Returns NULL on failure (in case of any compiler errors etc.)
	clang::CodeGenAction* emitLLVM(const char* sourcePath);

	clang::ASTContext& GetASTContext() 
	{ 
		return m_CompilerInstance.getASTContext(); 
	}
}; // class ClangParser
