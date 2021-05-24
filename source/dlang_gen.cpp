
#include "dlang_gen.h"

#include <sstream>
#include <iomanip>
#include <memory>
#include <unordered_set>

#include "llvm/Config/llvm-config.h"
#include "llvm/ADT/ScopeExit.h"
#include "llvm/ADT/ArrayRef.h"
#include "clang/AST/AST.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/Decl.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/Mangle.h"
#include "clang/AST/Expr.h"
#include "clang/AST/Stmt.h"
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
#include "clang/Sema/Sema.h"

#include "iohelpers.h"
#include "postedits.h"

#if __cpp_lib_filesystem
namespace fs = std::filesystem;
#elif __cpp_lib_experimental_filesystem
namespace fs = std::experimental::filesystem;
#endif


using namespace clang;
using namespace clang::ast_matchers;
using namespace clang::driver;
using namespace clang::tooling;
using namespace gentool;


thread_local clang::PrintingPolicy *DlangBindGenerator::g_printPolicy;

// D reserved keywords, possible overlaps from C/C++ sources
// Taken from https://dlang.org/spec/lex.html#keywords
std::unordered_set<std::string> reservedIdentifiers = {
  "align", "out", "ref", "version", "debug", "mixin", "with", "unittest", "typeof", "typeid", "super", "body",
  "shared", "pure", "package", "module", "inout", "in", "is", "import", "invariant", "immutable", "interface",
  "function", "delegate", "final", "export", "deprecated", "alias", "abstract", "synchronized",
  "byte", "ubyte", "uint", "ushort", "string",
  // Unsure how to format these
  "asm",
  "assert",
  "auto",
  "bool",
  "break",
  "case",
  "cast",
  "catch",
  "cdouble",
  "cent",
  "cfloat",
  "char",
  "class",
  "const",
  "continue",
  "creal",
  "dchar",
  "default",
  "delete",
  "do",
  "double",
  "else",
  "enum",
  "extern",
  "false",
  "finally",
  "float",
  "for",
  "foreach",
  "foreach_reverse",
  "goto",
  "idouble",
  "if",
  "ifloat",
  "int",
  "ireal",
  "lazy",
  "long",
  "macro",
  "new",
  "nothrow",
  "null",
  "override",
  "pragma",
  "private",
  "protected",
  "public",
  "real",
  "return",
  "scope",
  "short",
  "static",
  "struct",
  "switch",
  "template",
  "this",
  "throw",
  "true",
  "try",
  "ucent",
  "ulong",
  "union",
  "void",
  "wchar",
  "while",
  "__FILE__",
  "__FILE_FULL_PATH__",
  "__MODULE__",
  "__LINE__",
  "__FUNCTION__",
  "__PRETTY_FUNCTION__",
  "__gshared",
  "__traits",
  "__vector",
  "__parameters"
};

const char* MODULE_HEADER =
R"(
import core.stdc.config;
import std.bitmanip : bitfields;
import std.conv : emplace;

bool isModuleAvailable(alias T)() {
    mixin("import " ~ T ~ ";");
    static if (__traits(compiles, (mixin(T)).stringof))
        return true;
    else
        return false;
}
    
static if (__traits(compiles, isModuleAvailable!"nsgen" )) 
    static import nsgen;

struct CppClassSizeAttr
{
    alias size this;
    size_t size;
}
CppClassSizeAttr cppclasssize(size_t a) { return CppClassSizeAttr(a); }

struct CppSizeAttr
{
    alias size this;
    size_t size;
}
CppSizeAttr cppsize(size_t a) { return CppSizeAttr(a); }

struct CppMethodAttr{}
CppMethodAttr cppmethod() { return CppMethodAttr(); }

struct PyExtract{}
auto pyExtract(string name = null) { return PyExtract(); }

mixin template RvalueRef()
{
    alias T = typeof(this);
    static assert (is(T == struct));

    @nogc @safe
    ref const(T) byRef() const pure nothrow return
    {
        return this;
    }
}

)";

//
// Flattens hierarchy for nested types
//
std::string merge(std::list<const clang::RecordDecl *> &q)
{
    std::stringstream ss;

    for (const auto &item : q)
    {
        ss << item->getName().str();
        ss << "_";
    }

    return ss.str();
}

bool isReservedIdentifier(const std::string& id)
{
    // Implicit conversion, 0 => false, +1 => true
    return reservedIdentifiers.count(id);
}

// De-anonimize provided decl and all sub decls, will set generated identifier to the types
void deanonimizeTypedef(clang::RecordDecl* decl, const std::string_view optName = std::string_view(), int* count = nullptr)
{
    // TODO: This function is just ugly, refactor!
    int counter = 1; // will be appended to nested anonymous entries
    if (!count)
        count = &counter;
    if (!decl->getIdentifier())
    {
        if (optName.empty())
        {
            auto newName = std::string("_anon").append(std::to_string(*count));
            count += 1;
            const auto& newId = decl->getASTContext().Idents.get(newName);
            decl->setDeclName(DeclarationName(&newId));
        }
        else
        {
            const auto& newId = decl->getASTContext().Idents.get(optName.data());
            decl->setDeclName(DeclarationName(&newId));
        }
    }
    for(auto d : decl->decls())
    {
        if (d == decl)
            continue;

        if (auto td = llvm::dyn_cast<TypedefDecl>(d))
        {
            if (!td->getIdentifier()) // no name, add some
            {
                auto newName = std::string("_anon").append(std::to_string(*count));
                *count += 1;
                const auto& newId = td->getASTContext().Idents.get(newName);
                td->setDeclName(DeclarationName(&newId));
            }
            if (auto tdtype = td->getUnderlyingType().getTypePtr())
            {
                if (tdtype->isDependentType())
                    continue;
                if (auto rd = tdtype->getAsRecordDecl() )
                {
                    if (isa<ClassTemplateSpecializationDecl>(rd))
                        continue;
                    deanonimizeTypedef(rd);
                }
            }
        }
        else if (auto rec = llvm::dyn_cast<RecordDecl>(d))
        {
            if (!rec->getIdentifier())
            {
                auto newName = std::string("_anon").append(std::to_string(*count));
                *count += 1;
                const auto& newId = rec->getASTContext().Idents.get(newName);
                rec->setDeclName(DeclarationName(&newId));
            }
        }
    }
}


void printQualifier(NestedNameSpecifier* qualifier, llvm::raw_ostream& os)
{
    switch (qualifier->getKind()) 
    {
    case NestedNameSpecifier::Identifier:
        os << qualifier->getAsIdentifier()->getName();
        break;
    //case NestedNameSpecifier::Namespace:
    //    if (!q->getAsNamespace()->isAnonymousNamespace())
    //        os << q->getAsNamespace()->getName();
    //    break;
    case NestedNameSpecifier::NamespaceAlias:
        os << qualifier->getAsNamespaceAlias()->getName();
        break;
    case NestedNameSpecifier::TypeSpecWithTemplate:
    case NestedNameSpecifier::TypeSpec: {
        const Type *T = qualifier->getAsType();
        if (const TemplateSpecializationType *SpecType = dyn_cast<TemplateSpecializationType>(T)) 
        {
            SpecType->getTemplateName().print(os, *DlangBindGenerator::g_printPolicy, true);
            printDTemplateArgumentList(os, SpecType->template_arguments(), *DlangBindGenerator::g_printPolicy);
        } else {
            os << DlangBindGenerator::toDStyle(QualType(T, 0));
        }
        break;
        }
    }
}


// Replaces C++ token like arrow operator or scope double-colon
std::string& textReplaceArrowColon(std::string& in)
{
    while (true)
    {
        auto pos = in.rfind("::");
        if (pos == std::string::npos)
            pos = in.rfind("->");
            if (pos == std::string::npos)
                break;
        in.replace(pos, 2, ".");
    }
    return in;
}


std::string stripRefFromParam(const std::string& s) 
{
    auto p = s.find("ref ");
    if (p == 0)
        return std::string(s).replace(0, 4, "/*ref*/ ");
    return s;
}


std::string intTypeForSize(unsigned bitWidth, bool signed_ = true)
{
    if (bitWidth == 8) return signed_? "byte" : "ubyte";
    if (bitWidth == 16) return signed_? "short" : "ushort";
    if (bitWidth == 32) return signed_? "int" : "uint";
    if (bitWidth == 64) return signed_? "long" : "ulong";
    return signed_? "int" : "uint";
}


class NamespacePolicy_StringList : public NamespacePolicy
{
    DlangBindGenerator* gen;
    void setGenerator(DlangBindGenerator* g) { gen = g; }
    void beginEntry(const clang::Decl* decl, const std::string& extern_)
    {
        auto& out = gen->out;
        std::vector<std::string> nestedNS;
        gen->getJoinedNS(decl->getDeclContext(), nestedNS);

        out << "extern(" << extern_ << ", ";
        auto it = std::rbegin(nestedNS);
        while (it != nestedNS.rend())
        {
            out << "\"" << *it << "\""; if (it != (nestedNS.rend() - 1)) out << ",";
            it++;
        }
        out << ")" << std::endl;
    }
    void finishEntry(const clang::Decl* decl) {}
};


void nsfileOpen(const std::string_view path, std::ofstream& file)
{
    fs::path p(path);
    p.replace_extension("");
    auto name = p.filename().string() + "_ns";
    p.replace_filename(name);
    p.replace_extension(".d");
    file.open(p.string());
}


bool hasVirtualMethods(const RecordDecl* rd)
{
    if (!rd)
        return false;
    if (!isa<CXXRecordDecl>(rd))
        return false;

    auto isVirtual = [](const CXXMethodDecl* f) {
        return f->isVirtual() == true;
    };

    const auto rec = cast<CXXRecordDecl>(rd);
    auto found = std::find_if(rec->method_begin(), rec->method_end(), isVirtual) != rec->method_end();
    if (!rec->getDefinition())
        return found;
    return found || std::any_of(rec->bases_begin(), rec->bases_end(), [](auto a){ return hasVirtualMethods(a.getType()->getAsRecordDecl()); });
}

bool isPossiblyVirtual(const RecordDecl* rd)
{
    return hasVirtualMethods(rd);
}

