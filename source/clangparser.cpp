
#include "llvm/ADT/ArrayRef.h"
#include "clang/AST/AST.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/Decl.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/Mangle.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchersMacros.h"
#include "clang/Basic/TargetInfo.h"
#include "clang/Driver/Driver.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "clang/Parse/ParseAST.h"
#include "clang/CodeGen/CodeGenAction.h"
#include "clang/Lex/PreprocessorOptions.h"

#include "clangparser.h"

// STATIC VARIABLES
thread_local clang::LangOptions const *ClangParser::g_langOptions;
thread_local clang::PrintingPolicy *ClangParser::g_printPolicy;

using namespace clang;
using namespace clang::ast_matchers;
using namespace clang::driver;
using namespace clang::tooling;

void ClangParser::prepareCompilerforFile(const char *sourcePath)
{
    // To reuse the source manager we have to create these tables.
    m_CompilerInstance.getSourceManager().clearIDTables();

    // supply the file to the source manager and set it as main-file
    const clang::FileEntry *file = m_CompilerInstance.getFileManager().getFile(sourcePath);
    clang::FileID fileID = m_CompilerInstance.getSourceManager().createFileID(file, clang::SourceLocation(), clang::SrcMgr::CharacteristicKind::C_User);

    if (!m_CompilerInstance.getSourceManager().getMainFileID().isValid())
        m_CompilerInstance.getSourceManager().setMainFileID(fileID);

    // CodeGenAction needs this
    clang::FrontendOptions &feOptions = m_CompilerInstance.getFrontendOpts();
    feOptions.Inputs.clear();
    feOptions.Inputs.push_back(clang::FrontendInputFile(sourcePath,
                                                        clang::FrontendOptions::getInputKindForExtension(clang::StringRef(sourcePath).rsplit('.').second), false));
}

