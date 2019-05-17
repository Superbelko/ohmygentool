#pragma once

// llvm using some black magic
#pragma warning(push)
#pragma warning( disable: 4146 4996 )

#include <iostream>
#include <ostream>
#include <fstream>
#include <string>
#include <algorithm>
#include <unordered_map>
#include <string_view>
#include <memory>

#if __has_include(<filesystem>)
#include <filesystem>
#else
#include <experimental/filesystem>
#endif

#ifdef _WIN32
#include <corecrt_wstdio.h>
#else
#include <stdio.h>
#endif


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
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "clang/Parse/ParseAST.h"
#include "clang/CodeGen/CodeGenAction.h"
#include "clang/Lex/PreprocessorOptions.h"
//#include "clang/Tooling/Core/QualTypeNames.h" // fully qualified template args lookup 
#include "llvm/Support/raw_ostream.h"


#include "gentool/abstract_generator.h"
#include "gentool/options.h"
#include "iohelpers.h"

#pragma warning(pop)

// Similar to default printPretty for expressions, except prints D from C++ AST
void printPrettyD(const clang::Stmt *stmt, llvm::raw_ostream &OS, clang::PrinterHelper *Helper,
                     const clang::PrintingPolicy &Policy, unsigned Indentation = 0,
                     const clang::ASTContext *Context = nullptr);
// Same as above but for ctor initializers
void printPrettyD(const clang::CXXCtorInitializer *init, llvm::raw_ostream &OS, clang::PrinterHelper *Helper,
                     const clang::PrintingPolicy &Policy, unsigned Indentation = 0,
                     const clang::ASTContext *Context = nullptr);

bool hasVirtualMethods(const clang::RecordDecl* rd);

// Adjusts ref/pointer types to be used as variable type and wraps/unwraps if necessary
clang::QualType adjustForVariable(clang::QualType ty, clang::ASTContext* ctx = nullptr);

template<typename TA>
void printDTemplateArgumentList(llvm::raw_ostream &OS, llvm::ArrayRef<TA> Args, const clang::PrintingPolicy &Policy, bool SkipBrackets = false);

// Hacky marker class that is passed as printer helper, does nothing by itself
class DPrinterHelper_PointerReturn : public clang::PrinterHelper
{
public:
    virtual bool handledStmt(clang::Stmt* E, llvm::raw_ostream& OS);
};


class NamespacePolicy;
class NamespacePolicy_StringList;

struct DlangBindGenerator : public gentool::IAbstractGenerator
{
    friend class NamespacePolicy;
    friend class NamespacePolicy_StringList;
public:
    static thread_local clang::PrintingPolicy* g_printPolicy;
protected:
    std::unique_ptr<NamespacePolicy> nsPolicy;
    clang::SourceManager* SourceMgr;
    clang::Sema* sema;
	std::ofstream fileOut;
    std::ofstream mangleOut;
	OutStreamHelper out = OutStreamHelper(nullptr/*&std::cout*/, &fileOut);
	std::string classOrStructName;
	std::string finalTypeName;
	std::list<const clang::RecordDecl*> declStack; // for nesting structs or enums
	using StoredTypes = std::unordered_map<std::string, const clang::Decl*>;
	StoredTypes storedTypes;
	using FunctionDecls = std::unordered_map<std::string, const clang::FunctionDecl*>;
	FunctionDecls functionDecls;
	using EnumDecls = std::unordered_map<std::string, const clang::EnumDecl*>;
	EnumDecls enumDecls;
    using MacroDefs = std::unordered_map<std::string, bool>;
    MacroDefs macroDefs;
    using ForwardTypes = std::unordered_map<std::string, bool>;
	ForwardTypes forwardTypes;
    int localAnonRecordId = 1; // specific to top records anonimous counter
    int globalAnonTypeId = 0; // used to deanonimize at global scope
	int accumBitFieldWidth = 0; // accumulated width in bits, we need to round up to byte, short, int or long and split when necessary
    int mixinTemplateId = 0; // used to write unique id's for namespace hack
    bool handledClassDecl = false; // true if we did written decl body
	bool isPrevIsBitfield = false; // was last field is a bitfield? (needed to split up and pad)
    bool cppIsDefault = true;
    bool nogc = false;
    // Strip ref parameters from functions with bodies? (until DIP for r-value refs accepted they are a PITA to use in D)
    bool stripRefParam = false;
    bool skipBodies = false;
    bool mangleAll = false;
    bool oldNamespaces = false;