// Check for possible overrides. 
// Including cases where override attribute is not present or not even virtual
bool overridesBaseOf(const FunctionDecl* fn, const CXXRecordDecl* rec)
{
    static auto isStatic = [] (const FunctionDecl* a) {
        return isa<CXXMethodDecl>(a) && cast<CXXMethodDecl>(a)->isStatic();
    };
    static auto isSame = [] (const FunctionDecl* a, const FunctionDecl* b) -> bool {
        if (a->getAccess() == AccessSpecifier::AS_private || b->getAccess() == AccessSpecifier::AS_private)
            return false;
        if (isStatic(a) || isStatic(b))
            return false;
        if (a->getNumParams() != b->getNumParams())
            return false;
        if (a->getDeclKind() != b->getDeclKind())
            return false;
        if (a->getIdentifier() && b->getIdentifier())
            if (a->getName() != b->getName())
                return false;
        if (a->getNumParams() == 0 && b->getNumParams() == 0)
            return true;
        return std::equal(a->param_begin(), a->param_end(), b->param_begin(), b->param_end(), 
            [](const ParmVarDecl* p1, const ParmVarDecl* p2) {
                // return true if TYPES does MATCH, OR if their NAMES are EQUAL (but only if present)
                return p1->getType() == p2->getType() 
                || (!p1->getName().empty() && !p2->getName().empty() 
                    && (p1->getName() == p2->getName()));
        });
    };
    if (rec && rec->getDefinition())
    for (auto b : rec->bases())
    {
        const CXXRecordDecl* bc = nullptr;
        if (b.getType()->getTypeClass() == Type::TemplateSpecialization)
        {
            auto tp = b.getType().getTypePtr();
            auto tst = tp->getAs<TemplateSpecializationType>();
            if (auto td = tst->getTemplateName().getAsTemplateDecl())
            {
                if (auto tdec = td->getTemplatedDecl(); tdec && isa<CXXRecordDecl>(tdec))
                    bc = cast<CXXRecordDecl>(tdec);
            }
        }
        else
            bc = b.getType()->getAsCXXRecordDecl();

        if (!bc)
            continue;

        for(const auto m : bc->methods())
        {
            if (isSame(fn, m))
                return true;
        }
        if (overridesBaseOf(fn, bc))
            return true;
    }
    return false;
}

// In D classes are reference types, 
// to make it D way it needs one level of indirection stripped.
// NOTE: type parameter internally is a reference and will be modified
bool isClassPointer(clang::QualType type)
{
    while (type->isPointerType())
    {
        type = type->getPointeeType();
        if (type->isClassType() 
            && type->isRecordType() 
            && isPossiblyVirtual(type->getAsRecordDecl()))
        {
            return true;
        }
    }
    return false;
}

clang::QualType adjustForVariable(clang::QualType ty, clang::ASTContext* ctx)
{
    if (ty->isReferenceType() && ctx)
    {
        // Convert non-class refs to pointers
        // NOTE: this won't affect sizeof attribute,
        //  in some cases (GNU GCC?) it might differ from actual memory layout
        auto newtype = ctx->getPointerType(ty->getPointeeType());
        return newtype;
    }
    return ty;
}


void parseLateTemplate(FunctionDecl* fntemplate, clang::Sema* sema)
{
    auto& LPT = *sema->LateParsedTemplateMap.find(fntemplate)->second;
    sema->LateTemplateParser(sema->OpaqueParser, LPT);
}


std::unique_ptr<MangleContext> makeManglingContext(ASTContext* ast)
{
    std::unique_ptr<MangleContext> result;
    if (ast->getTargetInfo().getTargetOpts().Triple.find("windows") != std::string::npos)
        result.reset(clang::MicrosoftMangleContext::create(*ast, ast->getDiagnostics()));
    else
        result.reset(clang::ItaniumMangleContext::create(*ast, ast->getDiagnostics()));
    return result;
}


std::string getMangledName(const Decl* decl, MangleContext* ctx)
{
    std::string mangledName;
    llvm::raw_string_ostream ostream(mangledName);
#if (LLVM_VERSION_MAJOR >= 11)
    GlobalDecl GD;
#endif

    if (const CXXConstructorDecl *ctorDecl = dyn_cast<CXXConstructorDecl>(decl))
      #if (LLVM_VERSION_MAJOR < 11)
        ctx->mangleCXXCtor(ctorDecl, CXXCtorType::Ctor_Complete, ostream);
      #else
        GD = GlobalDecl(ctorDecl, CXXCtorType::Ctor_Complete);
      #endif

    else if (const CXXDestructorDecl *dtorDecl = dyn_cast<CXXDestructorDecl>(decl))
      #if (LLVM_VERSION_MAJOR < 11)
        ctx->mangleCXXDtor(dtorDecl, CXXDtorType::Dtor_Complete, ostream);
      #else
        GD = GlobalDecl(dtorDecl, CXXDtorType::Dtor_Complete);
      #endif

    else if (isa<FunctionDecl>(decl))
      #if (LLVM_VERSION_MAJOR < 11)
        ctx->mangleName(cast<FunctionDecl>(decl), ostream);
      #else
        GD = GlobalDecl(cast<FunctionDecl>(decl));
      #endif

#if (LLVM_VERSION_MAJOR >= 11)
    ctx->mangleName(GD, ostream);
#endif
    
    return ostream.str();
}

// Chooses wysiwyg string delimiter depending on macro content
char macroWysiwygDelimiter(const clang::MacroInfo* macro)
{
    if (!macro)
        return '`';
    
    auto hasCharacter = [macro] (char delim) -> bool {
        for (auto tok : macro->tokens())
        {
            if (tok.isLiteral())
            {
                auto literal = std::string_view(tok.getLiteralData(), tok.getLength());
                if (literal.find_first_of(delim, 0) != std::string_view::npos)
                    return true;
            }
        }
        return false;
    };

    // delimiters to choose from
    std::string delims = "`$%^#&/";

    auto delim = std::find_if_not(delims.begin(), delims.end(), hasCharacter);
    if (delim != delims.end())
        return *delim;

    return '`';
}

std::string macroToString(const clang::MacroInfo* macro)
{
    std::string s;
    llvm::raw_string_ostream os(s);

    for (auto tok : macro->tokens())
    {
        if (tok.isAnyIdentifier())
        {
            os << DlangBindGenerator::sanitizedIdentifier( std::string(tok.getIdentifierInfo()->getName()));
        }
        else if (tok.isLiteral())
        {
            os << std::string(tok.getLiteralData(), tok.getLength());
        }
        else if (auto kw = clang::tok::getKeywordSpelling(tok.getKind()))
        {
            os << kw;
        }
        else if (auto pu = clang::tok::getPunctuatorSpelling(tok.getKind()))
        { 
            static const std::vector<clang::tok::TokenKind> wstokens = { // tokens that needs ws after it
                clang::tok::comma, clang::tok::r_paren, clang::tok::r_brace, clang::tok::semi
            };
            os << pu;
            //bool ws = std::find(wstokens.begin(), wstokens.end(), tok.getKind()) != wstokens.end();
            //if (ws) os << " ";
        }
    }

    return os.str();
}

// Indicates that macro is likely be a literal value or expression, and does not contains keywords
bool isPrimitiveMacro(const clang::MacroInfo* macro)
{
    static const std::vector<clang::tok::TokenKind> otherAllowedTokens = {
        tok::period, tok::comma, tok::arrow, tok::l_paren, tok::r_paren, tok::l_square, tok::r_square, tok::coloncolon
    };
    static const std::vector<clang::tok::TokenKind> opTokens = {
        tok::plus, tok::minus, tok::star, tok::slash,
        tok::percent, tok::tilde, tok::caret, tok::amp, tok::pipe,
        tok::ampamp, tok::pipepipe, tok::exclaim,
        tok::lessless, tok::greatergreater, tok::greatergreatergreater
    };
    static auto isLiteralOrOperator = [](const Token tok) -> bool {
        return !tok.isAnyIdentifier() 
            && nullptr == clang::tok::getKeywordSpelling(tok.getKind())
            && (tok.isLiteral() 
                || (std::find(opTokens.begin(), opTokens.end(), tok.getKind()) != opTokens.end())
                || (std::find(otherAllowedTokens.begin(), otherAllowedTokens.end(), tok.getKind()) != otherAllowedTokens.end())
            )
        ;
    };
    auto b = macro->tokens_begin();
    auto e = macro->tokens_end();
    return std::all_of(b, e, [](Token tok) { return isLiteralOrOperator(tok); });
}

// Retrieves body definition of a base function (or base if not present)
// It is needed to emit actual parameter names since they might differ from declaration
// example:
//   class A { void doStuff(int i); }
//   ...
//   void A::doStuff(int value) { ... }
// will give second method with actual implementation
const FunctionDecl* getFnBody(const FunctionDecl* base)
{
    const FunctionDecl* fndef = nullptr;
    base->getBody(fndef);
    if (!fndef)
        fndef = base;
    return fndef;
}

bool DlangBindGenerator::isRelevantPath(const std::string_view path)
{
    if (path.size() == 0 || path.compare("<invalid loc>") == 0) 
        return false;

    auto [fullPath, _] = getFSPathPart(path);
    auto file = fs::path(fullPath);
    for (const auto &p : iops->paths)
    {
        auto inpath = fs::canonical(p);
        if (file.string().rfind(inpath.string()) != std::string::npos)
            return true;
    }
    return false;
}


void DlangBindGenerator::setOptions(const InputOptions *inOpt, const OutputOptions *outOpt)
{
    if (inOpt)
    {
        iops = inOpt;
        cppIsDefault = inOpt->standard.rfind("c++") != std::string::npos;
    }
    if (outOpt)
    {
        outops = outOpt;
        //if (fileOut.is_open())
        //    fileOut.close();
        //fileOut.open(outOpt->path);
        nsfileOpen(outOpt->path, mangleOut);
        if (std::find(outOpt->extras.begin(), outOpt->extras.end(), "attr-nogc") != outOpt->extras.end())
            nogc = true;
        if (std::find(outOpt->extras.begin(), outOpt->extras.end(), "no-param-refs") != outOpt->extras.end())
            stripRefParam = true;
        if (std::find(outOpt->extras.begin(), outOpt->extras.end(), "skip-bodies") != outOpt->extras.end())
            skipBodies = true;
        if (std::find(outOpt->extras.begin(), outOpt->extras.end(), "mangle-all") != outOpt->extras.end())
            mangleAll = true;

        // TODO: select valid policy here
        //nsPolicy.reset(new NamespacePolicy_StringList());
    }

    if (!nsPolicy)
        nsPolicy = std::make_unique<NamespacePolicy_StringList>();
    nsPolicy->setGenerator(this);
}


void DlangBindGenerator::prepare()
{
    feedback = new FeedbackContext();
    mixinTemplateId = 1;
    out << MODULE_HEADER;

    out << std::endl;
}