ClangParser::ClangParser(const gentool::InputOptions* ins, const gentool::OutputOptions* out)
{
    // Usually all examples try to build the CompilerInstance Object from CompilerInvocation object.
    // However, CompilterInstance already has an in-built CompilerInvocation object. So we use it.
    // Only problem is: target options are not set correctly for the in-built object. We do it manually.
    // This below line is just to assert that the object exist.
    clang::CompilerInvocation &invocation = m_CompilerInstance.getInvocation();

    // Diagnostics is needed - set it up
    m_CompilerInstance.createDiagnostics();

    // set the include directories path - you can configure these at run-time from command-line options
    clang::HeaderSearchOptions &headerSearchOptions = m_CompilerInstance.getHeaderSearchOpts();

    if (ins)
    {
        for (const auto &p : ins->systemIncludes)
        {
            headerSearchOptions.AddPath(p, clang::frontend::IncludeDirGroup::System, false, false);
        }
        for (const auto &p : ins->includes)
        {
            headerSearchOptions.AddPath(p, clang::frontend::IncludeDirGroup::Angled, false, false);
            //headerSearchOptions.AddPath(p, clang::frontend::IncludeDirGroup::Quoted, false, false);
        }
    }
    else
    {
        headerSearchOptions.AddPath("C:/Program Files (x86)/Windows Kits/10/Include/10.0.17134.0/ucrt", clang::frontend::IncludeDirGroup::System, false, false);
        headerSearchOptions.AddPath("C:\\Program Files (x86)\\Microsoft Visual Studio\\2017\\Community\\VC\\Tools\\MSVC\\14.15.26726\\include", clang::frontend::IncludeDirGroup::System, false, false);
        headerSearchOptions.AddPath("F:\\prog\\3rd_party\\!NDA\\PhysX-3.4\\PxShared\\include", clang::frontend::IncludeDirGroup::Angled, false, false);
        headerSearchOptions.AddPath("F:\\prog\\3rd_party\\!NDA\\PhysX-3.4\\PhysX_3.4\\Include", clang::frontend::IncludeDirGroup::Angled, false, false);
        // headerSearchOptions.Verbose = true;
    }

    // set few options
    clang::LangOptions &langOptions = m_CompilerInstance.getLangOpts();

    //auto& feOpts = m_CompilerInstance.getFrontendOpts();

    bool isCpp = true;
    int standardVersion = -1;
    if (!ins->standard.empty())
    {
        // get actual standard
        auto pp = ins->standard.rfind("++");
        if (pp == std::string::npos)
        {
            // assume it is C##
            isCpp = false;
            pp = 0;
        }
        else
        {
            pp += 2;
        }
        standardVersion = std::stoi(ins->standard.substr(pp));
    }

    langOptions.CPlusPlus = isCpp;
    langOptions.C99 = !isCpp && standardVersion == 99;
    langOptions.C11 = !isCpp && standardVersion == 11;
    langOptions.C17 = !isCpp && standardVersion == 17;
    langOptions.CPlusPlus11 = isCpp && standardVersion >= 11;
    langOptions.CPlusPlus14 = isCpp && standardVersion >= 14;
    langOptions.CPlusPlus17 = isCpp && standardVersion >= 17;
    langOptions.CPlusPlus2a = isCpp && standardVersion >= 18; // no actual repr for this yet
    langOptions.Bool = 1;
    langOptions.RTTI = 0;
    langOptions.DoubleSquareBracketAttributes = 1;
    //#if defined(_MSC_VER)
    langOptions.MSCompatibilityVersion = LangOptions::MSVCMajorVersion::MSVC2015;
    //langOptions.MSCompatibilityVersion = 19;
    //langOptions.MicrosoftExt = 1;
    langOptions.MSVCCompat = !isCpp;
    //langOptions.MSBitfields = 1;
    langOptions.DeclSpecKeyword = 1;
    langOptions.DelayedTemplateParsing = 1; // MSVC parses templates at the time of actual use
    m_CompilerInstance.getDiagnosticOpts().setFormat(clang::TextDiagnosticFormat::MSVC);
    //m_CompilerInstance.getTargetOpts().ABI = "microsoft";
    //#endif
    g_langOptions = &langOptions;
    if (g_printPolicy)
        delete g_printPolicy;
    g_printPolicy = new PrintingPolicy(langOptions);
    g_printPolicy->SuppressScope = true;
    g_printPolicy->Bool = true;
    g_printPolicy->SuppressTagKeyword = true;
    g_printPolicy->ConstantsAsWritten = true;   // prevent prettifying masks and other unpleasent things
    g_printPolicy->SuppressImplicitBase = true; // no "this->",  please
    //g_printPolicy->Indentation = 2;

    // Need to set the source manager before AST
    m_CompilerInstance.createFileManager();
    m_CompilerInstance.createSourceManager(m_CompilerInstance.getFileManager());

    // Need to set the target before AST. Adjust the default target options and create a target
    m_CompilerInstance.getTargetOpts().Triple = llvm::sys::getProcessTriple();
    auto cto = std::make_shared<clang::TargetOptions>(m_CompilerInstance.getTargetOpts());
    m_CompilerInstance.setTarget(clang::TargetInfo::CreateTargetInfo(m_CompilerInstance.getDiagnostics(), cto));

    // Create pre-processor and AST Context
    m_CompilerInstance.createPreprocessor(clang::TranslationUnitKind::TU_Module);
    m_CompilerInstance.createASTContext();

    if (m_CompilerInstance.hasPreprocessor())
    {
        clang::Preprocessor &preprocessor = m_CompilerInstance.getPreprocessor();

        if (ins)
        {
            for (const auto &def : ins->defines)
            {
                preprocessor.getPreprocessorOpts().addMacroDef(def);
            }
        }
        else
        {
#ifdef _WIN32
            preprocessor.getPreprocessorOpts().addMacroDef("_MSC_EXTENSIONS");
            preprocessor.getPreprocessorOpts().addMacroDef("_INTEGRAL_MAX_BITS=64");
            preprocessor.getPreprocessorOpts().addMacroDef("_MSC_VER=1915");
            preprocessor.getPreprocessorOpts().addMacroDef("_MSC_FULL_VER=191526726");
            preprocessor.getPreprocessorOpts().addMacroDef("_MSC_BUILD=0");
            preprocessor.getPreprocessorOpts().addMacroDef("_M_AMD64=100");
            preprocessor.getPreprocessorOpts().addMacroDef("_M_X64=100");
            preprocessor.getPreprocessorOpts().addMacroDef("_WIN64");
            preprocessor.getPreprocessorOpts().addMacroDef("_WIN32");
            preprocessor.getPreprocessorOpts().addMacroDef("_CPPRTTI");
            preprocessor.getPreprocessorOpts().addMacroDef("_MT");
#endif
        }

        preprocessor.getBuiltinInfo().initializeBuiltins(preprocessor.getIdentifierTable(), preprocessor.getLangOpts());
    }
}

bool ClangParser::parseAST(const char *sourcePath, clang::ast_matchers::MatchFinder* finder)
{
    // create the compiler instance setup for this file as main file
    prepareCompilerforFile(sourcePath);

    std::unique_ptr<clang::ASTConsumer> pAstConsumer(finder->newASTConsumer());
    clang::DiagnosticConsumer &diagConsumer = m_CompilerInstance.getDiagnosticClient();
    diagConsumer.BeginSourceFile(m_CompilerInstance.getLangOpts(), &m_CompilerInstance.getPreprocessor());
    clang::ParseAST(m_CompilerInstance.getPreprocessor(), pAstConsumer.get(), m_CompilerInstance.getASTContext());
    diagConsumer.EndSourceFile();

    return diagConsumer.getNumErrors() != 0;
}

// Executes CodeGenAction and returns the pointer (caller should own and delete it)
// Returns NULL on failure (in case of any compiler errors etc.)
clang::CodeGenAction* ClangParser::emitLLVM(const char *sourcePath)
{
    #if 0
    // create the compiler instance setup for this file as the main file
    prepareCompilerforFile(sourcePath);

    // Create an action and make the compiler instance carry it out
    clang::CodeGenAction *Act = new clang::EmitLLVMOnlyAction();
    if (!m_CompilerInstance.ExecuteAction(*Act))
        return nullptr;

    return Act;
    #endif
    return nullptr;
}