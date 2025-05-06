
#include "compiler.h"


#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/Basic/FileManager.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Basic/TargetOptions.h"
#include "clang/Basic/TargetInfo.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Lex/PreprocessorOptions.h"
#include "clang/Parse/ParseAST.h"
#include "llvm/Support/raw_ostream.h"

#if LLVM_VERSION_MAJOR > 17
#include "llvm/TargetParser/Host.h"
#else
#include "llvm/Support/Host.h"
#endif


Compiler* createCompiler(const char* file)
{
    // CompilerInstance will hold the instance of the Clang compiler for us,
    // managing the various objects needed to run the compiler.
    clang::CompilerInstance* TheCompInst = new clang::CompilerInstance();
    TheCompInst->createDiagnostics(nullptr,true);

    // Initialize target info with the default triple for our platform.
    auto TO = std::make_shared<clang::TargetOptions>();
    TO->Triple = llvm::sys::getDefaultTargetTriple();
    clang::TargetInfo *TI = clang::TargetInfo::CreateTargetInfo(
        TheCompInst->getDiagnostics(), TO);
    TheCompInst->setTarget(TI);

    TheCompInst->createFileManager();
    clang::FileManager &FileMgr = TheCompInst->getFileManager();
    TheCompInst->createSourceManager(FileMgr);
    clang::SourceManager &SourceMgr = TheCompInst->getSourceManager();
    TheCompInst->createPreprocessor(clang::TranslationUnitKind::TU_Complete);
    TheCompInst->createASTContext();

    // Set the main file handled by the source manager to the input file.
#if (LLVM_VERSION_MAJOR > 17)
const clang::FileEntryRef FileIn = FileMgr.getFileRef(file).get();
#elif (LLVM_VERSION_MAJOR > 12)
const clang::FileEntry *FileIn = FileMgr.getFile(file).get();
#else
const clang::FileEntry *FileIn = FileMgr.getFile(file);
#endif
    auto id = SourceMgr.getOrCreateFileID(FileIn, clang::SrcMgr::CharacteristicKind::C_User);
    SourceMgr.setMainFileID(id);
    TheCompInst->getDiagnosticClient().BeginSourceFile(
        TheCompInst->getLangOpts(),
        &TheCompInst->getPreprocessor());
    return (Compiler*) TheCompInst;
}

void freeCompiler(Compiler* compiler)
{
    if (compiler)
    {
        delete reinterpret_cast<clang::CompilerInstance*>(compiler);
    }
}

void parseAST(Compiler* compiler, ASTConsumer* consumer)
{
    auto TheCompInst = reinterpret_cast<clang::CompilerInstance*>(compiler);
    auto TheConsumer = reinterpret_cast<clang::ASTConsumer*>(consumer);
    clang::ParseAST(TheCompInst->getPreprocessor(), TheConsumer, TheCompInst->getASTContext());
}


void compilerAddMacro(Compiler* compiler, const char* macroDef)
{   
    auto ci = reinterpret_cast<clang::CompilerInstance*>(compiler);
    ci->getPreprocessorOpts().addMacroDef(std::string(macroDef));
}

void compilerAddHeaderPath(Compiler* compiler, const char* includePath)
{
    auto ci = reinterpret_cast<clang::CompilerInstance*>(compiler);
    ci->getHeaderSearchOpts().AddPath(std::string(includePath), clang::frontend::IncludeDirGroup::Quoted, false, false);
}