void DlangBindGenerator::finalize()
{
    for(auto f : forwardTypes)
    {
        if (f.second)
            out << "struct " << f.first << ";" << std::endl;
    }

    // rewind the file to add requiring mixins
    std::set<std::string> rvals;
    std::vector<AddRvalueHackAction*> rvas;
    for(std::unique_ptr<FeedbackAction>& p : feedback->actions)
        if (auto a = dyn_cast<AddRvalueHackAction>(p.get()))
        {
            if (rvals.find(a->declName) != rvals.end())
                continue;
            rvas.push_back(a);
            rvals.insert(a->declName);
        }

    std::remove_if(rvas.begin(), rvas.end(),
        [this](const AddRvalueHackAction* a) {
            return declLocations.find(a->declName) == declLocations.end();
        }
    );
    std::sort(rvas.begin(), rvas.end(),
        [this](const AddRvalueHackAction* lhs, const AddRvalueHackAction* rhs) {
            if (lhs->declName == rhs->declName)
                return false;
            auto lpos = declLocations.find(lhs->declName);
            auto rpos = declLocations.find(rhs->declName);
            if (lpos != declLocations.end() && rpos != declLocations.end())
                return lpos->second.line > rpos->second.line; // descending order
            return false;
        }
    );
    for(auto a : rvas)
        a->apply(this);

    // well, that sucks
    // TODO: fix duplication
    std::set<std::string> impmod;
    std::vector<AddImportAction*> imports;
    for(auto& a : feedback->actions)
    {
        if (auto addImport = dyn_cast<AddImportAction>(a.get()))
        {
            if (impmod.find(addImport->module) != impmod.end())
                continue;
            imports.push_back(addImport);
            impmod.insert(addImport->module);
        }
    }
    for (auto a : imports)
        a->apply(this);

    std::ofstream out;
    out.open(outops->path);
    out << bufOut.str();
    out.flush();
}

void DlangBindGenerator::onMacroDefine(const clang::Token* name, const clang::MacroDirective* macro)
{
    static const int LONG_MACRO_NUM_TOKENS = 120;
    char wysiwygDelim = macroWysiwygDelimiter(macro->getMacroInfo());
    auto formatMacroOpen = [wysiwygDelim] (const clang::MacroInfo* mi, const llvm::StringRef macroName) {
        std::string s;
        llvm::raw_string_ostream os(s);
        os << "enum " << macroName;
        if (mi->getNumParams())
            os << "(";
        for (auto p : mi->params())
        {
            os << sanitizedIdentifier(p->getName().str());
            if (p != *(mi->param_end()-1)) os << ", ";
        }
        if (mi->getNumParams())
            os << ")";
        os << " = ";
        if (mi->getNumParams() || !isPrimitiveMacro(mi))
        {
            // write opening for wysiwyg string long form
            if (wysiwygDelim != '`')
                os << "q\"";
            os << wysiwygDelim;
        }
        return os.str();
    };
    auto formatMacroClose = [wysiwygDelim] (const clang::MacroInfo* mi) {
        std::string s;
        llvm::raw_string_ostream os(s);
        if (mi->getNumParams() || !isPrimitiveMacro(mi))
        {
            os << wysiwygDelim;
            // write closing for wysiwyg string long form
            if (wysiwygDelim != '`')
                os << '"';
        }
        os << ";";
        return os.str();
    };

    if (!macro)
        return;

    auto path = macro->getLocation().printToString(*SourceMgr);
    if (!isRelevantPath(path))
        return;

    if (auto mi = macro->getMacroInfo())
    {
        if (mi->isUsedForHeaderGuard() || mi->getNumTokens() == 0)
            return;
        
        if (!name || !name->getIdentifierInfo())
            return;

        auto id = name->getIdentifierInfo()->getName();
        if ( macroDefs.find(id.str()) != macroDefs.end() )
            return;
        else 
            macroDefs.insert(std::make_pair(id.str(), true));

        if (mi->getNumTokens() < LONG_MACRO_NUM_TOKENS)
        {
            out << formatMacroOpen(mi, id);
            out << macroToString(mi);
            out << formatMacroClose(mi);
            out << std::endl;
        }
        else // 'long' macro
        {
            out << "//" << path << std::endl;
            out << "//#define " << id.str()
                << " ..." << std::endl;
        }
    }
}

void DlangBindGenerator::onInclude(
        StringRef FileName, 
        StringRef RelativePath, 
        StringRef SearchPath, 
        bool IsAngled, 
        clang::SrcMgr::CharacteristicKind FileType)
{
    auto mod = headerModuleMap->find(FileName.str());
    if (!mod.empty())
    {
        feedback->addAction(std::move(std::make_unique<AddImportAction>(mod)));
    }
}

void DlangBindGenerator::onBeginFile(const std::string_view file)
{
    out << std::endl;
    out << "// ------ " << file << std::endl;
    out << std::endl;
}


void DlangBindGenerator::onEndFile(const std::string_view file)
{
}


void DlangBindGenerator::onStructOrClassEnter(const clang::RecordDecl *decl)
{
    classOrStructName = decl->getName().str();
    finalTypeName = merge(declStack).append(classOrStructName);
    declStack.push_back(decl);

    bool forwardDecl = decl->getBraceRange().isInvalid();
    if (forwardTypes.find(classOrStructName) == forwardTypes.end())
        forwardTypes.emplace(std::make_pair(classOrStructName, forwardDecl));

    if (forwardDecl)
        return;

    if (!addType(decl, storedTypes))
        return;

    if (auto fw = forwardTypes.find(classOrStructName); fw != forwardTypes.end() && fw->second == true)
    {
        fw->second = forwardDecl;
    }

    handledClassDecl = true;
    
    const bool hasNamespace = decl->getDeclContext()->isNamespace();
    const auto externStr = externAsString(decl->getDeclContext()->isExternCContext());
    const bool isClassMangle = (decl->isClass() && !isPossiblyVirtual(decl));

    // linkage & namespace
    if (!hasNamespace)
        out << "extern(" << externStr << (isClassMangle ? ", class)" : ")") << std::endl;
    else
        nsPolicy->beginEntry(decl, externStr);


    TypeInfo ti;
    if (!decl->isTemplated()) // skip templated stuff for now
    {
        const auto tdd = decl->getTypeForDecl();
        if (!tdd)
            return;
        if (!decl->isCompleteDefinition())
        {
            // write and close forward decl
            out << "struct " << decl->getName().str() << ";" << std::endl;
            return;
        }
        ti = decl->getASTContext().getTypeInfo(decl->getTypeForDecl());
        if (ti.Width == 0 || ti.Align == 0)
            return;

        // size attr for verifying
        out << "@cppclasssize(" << ti.Width / 8 << ")"
            << " align(" << ti.Align / 8 << ")" << std::endl;
    }

    const auto cxxdecl = llvm::dyn_cast<CXXRecordDecl>(decl);
    bool isDerived = false;
    if (cxxdecl && (cxxdecl->getNumBases() || cxxdecl->getNumVBases()))
        isDerived = true;

    bool isVirtual = isPossiblyVirtual(decl);
    if (isVirtual)
        out << "class ";
    else if (decl->isUnion())
        out << "union ";
    else
        out << "struct ";

    if (classOrStructName.empty())
    {
        if (auto td = decl->getTypedefNameForAnonDecl())
        {
            classOrStructName = td->getName().str();
            const auto& newId = decl->getASTContext().Idents.get(td->getName());
            const_cast<RecordDecl*>(decl)->setDeclName(DeclarationName(&newId));
        }
        else
        {
            globalAnonTypeId += 1;
            deanonimizeTypedef(
                const_cast<RecordDecl*>(decl), 
                std::string("AnonType_").append(std::to_string(globalAnonTypeId))
            );
            classOrStructName = decl->getName().str();
        }
    }
    else
    {
        deanonimizeTypedef(const_cast<RecordDecl*>(decl), std::string_view(), &localAnonRecordId);
    }

    out << classOrStructName;

    if (cxxdecl)
    {
        TemplateDecl* tpd = nullptr;
        const TemplateArgumentList* tparams = nullptr;
        const ClassTemplateSpecializationDecl* tsd = nullptr;
        const ClassTemplatePartialSpecializationDecl* tps = dyn_cast<ClassTemplatePartialSpecializationDecl>(decl);
        if (isa<ClassTemplateSpecializationDecl>(decl))
            tsd = cast<ClassTemplateSpecializationDecl>(decl);
        if (tsd)
        {
            tparams = &tsd->getTemplateArgs();
            tpd = tsd->getSpecializedTemplate();
        }
        else
        {
            tpd = cxxdecl->getDescribedClassTemplate();
        }

        if ( decl->isThisDeclarationADefinition() ) // add only actual declarations
        {
            if (tps)
            {
                out << "(";
                writeTemplateArgs(tps->getTemplateParameters());
                out << ")";
            }
            else if (tparams)
            {
                out << "(";
                writeTemplateArgs(tparams);
                out << ")";
            }
            else if (tpd)
            {
                out << "(";
                writeTemplateArgs(tpd);
                out << ")";
            }
        }
        // adds list of base classes
        if (isVirtual)
            printBases(cxxdecl);
    }
    out << std::endl;
    out << "{" << std::endl;
    {
        IndentBlock _classbody(out, 4);
        if (decl->getIdentifier())
            declLocations.insert(std::make_pair(decl->getNameAsString(), DeclLocation((size_t)bufOut.tellp(), (size_t)out._indent)));
#if 0
        // Nope, this does not conform to actual layout, 
        // that means we have to apply align on every field instead
        if (!(decl->isUnion() || decl->isTemplated()))
            out << "align(" << ti.Align / 8 << "):" << std::endl;
#endif
        // TODO: precise mix-in where needed on finalize step
        //if (!isVirtual)
        //    out << "mixin RvalueRef;" << std::endl;
        handleInnerDecls(decl);
        int baseid = 0;
        if (isVirtual)
        {
            for (auto fakeBase : getNonVirtualBases(cxxdecl))
            {
                if (!isa<RecordDecl>(fakeBase))
                    continue;
                auto brd = cast<RecordDecl>(fakeBase);
                out << brd->getNameAsString() << " ";
                out << "_b" << baseid << ";" << std::endl;
                out << "alias " << "_b" << baseid << " this;" << std::endl;
                baseid+=1;
            }
        }
        else if (cxxdecl)
        {
            unsigned int numBases = cxxdecl->getNumBases();
            for (const auto b : cxxdecl->bases())
            {
                numBases--;
                RecordDecl* rd = b.getType()->getAsRecordDecl(); 
                if (rd && !isPossiblyVirtual(cast<RecordDecl>(rd)))
                    out << rd->getNameAsString() << " ";
                // templated base class
                else if (b.getType()->getTypeClass() == Type::TypeClass::TemplateSpecialization)
                    out << toDStyle(b.getType()) << " ";

                out << "_b" << baseid << ";" << std::endl;
                if (baseid == 0) 
                    out << "alias " << "_b" << baseid << " this;" << std::endl;
                baseid+=1;
            }
        }
        handleFields(decl);
        if (cxxdecl)
            handleMethods(cxxdecl);
    }
    out << "}" << std::endl;

    if (hasNamespace)
        nsPolicy->finishEntry(decl);
}