	gentool::InputOptions const* iops;

public:
    virtual bool isRelevantPath(const std::string_view path) override;
    virtual void setOptions(const gentool::InputOptions* inOpt, const gentool::OutputOptions* outOpt) override;
    virtual void prepare() override;
    virtual void finalize() override;
    virtual void onBeginFile(const std::string_view file) override;
    virtual void onEndFile(const std::string_view file) override;
    virtual void onStructOrClassEnter(const clang::RecordDecl* decl) override;
    virtual void onStructOrClassLeave(const clang::RecordDecl* decl) override;
    virtual void onEnum(const clang::EnumDecl* decl) override;
    virtual void onFunction(const clang::FunctionDecl* decl) override;
    virtual void onTypedef(const clang::TypedefDecl* decl) override;
    virtual void onGlobalVar(const clang::VarDecl* decl) override;

    // On define macro event
    void onMacroDefine(const clang::Token* name, const clang::MacroDirective* macro);
    void setSourceManager(clang::SourceManager* SM) { this->SourceMgr = SM; }
    void setSema(clang::Sema* Sema) { this->sema = Sema; }

    // Get file path and line & column parts (if any) from source location
    static std::tuple<std::string, std::string> getFSPathPart(const std::string_view loc);
    // Get line and column from source location as separate items
    static std::tuple<std::string, std::string> getLineColumnPart(const std::string_view loc);
private:
    static std::string _adjustVarInit(const std::string& e);
    static std::string _wrapParens(clang::QualType type);
    static void _typeRoll(clang::QualType type, std::vector<std::string>& parts);
    static std::string _toDBuiltInType(clang::QualType type);
public:
    static std::string toDStyle(clang::QualType type);
    static std::string sanitizedIdentifier(const std::string& id);
    static std::string getAccessStr(clang::AccessSpecifier ac, bool isStruct = false);
    // Get overloaded operator name & template args, such as { 'opBinary', 'string op:"+"'}
    static std::tuple<std::string, std::string, bool> getOperatorName(const clang::FunctionDecl* decl);
private:
    // returns "C" or "C++" depending on settings
    std::string externAsString(bool isExternC = false) const;
    void getJoinedNS(const clang::DeclContext* decl, std::vector<std::string>& parts);
    // Writes comma separted inheritance list, returns non-virtual bases to be used with 'alias this'
    void printBases(const clang::CXXRecordDecl* decl);
    std::vector<const clang::CXXRecordDecl*> getNonVirtualBases(const clang::CXXRecordDecl* decl);
    void handleMethods(const clang::CXXRecordDecl* decl);
    void handleFields(const clang::RecordDecl* decl);
    void handleInnerDecls(const clang::RecordDecl* decl);
    std::string getNextMixinId();

    // Functions below are specific to functions 
    void writeFnRuntimeArgs(const clang::FunctionDecl* fn);
    void writeTemplateArgs(const clang::TemplateDecl* td);
    void writeTemplateArgs(const clang::TemplateArgumentList* ta);
    void writeTemplateArgs(const clang::TemplateParameterList* tp);

    void writeFnBody(clang::FunctionDecl* fn, bool commentOut = false);

    
    // Try to insert entry into dict, return true on success.
    template <typename T, typename S>
    bool addType(const T* entry, S& dict)
    {
#if __cpp_lib_filesystem
namespace fs = std::filesystem;
#elif __cpp_lib_experimental_filesystem
namespace fs = std::experimental::filesystem;
#endif
        auto& SM = entry->getASTContext().getSourceManager();
        std::string loc = entry->getLocation().printToString(SM);
        auto [path, linecol] = getFSPathPart(loc);
        if (path.empty()) { // second attempt with different approach
            auto sfile = SM.getFileID(entry->getLocation());
            auto* f = SM.getFileEntryForID(sfile);
            if (f) path = f->getName();
        }
        if (path.empty())
            llvm::errs() << "addType() error: empty path for entry '" << entry->getName() << "'\n";

        loc = path + linecol;
        
        bool res = false;
		if ( dict.find(loc) == dict.end() )
		{
			dict.insert(std::make_pair(loc, entry));
            res = true;
		}
        return res;
    }
};


class NamespacePolicy
{
public:
    virtual ~NamespacePolicy() {}
    virtual void setGenerator(DlangBindGenerator* g) = 0;
    // Called before writing record with namespace
    virtual void beginEntry(const clang::Decl* decl, const std::string& extern_) = 0;
    // Called after record done writing
    virtual void finishEntry(const clang::Decl* decl) = 0;
};