void DlangBindGenerator::onStructOrClassLeave(const clang::RecordDecl *decl)
{
    declStack.pop_back();

    if (declStack.empty())
        localAnonRecordId = 1;

    if (handledClassDecl)
        out << std::endl 
            << std::endl;

    handledClassDecl = false;
}


void DlangBindGenerator::onEnum(const clang::EnumDecl *decl)
{
    if (!addType(decl, enumDecls))
        return;

    const auto size = std::distance(decl->enumerator_begin(), decl->enumerator_end());
    auto enumTypeString = toDStyle(decl->getIntegerType());

    if (auto td = decl->getTypedefNameForAnonDecl())
    {
        addType(cast<TypedefDecl>(td), storedTypes);
        if (cppIsDefault && td->getUnderlyingType().getTypePtr() != decl->getTypeForDecl())
        {
            enumTypeString = toDStyle(td->getUnderlyingType());
            out << "alias " << enumTypeString << " = " << toDStyle(decl->getIntegerType()) << ";" << std::endl;
        }
    }

    bool hasName = !decl->getName().empty();
    if (hasName)
    {
        // enum myEnum
        out << "enum " << decl->getName().str() << std::endl;
    }
    else
    {
        out << "enum"
            << " : " << enumTypeString << std::endl;
    }

    out << "{" << std::endl;

    for (const auto e : decl->enumerators())
    {
        out << "    ";
        out << e->getNameAsString() << " = " << e->getInitVal().toString(10, true);
        if (size > 1)
            out << ", ";
        out << std::endl;
    }

    out << "}" << std::endl;
    out << std::endl;

    // set up aliases to mimic C++ enum scope
    if (!decl->isScoped() && hasName)
    {
        for (const auto e : decl->enumerators())
        {
            // alias ENUMERATOR = myEnum.ENUMERATOR;
            out << "alias " << e->getName().str() 
                << " = " << decl->getName().str() << "." << e->getNameAsString() << ";";
            out << std::endl;
        }
        out << std::endl;
    }
}


void DlangBindGenerator::onFunction(const clang::FunctionDecl *decl)
{
    if (!decl->getIdentifier())
        return;

    if (!addType(decl, functionDecls))
        return;

    const auto fn = decl;
    const bool hasNamespace = decl->getDeclContext()->isNamespace();
    const auto externStr = externAsString(decl->getDeclContext()->isExternCContext());


    // linkage & namespace
    if (!hasNamespace)
        out << "extern(" << externStr << ")" << std::endl;
    else
        nsPolicy->beginEntry(decl, externStr);

    // pragma mangle
    if (!fn->isTemplated())
    {
        if ((mangleAll && externStr == "C++") || isReservedIdentifier(fn->getName().str()))
        {
            clang::ASTContext& ast = fn->getASTContext();
            std::unique_ptr<MangleContext> mangleCtx = makeManglingContext(&ast);
            out << "pragma(mangle, \"" << getMangledName(fn, mangleCtx.get()) << "\")" << std::endl;
        }
    }

    // ret type
    {
        bool shouldStripIndirection = isClassPointer(fn->getReturnType());
        auto adjustedType = shouldStripIndirection ? fn->getReturnType()->getPointeeType() : fn->getReturnType();
        const auto typeStr = toDStyle(adjustedType);
        out << typeStr << " ";
    }
    // function name
    out << sanitizedIdentifier(fn->getName().str());

    if (fn->isTemplated())
    {
        // note that we write to already opened parenthesis
        if (const auto ftd = fn->getDescribedTemplate())
        {
            out << "(";
            writeTemplateArgs(ftd);
            out << ")";
        }
    }

    // argument list
    out << "(";
    writeFnRuntimeArgs(getFnBody(fn));
    out << ")";
    if (nogc)
        out << " @nogc";

    if (!skipBodies && fn->hasBody())
        writeFnBody(const_cast<FunctionDecl*>(fn));
    else
        out << ";" << std::endl;

    if (hasNamespace)
        nsPolicy->finishEntry(decl);

    out << std::endl;
}


void DlangBindGenerator::onTypedef(const clang::TypedefDecl *decl)
{
    if (!addType(decl, storedTypes))
        return;

    const auto typedefName = decl->getName().str();
    bool extC = decl->getDeclContext()->isExternCContext();
    bool functionType = decl->getUnderlyingType()->isFunctionType();
    if (decl->getUnderlyingType()->isPointerType())
        functionType = decl->getUnderlyingType()->getPointeeType()->isFunctionType();

    if (auto tdtype = decl->getUnderlyingType().getTypePtr())
    {
        while(tdtype->isPointerType())
            tdtype = tdtype->getPointeeType().getTypePtr();

        auto rd = tdtype->getAsRecordDecl();
        if (rd)
        {
            if(!rd->getIdentifier())
            {
                deanonimizeTypedef(rd, typedefName);
                onStructOrClassEnter(rd);
                onStructOrClassLeave(rd);
                // We're done here, but it also might be an option to append 
                // underscore and add original typedef'ed name as alias
                return; 
            }
            else if (rd->getNameAsString() == typedefName)
                return;
        }

        // write enum but don't write alias for typedef
        if (auto td = tdtype->getAsTagDecl())
        {
            if (auto enumdecl = dyn_cast<EnumDecl>(td))
            {
                if (!cppIsDefault) {
                    onEnum(enumdecl);
                    return;
                }
            }
        }
    }

    out << "alias " << typedefName << " = ";
    if (functionType)
    {
        out << "extern(" << externAsString(extC) << ") ";
    }
    if (decl->getUnderlyingType()->isEnumeralType())
        out << "int;";
    else
        out << toDStyle(decl->getUnderlyingType()) << ";";
    out << std::endl;

    out << std::endl;
}


void DlangBindGenerator::onGlobalVar(const clang::VarDecl *decl)
{
    if (!addType(decl, storedTypes))
        return;

    bool extC = decl->isExternC();
    bool isExtern = decl->getStorageClass() == StorageClass::SC_Extern;
    bool isStatic = decl->getStorageClass() == StorageClass::SC_Static;

    bool isFnType = decl->getType()->isFunctionType();

    if (isFnType)
    {
        out << "extern(" << externAsString(extC) << ") ";
    }
    if (isExtern)
        out << "extern ";
    if (isStatic)
        out << "__gshared static "; // has no effect on module level, so probably will need to handle this somehow
    out << toDStyle(decl->getType()) << " ";
    out << decl->getName().str() << " ";
    if (const auto init = decl->getInit())
    {
        std::string s;
        llvm::raw_string_ostream os(s);
        printPrettyD(init, os, nullptr, *DlangBindGenerator::g_printPolicy, 0, &decl->getASTContext(), feedback);
        out << " = ";
        out << os.str();
    }
    out << ";";
    out << std::endl;
}


// Wraps complex multiple level pointers in parenthesis
std::string DlangBindGenerator::_wrapParens(QualType type)
{
    QualType temp = type;
    std::vector<std::string> parts;
    std::string result;

    // if multilevel pointer, pointer to const, etc...
    if (type->isPointerType() || type->isReferenceType())
    {
        _typeRoll(type, parts);

        if (parts.size() > 0)
        {
            result.clear();
            for (int i = 0; i < parts.size(); i++)
            {
                result.append(parts.at(i));
            }
        }
    }
    else
        result = toDStyle(type);
    return result;
}


// Supposed to recusrsively walk down, and then on walk up write reversed parts
// e.g. const float const * -> const(const(float)*)*
void DlangBindGenerator::_typeRoll(QualType type, std::vector<std::string> &parts)
{
    bool isConst = type.isConstQualified();
    if (type->isReferenceType())
    {
        parts.push_back("ref ");
        type = type->getPointeeType();
        _typeRoll(type, parts);
        return;
    }
    if (isConst)
        parts.push_back("const(");

    if (type->isPointerType())
        _typeRoll(type->getPointeeType(), parts);
    else
    {
        if (type->isReferenceType())
            parts.push_back("ref ");

        if (isConst)
            type.removeLocalConst();

        parts.push_back(toDStyle(type));
        // if (isConst)
        // 	type.addConst();
    }

    // This added complexity is for having matching pairs in parts list
    if (isConst && type->isPointerType())
        parts.push_back(")*");
    else if (isConst)
        parts.push_back(")");
    else if (type->isPointerType())
        parts.push_back("*");
}


std::string DlangBindGenerator::toDStyle(QualType type)
{
    std::string res;
    type.removeLocalRestrict();
    type.removeLocalVolatile();
    const auto typeptr = type.getTypePtr();

    if (type->isPointerType() || type->isReferenceType())
    {
        if (type->getPointeeType()->isFunctionType())
            return toDStyle(type->getPointeeType());
        res = _wrapParens(type);
    }
    else if (const auto pt = llvm::dyn_cast<ParenType>(typeptr))
    {
        res = toDStyle(pt->getInnerType());
    }
    else if (const auto td = llvm::dyn_cast<TypedefType>(typeptr))
    {
        res = td->getDecl()->getNameAsString();
    }
    else if (type->isArrayType())
    {

        if (type->isConstantArrayType())
        {
            if (const auto arr = llvm::dyn_cast<ConstantArrayType>(type))
            {
                std::string _res = toDStyle(arr->getElementType());
                // do pointer replace with 1-sized arrays?
                _res.append("[");
                _res.append(arr->getSize().toString(10, false));
                _res.append("]");
                res = _res;
            }
        }
        else if (const auto arr = llvm::dyn_cast<ArrayType>(type))
        {
            std::string _res = toDStyle(arr->getElementType());
            res.append("[]");
            res = _res;
        }
    }
    else if (type->isFunctionType())
    {
        const auto fp = type->getAs<FunctionProtoType>();

        auto psize = fp->getNumParams();
        const auto ret = fp->getReturnType();
        std::string s;
        s.append(toDStyle(ret));
        s.append(" function(");
        for (const auto p : fp->param_types())
        {
            s.append(toDStyle(p));
            // TODO: extract parameter names from AST?
            //s.append(" ");
            //s.append(p->getName().str());
            if (psize > 1)
            {
                s.append(", ");
                psize -= 1;
            }
        }
        s.append(")");
        res = s;
    }
    else if (typeptr->getTypeClass() == Type::TypeClass::Elaborated)
    {
        auto et = typeptr->getAs<ElaboratedType>();
        std::string s;
        llvm::raw_string_ostream os(s);
        // print nested name
        if (auto q = et->getQualifier())
        {
            printQualifier(q, os);
            if (q->getKind() != NestedNameSpecifier::SpecifierKind::Namespace)
                os << ".";
        }
        os << toDStyle(et->getNamedType());
        res = os.str();
    }
    else if (type->isBuiltinType())
    {
        res = _toDBuiltInType(type);
    }
    else if (const auto tsp = type->getAs<TemplateSpecializationType>()) // matches MyArray<int> field
    {
        std::string s;
        llvm::raw_string_ostream os(s);
        tsp->getTemplateName().print(os, *DlangBindGenerator::g_printPolicy);
        os << "!"
           << "(";
        //s.clear();
        const auto numArgs = tsp->getNumArgs();
        unsigned int i = 0;
        for (const auto arg : tsp->template_arguments())
        {
            if (i > 0 && i < numArgs)
                os << ", ";
            if (arg.getKind() == TemplateArgument::ArgKind::Expression)
                printPrettyD(arg.getAsExpr(), os, nullptr, *DlangBindGenerator::g_printPolicy);
            else if (arg.getKind() == TemplateArgument::ArgKind::Type)
                os << toDStyle(arg.getAsType());
            else
                arg.print(*DlangBindGenerator::g_printPolicy, os);
            i += 1;
        }
        os << ")";
        res = os.str();
    }
    else if (type->isStructureOrClassType() || type->isEnumeralType() || type->isUnionType()) // special case for stripping enum|class|struct keyword
    {
        std::string str;
        if (auto rd = type->getAsRecordDecl())
        {
            str = rd->getName().str();
            #if 0
            // this version will pick nested records skipping namespces,
            // however the function doesn't have optional parameter yet 
            // so we can't choose desired behaviour
            std::vector<std::string> nestedDecl;
            nestedDecl.push_back(rd->getName().str());
            auto ctx = rd->getDeclContext();
            while(!ctx->isTranslationUnit()) 
            {
                if (ctx->isRecord())
                {
                    auto r = cast<RecordDecl>(ctx);
                    nestedDecl.push_back(r->getName().str());
                }
                ctx = ctx->getParent();
            }
            for(auto it = nestedDecl.rbegin(); it != nestedDecl.rend(); it++ )
            {
                if (it != nestedDecl.rbegin())
                    str.append(".");
                str.append(*it);
            }
            #endif
        }
        else 
        {
            // split on whitespace in between "class SomeClass*" and take the part on the right
            str = type.getAsString(*DlangBindGenerator::g_printPolicy);
            auto ws = str.find_first_of(' ', 0);
            if (ws != std::string::npos)
            {
                str = str.substr(ws + 1);
            }
        }
        res = str;
    }
    else if (typeptr && typeptr->isDependentType() && typeptr->getAsRecordDecl())
    {
        if (const auto rec = typeptr->getAsCXXRecordDecl())
        {
            if (const auto dt = rec->getDescribedClassTemplate())
            {
                std::string s;
                llvm::raw_string_ostream os(s);
                os << dt->getName().str();
                os << "!"
                   << "(";
                const auto tplist = dt->getTemplateParameters();
                for (const auto tp : *tplist)
                {
                    os << tp->getName().str();
                    if (tp != *(tplist->end() - 1))
                        os << ", ";
                }
                os << ")";
                res = os.str();
            }
        }
    }
    else if (const auto dn = typeptr->getAs<DependentNameType>())
    {
        std::string s;
        llvm::raw_string_ostream os(s);
        // print nested name
        if (auto q = dn->getQualifier())
        {
            printQualifier(q, os);
            if (q->getKind() != NestedNameSpecifier::SpecifierKind::Namespace)
                os << ".";
        }
        os << dn->getIdentifier()->getName().str();
        res = os.str();
    }
    else
    {
        res = type.getAsString(*DlangBindGenerator::g_printPolicy);
    }

    while (true)
    {
        auto pos = res.rfind("::");
        if (pos == std::string::npos)
            break;
        res.replace(pos, 2, ".");
    }

    return res;
}


std::string DlangBindGenerator::_toDBuiltInType(QualType type)
{
    using TY = BuiltinType::Kind;
    const auto bt = type->getAs<BuiltinType>();
    switch (bt->getKind())
    {
    case TY::Bool:
        return "bool";
    case TY::Char_S:
    case TY::SChar:
        return "char";
    case TY::Char_U:
    case TY::UChar:
        return "ubyte";
    case TY::UShort:
        return "ushort";
    case TY::UInt:
        return "uint";
    case TY::ULong:
        return "cpp_ulong";
    case TY::Long:
        return "cpp_long";
    case TY::ULongLong:
        return "ulong";
    case TY::LongLong:
        return "long";
    case TY::LongDouble:
        return "c_long_double";
    default:
        return type.getAsString(*DlangBindGenerator::g_printPolicy);
    }
    assert(bt != nullptr && "clang::BuiltinType expected");
}


std::string DlangBindGenerator::sanitizedIdentifier(const std::string &id)
{
    if (isReservedIdentifier(id))
        return std::string().append(id).append("_");
    else
        return id;
}


std::string DlangBindGenerator::externAsString(bool isExternC) const
{
    if (cppIsDefault && !isExternC)
    return "C++";
    else
    return "C";
}


// return nested namespaces list for `extern(C++, ...namespace list here...)`
// note that it walks up, so the resulting list is inverted
void DlangBindGenerator::getJoinedNS(const clang::DeclContext *decl, std::vector<std::string> &parts)
{
    if (!decl)
        return;
    if (decl->isTranslationUnit())
        return;
    if (const auto ns = llvm::dyn_cast<NamespaceDecl>(decl))
    {
        parts.push_back(ns->getName().str());
    }

    getJoinedNS(decl->getParent(), parts);
}

void DlangBindGenerator::printBases(const clang::CXXRecordDecl *decl)
{
    unsigned int numBases = decl->getNumBases();
    bool first = true;

    // some quirks with array range ret type...
    for (const auto b : decl->bases())
    {
        numBases--;
        Decl* rd = b.getType()->getAsRecordDecl(); 
        if (rd && !isPossiblyVirtual(cast<RecordDecl>(rd)))
            continue;
        
        if (first) {
            out << " : ";
            first = false;
        }
        out << toDStyle(b.getType());
        if (numBases)
            out << ", ";
    }
}


std::vector<const clang::CXXRecordDecl*> DlangBindGenerator::getNonVirtualBases(const clang::CXXRecordDecl* decl)
{
    std::vector<const clang::CXXRecordDecl*> nonvirt;

    for (const auto b : decl->bases())
    {
        auto rd = b.getType()->getAsRecordDecl(); 
        if (rd) 
            if (auto cxxrec = cast<CXXRecordDecl>(rd); !isPossiblyVirtual(cxxrec))
                nonvirt.push_back(cxxrec);
    }

    return nonvirt;
}


void DlangBindGenerator::handleInnerDecls(const clang::RecordDecl *decl)
{
    for (const auto it : decl->decls())
    {
        if (it == decl)
            continue;
        if (const auto e = llvm::dyn_cast<EnumDecl>(it))
        {
            onEnum(e);
        }
        if (const auto var = llvm::dyn_cast<VarDecl>(it))
        {
            //out << "@cppsize(" << finfo.Width / 8 << ")" << " ";
            out << "__gshared static ";
            out << getAccessStr(it->getAccess(), !decl->isClass()) << " ";
            out << toDStyle(var->getType()) << " ";
            out << sanitizedIdentifier(var->getName().str()) << ";";
            out << std::endl;
        }
        if (const auto d = llvm::dyn_cast<ClassTemplateDecl>(it))
        {
            auto r = d->getTemplatedDecl();
            onStructOrClassEnter(r);
            onStructOrClassLeave(r);
            continue;
        }
        if (const auto d = llvm::dyn_cast<RecordDecl>(it))
        {
            onStructOrClassEnter(d);
            onStructOrClassLeave(d);
        }
        if (const auto m = llvm::dyn_cast<FunctionTemplateDecl>(it))
        {
            if (auto fn = m->getAsFunction())
            {
                // skip "problematic" methods and functions for now
                if (!fn->getIdentifier())
                    continue;
                out << getAccessStr(m->getAccess()) << " ";
                onFunction(fn);
            }
        }
        if (isa<TypedefDecl>(it))
        {
            const auto td = cast<TypedefDecl>(it);
            onTypedef(td);
        }
    }
}


void DlangBindGenerator::handleFields(const clang::RecordDecl *decl)
{
    // TODO: count in types and alignment for bit fields

    isPrevIsBitfield = false;
    accumBitFieldWidth = 0;
    std::string bitwidthExpr;

    for (const auto it : decl->fields())
    {
        unsigned int bitwidth = 0;

        /*
			mixin(bitfields!(
				uint, "x",    2,
				int,  "y",    3,
				uint, "z",    2,
				bool, "flag", 1));
		*/

        RecordDecl *rec = it->getType()->getAsRecordDecl();

        bool isDependent = it->getType()->isDependentType();
        bool isForwardDecl = true;
        if (rec)
            isForwardDecl = rec->getDefinition() == nullptr;

        TypeInfo finfo;
        if (!isDependent)
        if (!isForwardDecl || it->getType()->isBuiltinType() || it->getType()->isPointerType() || it->getType()->isArrayType())
        {
            finfo = it->getASTContext().getTypeInfo(it->getType()); 
        }

        bool bitfield = it->isBitField();
        if (bitfield)
        {
            if (!isPrevIsBitfield)
            {
                out << "mixin(bitfields!(" << std::endl;
            }
            std::string s;
            llvm::raw_string_ostream os(s);
            //it->getBitWidth()->printPretty(os, nullptr, *DlangBindGenerator::g_printPolicy);
            printPrettyD(it->getBitWidth(), os, nullptr, *DlangBindGenerator::g_printPolicy, 0, &decl->getASTContext(), feedback);
            bitwidthExpr = os.str();
            bitwidth = it->getBitWidthValue(decl->getASTContext());
            accumBitFieldWidth += bitwidth;
        }
        else
        {
            // pad it!
            if (isPrevIsBitfield)
            {
                IndentBlock _indent(out, 4);
                // still on same line
                out << "," << std::endl;

                if (accumBitFieldWidth % 8 != 0)
                    if (accumBitFieldWidth <= 8)
                    {
                        out << "uint, \"\", " << 8 - accumBitFieldWidth;
                    }
                    else if (accumBitFieldWidth <= 16)
                    {
                        out << "uint, \"\", " << 16 - accumBitFieldWidth;
                    }
                    else if (accumBitFieldWidth <= 32)
                    {
                        out << "uint, \"\", " << 32 - accumBitFieldWidth;
                    }
                    else if (accumBitFieldWidth <= 64)
                    {
                        out << "uint, \"\", " << 64 - accumBitFieldWidth;
                    }
                    else
                        assert(0 && "Split it up already!");

                accumBitFieldWidth = 0;

                // close bitfield
                out << "));" << std::endl;
            }
        }

        bool shouldStripIndirection = isClassPointer(it->getType());
        auto adjustedType = shouldStripIndirection ? it->getType()->getPointeeType() : it->getType();
        auto fieldTypeStr = toDStyle(adjustForVariable(adjustedType, &decl->getASTContext()));
        if (!it->getIdentifier() && !bitfield)
        {
            constexpr const int ANON_PREFIX_LEN = 5;
            static const auto ANON_PREFIX = "_anon";
            if (fieldTypeStr.compare(ANON_PREFIX) != -1)
            {
                auto n = std::stoi(fieldTypeStr.substr(ANON_PREFIX_LEN));
                const auto& newId = decl->getASTContext().Idents.get(std::string("a") + std::to_string(n) + "_");
                const_cast<FieldDecl*>(it)->setDeclName(DeclarationName(&newId));
            }
        }

        if (bitfield)
        {
            // TODO: close this bitfield and start a new one on zero width
            IndentBlock _indent(out, 4);
            if (isPrevIsBitfield) // we are still on same line
                out << "," << std::endl;
            out << fieldTypeStr << ", ";
            out << "\"";
            if (it->getIdentifier())
                out << sanitizedIdentifier(it->getName().str());
            else 
                out << "";
            out << "\", ";
            out << bitwidthExpr;
        }
        else
        {
            if (finfo.Width)
                out << "@cppsize(" << finfo.Width / 8 << ")"
                    << " ";
            else
                out << "@cppsize(" << 0 << ")"
                    << " ";

            out << getAccessStr(it->getAccess(), !decl->isClass()) << " ";
            out << fieldTypeStr << " ";
            out << sanitizedIdentifier(it->getName().str()) << ";";
            out << std::endl;
        }

        isPrevIsBitfield = bitfield;
    }

    // if we end up with last field being a bit field
    // close bitfield
    if (isPrevIsBitfield)
        out << "));" << std::endl;
}


std::string DlangBindGenerator::getAccessStr(clang::AccessSpecifier ac, bool isStruct /* = false */)
{
    switch (ac)
    {
    case AccessSpecifier::AS_public:
        return "public";
        break;
    case AccessSpecifier::AS_protected:
        return "protected";
        break;
    case AccessSpecifier::AS_private:
        return "private";
        break;
    default:
        return isStruct ? "public" : "private";
    }
}


void DlangBindGenerator::handleMethods(const clang::CXXRecordDecl *decl)
{
    static auto isIdentityAssignmentOperator = [](const CXXMethodDecl* m) { 
        return m->getOverloadedOperator() == OverloadedOperatorKind::OO_Equal
                && m->getNumParams() > 0
                && m->getReturnType()->isReferenceType()
                && m->parameters()[0]->getType()->getAsRecordDecl() == m->getReturnType()->getAsRecordDecl();
    };

    clang::ASTContext *ast = &decl->getASTContext();
    std::unique_ptr<MangleContext> mangleCtx = makeManglingContext(ast);

    bool isVirtualDecl = isPossiblyVirtual(decl);

    for (const auto m : decl->methods())
    {
        // Try to parse delayed templates to get body AST available
        if (m->isLateTemplateParsed())
        {
            parseLateTemplate(m, sema);
        }

        std::string mangledName;
        std::string funcName;
        bool hasRetType = true;
        bool hasConstPtrToNonConst = false;
        bool isStatic = m->isStatic();
        bool moveCtor = false;
        bool copyCtor = false;
        bool isCtor = false;
        bool isDefaultCtor = false;
        bool isDtor = false;
        bool isConversionOp = false;
        bool disable = false;
        bool customMangle = false;

        // TODO: implicit methods starts at decl position itself, skip anything already listed
        clang::SourceLocation location;
#if (LLVM_VERSION_MAJOR < 8)
        location m->getLocStart();
#else
        location = m->getBeginLoc();
#endif
        std::string locString = location.printToString(decl->getASTContext().getSourceManager());
        if (std::find_if(storedTypes.begin(), storedTypes.end(), [&locString](const auto& pair) { return pair.first == locString; } ) != storedTypes.end())
        {
            continue;
        }

        // TODO: add policy to be able to deal with generated ctor/move/copy
        if (m->isDefaulted() && !m->isExplicitlyDefaulted())
            continue;

        funcName = m->getNameAsString();
        bool isOperator = m->isOverloadedOperator();
        if (!mangleCtx->shouldMangleDeclName(m))
        {
            mangledName = m->getNameInfo().getName().getAsString();
        }
        else
        {
            if (const auto ct = llvm::dyn_cast<CXXConstructorDecl>(m))
            {
                funcName = "this";
                hasRetType = false;
                isCtor = true;
                isDefaultCtor = ct->isDefaultConstructor();
                moveCtor = ct->isMoveConstructor();
                copyCtor = ct->isCopyConstructor();
            }
            else if (auto dt = llvm::dyn_cast<CXXDestructorDecl>(m))
            {
                funcName = "~this";
                hasRetType = false;
                isDtor = true;
            }
            if (!(decl->isTemplated() || m->isTemplated()))
            {
                mangledName = getMangledName(m, mangleCtx.get());
            }
        }

        if (const auto conv = llvm::dyn_cast<CXXConversionDecl>(m))
        {
            // try remove ref from template argument for conversions
            auto targetType = conv->getConversionType();
            if (targetType->isReferenceType())
                targetType = targetType->getPointeeType();

            const auto tn = toDStyle(targetType);
            funcName = "opCast(Ty:" + tn + ")";
            isConversionOp = true;
        }

        if (isOperator)
        {
            

            // Get and recombine name with template args (if any)
            auto [name, templatedOp, customMangle_] = getOperatorName(m);
            funcName = name;
            if (!templatedOp.empty()) 
                funcName.append("(" + templatedOp + ")");
            customMangle = customMangle_;
        }

        if (m->getIdentifier() && isReservedIdentifier(m->getName().str()))
            customMangle = true;

        bool possibleOverride = !(isCtor || isDtor) && (overridesBaseOf(m, decl) || (m->size_overridden_methods() > 0));
        bool cantOverride = possibleOverride && !(m->hasAttr<OverrideAttr>() || m->isVirtual());
        bool commentOut = (!isVirtualDecl && isDefaultCtor) || isIdentityAssignmentOperator(m) || (isVirtualDecl && cantOverride);
        // default ctor for struct not allowed
        
        
        if ((mangleAll || customMangle) && !m->isTemplated())
        {
            if (commentOut)
                out << "//";
                
            auto pos = funcName.find('(');
            if (pos == std::string::npos)
                pos = funcName.length();

            // This will be handled as an option to write mangling to external file
            #if 0
            if (mangleOut.is_open())
            {
                mangleOut << "pragma(mangle, \"" << mangledName << "\")" << std::endl;
                mangleOut << "void "
                          << decl->getNameAsString() << "_" << std::string_view(funcName.data(), pos)
                          << "();"
                          << std::endl;
            }
            
            // TODO: clang 7 seems to be ok now, implement this
            if (ast->getLangOpts().isCompatibleWithMSVC(LangOptions::MSVC2015))
            {
                out << "@pyExtract(\"" 
                    <<     decl->getNameAsString() << "::" << m->getNameAsString()
                    << "\")" << "   pragma(mangle, " << "nsgen."
                    <<     decl->getNameAsString() << "_" << std::string_view(funcName.data(), pos) << ".mangleof)"
                    << std::endl;
            }
            else 
            #endif
            {
                out << "pragma(mangle, \"" << mangledName << "\")" << std::endl;
            }
        }

        if (commentOut)
        {
            out << "// ";
        }

        if (m->isDefaulted())
            out << "// (default) ";

        if (moveCtor)
            out << "// move ctor" << std::endl;
        if (copyCtor)
            out << "// copy ctor" << std::endl;

        if (m->isInlined() && m->hasInlineBody())
        {
            out << "/* inline */ ";
        }

        out << getAccessStr(m->getAccess(), !decl->isClass()) << " ";


        if (m->isStatic())
            out << "static ";

        if (isVirtualDecl && m->isPure())
            out << "abstract ";

        if (isVirtualDecl && possibleOverride)
            out << "override ";

        if (isVirtualDecl && !m->isVirtual())
            out << "final ";

        if (moveCtor && m->getAccess() == AccessSpecifier::AS_private)
        {
            out << "@disable ";
        }

        if (hasRetType) 
        {
            bool shouldStripIndirection = isClassPointer(m->getReturnType());
            auto adjustedType = shouldStripIndirection ? m->getReturnType()->getPointeeType() : m->getReturnType();
            out << toDStyle(adjustedType) << " ";
        }

        if (isOperator || m->getIdentifier() == nullptr)
            out << funcName;
        else
            out << sanitizedIdentifier(m->getName().str());
        
        // runtime args
        out << "(";
        writeFnRuntimeArgs(getFnBody(m));
        out << ")";

        if (m->isConst())
            out << " const ";

        if (nogc) 
            out << " @nogc ";

        if (hasConstPtrToNonConst)
            out << "/* MANGLING OVERRIDE NOT YET FINISHED, ADJUST MANUALLY! */ ";
        
        if (!skipBodies && m->hasBody())
            writeFnBody(m, commentOut);
        else
            out << ";" << std::endl;
    
        out << std::endl;

        if (isDefaultCtor && !isVirtualDecl)
            addStructDefaultCtorReplacement(dyn_cast<CXXConstructorDecl>(m), mangledName);
    }
}


void DlangBindGenerator::addStructDefaultCtorReplacement(const clang::CXXConstructorDecl* ctor, const std::string_view knownMangling)
{
    static const auto suffix = "_default_ctor";

    // write mangling part
    if (!ctor->getBody())
    {
        if (!knownMangling.empty())
            out << "pragma (mangle, \"" << knownMangling << "\")" << std::endl;
        out << "extern(C++) ";
    }

    out << getAccessStr(ctor->getAccess(), true /*isStruct*/) << " ";
    out << "final void ";
    out << suffix;
    out << "(";
    writeFnRuntimeArgs(getFnBody(ctor));
    out << ")";
    if (ctor->getBody()) 
    {
        out << " ";
        writeFnBody(const_cast<CXXConstructorDecl*>(ctor));
    }
    else
        out << ";" << std::endl;
    out << std::endl;
}


void DlangBindGenerator::writeFnRuntimeArgs(const clang::FunctionDecl* fn)
{
    bool inlined = fn->getBody() && fn->isInlined();
    bool noRefs = inlined && stripRefParam;
    for (const auto fp : fn->parameters())
    {
        bool shouldStripIndirection = isClassPointer(fp->getType());
        auto adjustedType = shouldStripIndirection ? fp->getType()->getPointeeType() : fp->getType();
        const auto typeStr = toDStyle(adjustedType);
        if (noRefs)
            out << stripRefFromParam(typeStr);
        else
            out << typeStr;
        out << " " << sanitizedIdentifier(fp->getName().str());

        bool isdefaultAvail = !(fp->hasUninstantiatedDefaultArg() || fp->hasUnparsedDefaultArg());
        if (isdefaultAvail)
        if (const auto defaultVal = fp->getDefaultArg()) 
        {
            bool isNull = false;
            if (fp->getType()->isPointerType())
            {
                auto nullkind = defaultVal->isNullPointerConstant(fn->getASTContext(), Expr::NullPointerConstantValueDependence::NPC_NeverValueDependent);
                if (nullkind != Expr::NullPointerConstantKind::NPCK_NotNull)
                    isNull = true;
            }

            std::string s;
            llvm::raw_string_ostream os(s);
            if (isNull)
                os << "null";
            else 
                printPrettyD(defaultVal, os, nullptr, *DlangBindGenerator::g_printPolicy, 0, &fn->getASTContext(), feedback);
            out << " = " << os.str();

            // add rvalue-ref hack
            if (fp->getType()->isReferenceType()) 
            {
                
                if (auto rec = defaultVal->getType()->getAsRecordDecl(); rec && rec->getIdentifier())
                    feedback->addAction(
                        std::move(std::make_unique<AddRvalueHackAction>(std::string(rec->getName())))
                    );
                // TODO: skip on function calls
                out << ".byRef ";
            }
        }

        if (fp != *(fn->param_end() - 1))
            out << ", ";
    }

    if (fn->isVariadic())
    {
        if (fn->getNumParams())
            out << ", ";
        out << "...";
    }

}

void DlangBindGenerator::writeTemplateArgs(const clang::TemplateDecl* td)
{
    const auto tplist = td->getTemplateParameters();
    for (const auto tp : *tplist)
    {
        if (isa<NonTypeTemplateParmDecl>(tp))
        {
            auto nt = cast<NonTypeTemplateParmDecl>(tp);
            out << toDStyle(nt->getType()) << " ";
            out << tp->getNameAsString();
            if (auto defaultVal = nt->getDefaultArgument())
            {
                std::string s;
                llvm::raw_string_ostream os(s);
                printPrettyD(defaultVal, os, nullptr, *DlangBindGenerator::g_printPolicy, 0, &td->getASTContext(), feedback);
                out << " = " << os.str();
            }
        }
        else
            out << tp->getName().str();
        if (tp != *(tplist->end() - 1))
            out << ", ";
    }
}

void DlangBindGenerator::writeTemplateArgs(const clang::TemplateArgumentList* ta)
{
    bool first = true;
    for (const auto tp : ta->asArray())
    {
        auto tk = tp.getKind();
        std::string s;
        llvm::raw_string_ostream os(s);
        switch(tk)
        {
            case TemplateArgument::ArgKind::Integral:
                out << intTypeForSize(tp.getAsIntegral().getBitWidth()) << " T: ";
                out << tp.getAsIntegral().toString(10);
                break;
            case TemplateArgument::ArgKind::Expression:
                printPrettyD(tp.getAsExpr(), os, nullptr, *DlangBindGenerator::g_printPolicy);
                out << s;
                break;
            case TemplateArgument::ArgKind::Type:
                out << toDStyle(tp.getAsType());
                break;
            default: break;
        }
        if (first)
            first = false;
        else
            out << ", ";
    }
}

void DlangBindGenerator::writeTemplateArgs(const clang::TemplateParameterList* tp)
{
    bool first = true;
    for (const auto T : tp->asArray())
    {
        // TemplateTypeParmDecl | NonTypeTemplateParmDecl | ?
        if (isa<NonTypeTemplateParmDecl>(T))
        {
            auto nt = cast<NonTypeTemplateParmDecl>(T);
            out << toDStyle(nt->getType()) << " ";
            if (auto defaultVal = nt->getDefaultArgument())
            {
                std::string s;
                llvm::raw_string_ostream os(s);
                printPrettyD(defaultVal, os, nullptr, *DlangBindGenerator::g_printPolicy);
                out << " = " << os.str();
            }
        }
        out << T->getName().str();
        if (first)
            first = false;
        else
            out << ", ";
    }
}


void DlangBindGenerator::writeFnBody(clang::FunctionDecl* fn, bool commentOut)
{
    auto ast = &fn->getASTContext();
    RecordDecl* decl = nullptr;
    if (fn->getDeclContext()->isRecord())
        decl = cast<RecordDecl>(fn->getDeclContext());

    auto asCXXDecl = [&decl] () { return cast<CXXRecordDecl>(decl); };

    auto writeMultilineExpr = [this, commentOut, ast](auto expr, bool ptrRet = false) {
        std::string s;
        llvm::raw_string_ostream os(s);
        std::stringstream ss;
        DPrinterHelper_PointerReturn rp;
        printPrettyD(expr, os, ptrRet ? &rp : nullptr, *DlangBindGenerator::g_printPolicy, 0, ast, feedback);
        ss << os.str();
        std::string line;
        while (std::getline(ss, line))
        {
            if (commentOut) 
                out << "//";
            out << textReplaceArrowColon(line);
            if (!ss.eof()) 
                out << std::endl;
        }
    };

    // D requires that structs have known compile time initializer
    // so it is basically prohibited to have zero argument default ctor
    // though it can be hacked around to have default arguments ctor to look like one
    static auto fakeDefaultCtor = [] (const clang::FunctionDecl* fn) -> bool {
        if (fn->param_size() == 0)
            return false;
        return std::all_of(fn->param_begin(), fn->param_end(), [](ParmVarDecl* p) {return p->hasDefaultArg(); });
    };

    const FunctionDecl* fndef = nullptr;
    if (!fn->getBody())
        fn->getBody(fndef);
    if (!fndef)
        fndef = fn;

    if (fndef->isLateTemplateParsed())
        parseLateTemplate(const_cast<FunctionDecl*>(fndef), sema);

    // for now just mix initializer list and ctor body in extra set of braces
    bool hasInitializerList = false;
    bool isEmptyBody = true;
    bool isTemplated = fn->isTemplated();
    if (decl)
        isTemplated = isTemplated || decl->isTemplated();
    if (fndef->getBody())
        if (auto body = dyn_cast<CompoundStmt>(fndef->getBody())) // can it actually be anything else?
            isEmptyBody = body->body_empty();

    if (isa<CXXConstructorDecl>(fn))
    {
        const auto ctdecl = cast<CXXConstructorDecl>(fn);
        hasInitializerList = ctdecl->getNumCtorInitializers() != 0
            && std::find_if( ctdecl->init_begin(), ctdecl->init_end(), 
                //[](auto x) {return x->isInClassMemberInitializer();}
                [](auto x) {return x->isWritten();}
                ) != ctdecl->init_end();

        if (hasInitializerList)
            out << "{" << std::endl << "// initializer list" << std::endl;

        for(const auto init : ctdecl->inits())
        {
            if (!init->isWritten())
                continue;
            // This will be handled at some point later by memberIterate() 
            if (init->isInClassMemberInitializer())
                continue;
            if (auto member = init->getMember())
            {
                writeMultilineExpr(init);
            }
            if (init->isBaseInitializer() && hasInitializerList)
            {
                bool tempComm = commentOut;
                auto scope = llvm::make_scope_exit ( [&]{commentOut = tempComm;} );
                // check if aliased base class have constructor available
                if (auto basector = dyn_cast<CXXConstructExpr>(init->getInit()))
                {
                    if (!isPossiblyVirtual(decl))
                    {
                        auto baserec = basector->getType()->getAsCXXRecordDecl();
                        if ( auto cxxdecl = asCXXDecl(); cxxdecl && cxxdecl->getNumBases() 
                            && cxxdecl->bases_begin()->getType()->getAsCXXRecordDecl() == baserec)
                        {
                            if (basector->getNumArgs() > 0)
                            {
                                if (commentOut)
                                    out << "//";
                                out << "_b0.__ctor(";
                                writeMultilineExpr(init);
                                out << ");" << std::endl;
                                continue;
                            }
                            else
                                 commentOut = true;
                        }
                    }
                }
                

                if (commentOut)
                    out << "//";
                if (isPossiblyVirtual(decl))
                    out << "super";
                else
                {
                    auto iexpr = init->getInit()->IgnoreParens();
                    bool isDefaultCtorCall = false;
                    if (iexpr && isa<CXXConstructExpr>(iexpr))
                    {
                        if (auto ctinit = dyn_cast<CXXConstructExpr>(iexpr))
                        {
                            isDefaultCtorCall = ctinit->getConstructor()->isDefaultConstructor() 
                                && !fakeDefaultCtor(ctdecl);
                        }
                    }
                    auto initBaseTypeClass = init->getBaseClass()->getTypeClass();
                    if (initBaseTypeClass == Type::Typedef)
                    {
                        // FIXME: fix multiple inheritance, though it's not supported, not yet
                        // for this case (when we have non-virtual inheritance) find mixed-in index and call ctor on it
                        //auto ty = dyn_cast<TypedefType>(init->getBaseClass());
                        //auto cxxdecl = asCXXDecl();
                        //auto found = std::find_if(cxxdecl->bases_begin(), cxxdecl->bases_end(), 
                        //    [ty](CXXBaseSpecifier b) { return b.getType()->getAsRecordDecl() == ty->getAsRecordDecl(); }
                        //);
                        //auto baseidx = std::distance(cxxdecl->bases_begin(), found);
                        //out << "_b" << baseidx;
                        
                        if (init->isBaseInitializer() && isDefaultCtorCall)
                            out << "_b0._default_ctor";
                        else
                            out << "_b0.__ctor";
                    }
                    else if (initBaseTypeClass == Type::TemplateTypeParm)
                    {
                        auto* t = dyn_cast<TemplateTypeParmType>(init->getBaseClass());
                        if (t && t->getIdentifier())
                            out << t->getIdentifier()->getName().str();
                        else
                            out << "/*not yet implemented*/";
                    }
                    else if (initBaseTypeClass == Type::TemplateSpecialization)
                    {
                        auto* t = dyn_cast<TemplateSpecializationType>(init->getBaseClass());
                        if (t) 
                        {
                            if (auto cxxdecl = asCXXDecl())
                            {
                                //out << toDStyle(init->getTypeSourceInfo()->getType());

                                auto found = std::find_if(cxxdecl->bases_begin(), cxxdecl->bases_end(), 
                                    [t](CXXBaseSpecifier b) { return b.getType().getTypePtr()->getAsRecordDecl() == t->getAsRecordDecl(); }
                                );
                                if (found != cxxdecl->bases_end())
                                {
                                    auto baseidx = std::distance(cxxdecl->bases_begin(), found);
                                    out << "_b" << baseidx;
                                }
                                else
                                {
                                    // TODO: base type unable to compare with template args
                                    //   do something else to get correct base
                                    out << "_b0";
                                }
                            }
                            else // unknown, just assume there is only one base
                                out << "_b0";
                            
                            if (init->isBaseInitializer())
                            {
                                if (isDefaultCtorCall)
                                    out << "._default_ctor";
                                else
                                    out << ".__ctor";
                            }
                        }
                    }
                    else
                        out << init->getBaseClass()->getAsCXXRecordDecl()->getName().str();
                }
                writeMultilineExpr(init);
                out << ";" << std::endl;
            }
        }

        if (hasInitializerList && !isEmptyBody)
            out << "// ctor body" << std::endl;
    }

    if (!isEmptyBody)
    {
        // write body after initializer list
        writeMultilineExpr(fndef->getBody(), fn->getReturnType()->isPointerType());
        out << std::endl;
    }

    // close extra braces
    if (hasInitializerList && isa<CXXConstructorDecl>(fn))
    {
        if (commentOut)
            out << "//";
        out << "}";
    }
    else if (isEmptyBody)
        out << " {} " << std::endl;

    if (!(fndef->getBody() || hasInitializerList))
    {
        out << ";";
    }
}


std::tuple<std::string, std::string> DlangBindGenerator::getFSPathPart(const std::string_view loc)
{
    std::string path;
    std::string lineCol;
    size_t colPos = std::string_view::npos;

    auto dotPos = loc.find_last_of('.');
    if (dotPos != std::string_view::npos)
    {
        colPos = loc.find_first_of(':', dotPos);
        if (colPos != std::string_view::npos)
        {
            lineCol = loc.substr(colPos);
            path = loc.substr(0, colPos);
        }
    }

    if (loc.length() && path.empty())
        path = std::string(loc);

    std::error_code _;
    path = fs::canonical(path, _).string();
    return std::make_tuple(path, lineCol);
}

std::tuple<std::string, std::string> DlangBindGenerator::getLineColumnPart(const std::string_view loc)
{
    std::string line;
    std::string col;

    auto colPos = loc.find_first_of(':');
    if (colPos != std::string_view::npos)
    {
        line = loc.substr(0, colPos-1);
        col = loc.substr(colPos+1);
    }
    else
        line = loc;

    return std::make_tuple(line, col);
}

std::string DlangBindGenerator::getNextMixinId()
{
    std::ostringstream ss;
    ss << "mxtid" << std::setw(3) << std::setfill('0') << mixinTemplateId;
    mixinTemplateId += 1;
    return ss.str();
}

std::tuple<std::string, std::string, bool> DlangBindGenerator::getOperatorName(const clang::FunctionDecl* decl)
{
    const auto op = decl->getOverloadedOperator();
    const auto psize = decl->param_size();
    return getOperatorName(op, psize);
}


std::tuple<std::string, std::string, bool> DlangBindGenerator::getOperatorName(clang::OverloadedOperatorKind kind, int arity)
{
    const bool isBinary = arity == 1;
    const bool isUnary = arity == 0;

    auto arityStr = [isBinary]() {return isBinary? "opBinary" : "opUnary";};
    auto getOpArgs = [](const std::string& s) { return std::string("string op : \"") + s + "\""; };

    bool customMangle = false;
    std::string funcName;
    std::string opSign;
    switch (kind)
    {
    case OverloadedOperatorKind::OO_Plus:
        funcName = arityStr(); opSign = getOpArgs("+");
        break;
    case OverloadedOperatorKind::OO_Minus:
        funcName = arityStr(); opSign = getOpArgs("-");
        break;
    case OverloadedOperatorKind::OO_Star:
        funcName = arityStr(); opSign = getOpArgs("*");
        break;
    case OverloadedOperatorKind::OO_Slash:
        funcName = arityStr(); opSign = getOpArgs("/");
        break;
    case OverloadedOperatorKind::OO_Percent:
        funcName = arityStr(); opSign = getOpArgs("%");
        break;
    case OverloadedOperatorKind::OO_Caret:
        funcName = arityStr(); opSign = getOpArgs("^");
        break;
    case OverloadedOperatorKind::OO_Amp:
        funcName = arityStr(); opSign = getOpArgs("&");
        break;
    case OverloadedOperatorKind::OO_Pipe:
        funcName = arityStr(); opSign = getOpArgs("|");
        break;
    case OverloadedOperatorKind::OO_Tilde:
        funcName = arityStr(); opSign = getOpArgs("~");
        break;
    case OverloadedOperatorKind::OO_MinusMinus:
        funcName = arityStr(); opSign = getOpArgs("--");
        break;
    case OverloadedOperatorKind::OO_PlusPlus:
        funcName = arityStr(); opSign = getOpArgs("++");
        break;
    case OverloadedOperatorKind::OO_Call:
        funcName = "opCall";
        break;
    case OverloadedOperatorKind::OO_Subscript:
        funcName = "opIndex";
        break;
    case OverloadedOperatorKind::OO_AmpAmp:
        funcName = "op_and"; customMangle = true;
        break;
    case OverloadedOperatorKind::OO_PipePipe:
        funcName = "op_or"; customMangle = true;
        break;
    case OverloadedOperatorKind::OO_Less:
        funcName = "op_lt"; customMangle = true;
        break;
    case OverloadedOperatorKind::OO_Greater:
        funcName = "op_gt"; customMangle = true;
        break;
    case OverloadedOperatorKind::OO_LessEqual:
        funcName = "op_le"; customMangle = true;
        break;
    case OverloadedOperatorKind::OO_GreaterEqual:
        funcName = "op_ge"; customMangle = true;
        break;
    case OverloadedOperatorKind::OO_Exclaim:
        funcName = "op_not"; customMangle = true;
        break;
    case OverloadedOperatorKind::OO_ExclaimEqual:
        funcName = "op_ne"; customMangle = true;
        break;
    case OverloadedOperatorKind::OO_PlusEqual:
        funcName = "opOpAssign"; opSign = getOpArgs("+");
        break;
    case OverloadedOperatorKind::OO_MinusEqual:
        funcName = "opOpAssign"; opSign = getOpArgs("-");
        break;
    case OverloadedOperatorKind::OO_StarEqual:
        funcName = "opOpAssign"; opSign = getOpArgs("*");
        break;
    case OverloadedOperatorKind::OO_SlashEqual:
        funcName = "opOpAssign"; opSign = getOpArgs("/");
        break;
    case OverloadedOperatorKind::OO_PipeEqual:
        funcName = "opOpAssign"; opSign = getOpArgs("|");
        break;
    case OverloadedOperatorKind::OO_AmpEqual:
        funcName = "opOpAssign"; opSign = getOpArgs("&");
        break;
    case OverloadedOperatorKind::OO_CaretEqual:
        funcName = "opOpAssign"; opSign = getOpArgs("^");
        break;
    case OverloadedOperatorKind::OO_LessLessEqual:
        funcName = "opOpAssign"; opSign = getOpArgs("<<");
        break;
    case OverloadedOperatorKind::OO_GreaterGreaterEqual:
        funcName = "opOpAssign"; opSign = getOpArgs(">>");
        break;
    case OverloadedOperatorKind::OO_Equal:
        funcName = "opAssign";
        break;
    case OverloadedOperatorKind::OO_EqualEqual:
        funcName = "opEquals";
        break;
    case OverloadedOperatorKind::OO_Arrow:
        funcName = "opUnary"; opSign = getOpArgs("->");
        break;
    case OverloadedOperatorKind::OO_New:
        funcName = "op_new"; customMangle = true;
        break;
    case OverloadedOperatorKind::OO_Array_New:
        funcName = "op_newArr"; customMangle = true;
        break;
    case OverloadedOperatorKind::OO_Delete:
        funcName = "op_delete"; customMangle = true;
        break;
    case OverloadedOperatorKind::OO_Array_Delete:
        funcName = "op_deleteArr"; customMangle = true;
        break;
    default:
        funcName = "op"; opSign = getOperatorSpelling(kind);
        break;
    }

    return std::make_tuple(funcName, opSign, customMangle);
}

std::string DlangBindGenerator::getOperatorString(clang::OverloadedOperatorKind kind, int arity)
{
    auto [name, sign, _] = getOperatorName(kind, arity);
    std::string str = name;
    llvm::raw_string_ostream buf(str);
    if (sign.length())
    {
        // opBinary!"+"
        buf << "!" << "\"" << sign << "\"";
    }
    return buf.str();
}

IncludeMap::IncludeMap()
{
    add("complex.h", "core.stdc.complex");

    add("ctype.h", "core.stdc.ctype");
    add("cctype", "core.stdc.ctype");

    add("errno.h", "core.stdc.errno");
    add("cerrno", "core.stdc.errno");

    add("fenv.h", "core.stdc.fenv");

    add("float.h", "core.stdc.float_");
    add("cfloat", "core.stdc.float_");

    add("inttypes.h", "core.stdc.inttypes");

    add("limits.h", "core.stdc.limits");
    add("climits", "core.stdc.limits");

    add("locale.h", "core.stdc.locale");
    add("clocale", "core.stdc.locale");

    add("math.h", "core.stdc.math");
    add("cmath", "core.stdc.math");

    add("signal.h", "core.stdc.signal");
    add("csignal", "core.stdc.signal");

    add("stdarg.h", "core.stdc.stdarg");
    add("cstdarg", "core.stdc.stdarg");

    add("stddef.h", "core.stdc.stddef");
    add("cstddef", "core.stdc.stddef");

    add("stdint.h", "core.stdc.stdint");
    add("cstdint", "core.stdc.stdint");

    add("stdio.h", "core.stdc.stdio");
    add("cstdio", "core.stdc.stdio");

    add("stdlib.h", "core.stdc.stdlib");
    add("cstdlib", "core.stdc.stdlib");

    add("string.h", "core.stdc.string");
    add("cstring", "core.stdc.string");

    add("tgmath.h", "core.stdc.tgmath");
    add("ctgmath", "core.stdc.tgmath");

    add("time.h", "core.stdc.time");
    add("ctime", "core.stdc.time");

    add("wchar.h", "core.stdc.wchar_");
    add("cwchar", "core.stdc.wchar_");

    add("wctype.h", "core.stdc.wctype");
    add("cwctype", "core.stdc.wctype");
}

std::string IncludeMap::find(const std::string& header)
{
    if (isGlobalMatchAny(header))
        return std::string();

    std::string path = header;
    auto pos = path.find("\\");
    while(pos != std::string::npos)
    {
        path.replace(pos, 1, "/");
        pos = path.find("\\");
    }

    auto m = _mappings.find(path);
    return m == _mappings.end() ? std::string() : m->second;
}

void IncludeMap::add(const std::string& header, const std::string& mappedModule)
{
    if (isGlobalMatchAny(header))
        return;

    _mappings.insert(std::make_pair(header, mappedModule));
}
