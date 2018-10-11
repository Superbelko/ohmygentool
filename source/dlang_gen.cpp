
#include "dlang_gen.h"

#include <sstream>

#include "llvm/ADT/ArrayRef.h"
#include "clang/AST/AST.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
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

#include "clangparser.h"
#include "iohelpers.h"

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


// D reserved keywords, possible overlaps from C/C++ sources
std::vector<std::string> reservedIdentifiers = {
    "out", "ref", "version", "debug", "mixin", "with", "unittest", "typeof", "typeid", "super", "body",
    "shared", "pure", "package", "module", "inout", "in", "import", "invariant", "immutable", "interface",
    "function", "delegate", "final", "export", "deprecated", "alias", "abstract", "synchronized",
    "byte", "ubyte", "uint", "ushort"
};

const char* MODULE_HEADER =
R"(
import core.stdc.config;
import std.bitmanip : bitfields;

bool isModuleAvailable(alias T)() {
    mixin("import " ~ T ~ ";");
    static if (__traits(compiles, mixin(T).stringof))
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


// De-anonimize provided decl and all sub decls, will set generated identifier to the types
void deanonimizeTypedef(clang::RecordDecl* decl, const std::string_view optName = std::string_view())
{
    // TODO: This function is just ugly, refactor!
    int counter = 1; // will be appended to nested anonymous entries
    if (!decl->getIdentifier())
    {
        if (optName.empty())
        {
            auto newName = std::string("_anon").append(std::to_string(counter));
            counter += 1;
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
        if (auto td = llvm::dyn_cast<TypedefDecl>(d))
        {
            if (!td->getIdentifier()) // no name, add some
            {
                auto newName = std::string("_anon").append(std::to_string(counter));
                counter += 1;
                const auto& newId = td->getASTContext().Idents.get(newName);
                td->setDeclName(DeclarationName(&newId));
            }
            if (auto tdtype = td->getUnderlyingType().getTypePtr())
            {
                if (auto rd = tdtype->getAsRecordDecl() )
                {
                    deanonimizeTypedef(rd);
                }
            }
        }
        else if (auto rec = llvm::dyn_cast<RecordDecl>(d))
        {
            if (!rec->getIdentifier())
            {
                auto newName = std::string("_anon").append(std::to_string(counter));
                counter += 1;
                const auto& newId = rec->getASTContext().Idents.get(newName);
                rec->setDeclName(DeclarationName(&newId));
            }
        }
    }
}


// Replaces C++ token like arrow operator or scope double-colon
void textReplaceArrowColon(std::string& in)
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
}


bool DlangBindGenerator::isRelevantPath(const std::string_view path)
{
    if (path.compare("<invalid loc>") == 0) 
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
        cppIsDefault = inOpt->standard.compare("c++") != std::string::npos;
    }
    if (outOpt)
    {
        if (fileOut.is_open())
            fileOut.close();
        fileOut.open(outOpt->path);
        if (std::find(outOpt->extras.begin(), outOpt->extras.end(), "attr-nogc") != outOpt->extras.end())
            nogc = true;
    }
}


void DlangBindGenerator::prepare()
{
    mixinTemplateId = 1;
    out << MODULE_HEADER;

    out << std::endl;
}


void DlangBindGenerator::finalize()
{
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

    if (!addType(decl, storedTypes))
        return;

    std::vector<std::string> nestedNS;
    getJoinedNS(decl, nestedNS);
    const bool hasNamespace = nestedNS.size() > 0;
    std::string mixinId;

    // Note: until proper solution implemnted in D (probably namespace as string)
    //  write hacky solution of wrapping namespace
    //  inside mixin template with code mixin
    // 
    // mixin template uniq_id_here {
    //  mixin(`extern(C++, namespace.list) void actual_function();`)
    // } mixin uniq_id_here;

    // linkage & namespace
    if (!hasNamespace)
        out << "extern(" << externAsString(decl->getDeclContext()->isExternCContext()) << ")" << std::endl;
    else
    {
        mixinId = getNextMixinId();
        out << "mixin template " << mixinId << "() {" << std::endl;
        out.imore(4); // adds indent to all stuff below
        out << "mixin(`";

        out << "extern(" << externAsString(decl->getDeclContext()->isExternCContext()) << ", ";
        auto it = std::rbegin(nestedNS);
        while (it != nestedNS.rend())
        {
            out << *it; if (it != (nestedNS.rend() - 1)) out << ".";
            it++;
        }
        out << ")" << std::endl;
    }

    

    TypeInfo ti;
    if (!decl->isTemplated()) // skip templated stuff for now
    {
        const auto tdd = decl->getTypeForDecl();
        if (!tdd)
            return;
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


    if (decl->isClass() || isDerived)
        out << "class ";
    else if (decl->isStruct())
        out << "struct ";
    else if (decl->isUnion())
        out << "union ";

    if (classOrStructName.empty())
    {
        globalAnonTypeId += 1;
        deanonimizeTypedef(
            const_cast<RecordDecl*>(decl), 
            std::string("AnonType_").append(std::to_string(globalAnonTypeId))
        );
        classOrStructName = decl->getName().str();
    }
    else
    {
        // FIXME: OH NO!
        deanonimizeTypedef(const_cast<RecordDecl*>(decl));
    }


    if (classOrStructName == "rcScopedDelete")
        int x = 0;

    out << classOrStructName;
    if (cxxdecl)
    {
        auto classTemplate = cxxdecl->getDescribedClassTemplate();
		if (classTemplate) 
			classTemplate = classTemplate->getCanonicalDecl(); //retrieve real decl
		if (classTemplate)
        {
            if ( classTemplate->isThisDeclarationADefinition() ) // add only actual declarations
            {
                const auto tparams = classTemplate->getTemplateParameters();
                if (tparams)
                {
                    out << "(";
                    for(const auto tp : *tparams)
                    {
                        out << tp->getName().str();
                        if (tp != *(tparams->end()-1))
                            out << ", ";
                    }
                    out << ")";
                }
            }
        }
        // adds list of base classes
        printBases(cxxdecl);
    }
    out << std::endl;
    out << "{" << std::endl;
    {
        IndentBlock _classbody(out, 4);
#if 0
        // Nope, this does not conform to actual layout, 
        // that means we have to apply align on every field instead
        if (!(decl->isUnion() || decl->isTemplated()))
            out << "align(" << ti.Align / 8 << "):" << std::endl;
#endif
        innerDeclIterate(decl);
        fieldIterate(decl);
        if (cxxdecl)
        methodIterate(cxxdecl);
    }
    out << "}" << std::endl;
    
    if (hasNamespace)
    {
        out << "`);" << std::endl;
        out.iless(4); // indent out from mixin
        out << "} "  << "mixin " << mixinId << ";" << std::endl;
    }
}


void DlangBindGenerator::onStructOrClassLeave(const clang::RecordDecl *decl)
{
    declStack.pop_back();
    out << std::endl;
}


void DlangBindGenerator::onEnum(const clang::EnumDecl *decl)
{
    if (!addType(decl, enumDecls))
        return;

    const auto size = std::distance(decl->enumerator_begin(), decl->enumerator_end());
    const auto enumTypeString = toDStyle(decl->getIntegerType());

    bool hasName = !decl->getName().empty();
    if (hasName)
    {
        // alias myEnum = int;
        out << "alias " << decl->getName().str() << " = " << enumTypeString << ";" << std::endl;
        // enum : myEnum
        out << "enum "
            << " : " << decl->getName().str() << std::endl;
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
}


void DlangBindGenerator::onFunction(const clang::FunctionDecl *decl)
{
    if (!addType(decl, functionDecls))
        return;

    const auto fn = decl;
    std::vector<std::string> nestedNS;
    getJoinedNS(decl, nestedNS);
    const bool hasNamespace = nestedNS.size() > 0;
    std::string mixinId;

    // Note: until proper solution implemnted in D (probably namespace as string)
    //  write hacky solution of wrapping namespace
    //  inside mixin template with code mixin
    // 
    // mixin template uniq_id_here {
    //  mixin(`extern(C++, namespace.list) void actual_function();`)
    // } mixin uniq_id_here;

    // linkage & namespace
    if (!hasNamespace)
        out << "extern(" << externAsString(fn->isExternC()) << ")" << std::endl;
    else
    {
        mixinId = getNextMixinId();
        out << "mixin template " << mixinId << "() {" << std::endl;
        out.imore(4); // adds indent to all stuff below
        out << "mixin(`";

        out << "extern(" << externAsString(fn->isExternC()) << ", ";
        auto it = std::rbegin(nestedNS);
        while (it != nestedNS.rend())
        {
            out << *it; if (it != (nestedNS.rend() - 1)) out << ".";
            it++;
        }
        out << ")" << std::endl;
    }

    // ret type
    {
        const auto typeStr = toDStyle(fn->getReturnType());
        out << typeStr << " ";
    }
    // function name
    out << fn->getName().str() << " (";

    if (fn->isTemplated())
    {
        // note that we write to already opened parenthesis
        const auto ftd = fn->getDescribedTemplate();
        const auto tplist = ftd->getTemplateParameters();
        for (const auto tp : *tplist)
        {
            out << tp->getName().str();
            if (tp != *(tplist->end() - 1))
                out << ", ";
        }
        // close template params list and open runtime args
        out << ")(";
    }

    // argument list
    for (auto p = fn->param_begin(); p != fn->param_end(); p++)
    {
        if (p != fn->param_begin() && p != fn->param_end())
            out << ", ";

        const auto typeStr = toDStyle((*p)->getType());
        out << typeStr << " " << sanitizedIdentifier((*p)->getName().str());

        if (const auto defaultVal = (*p)->getDefaultArg())
        {
            std::string s;
            llvm::raw_string_ostream os(s);
            defaultVal->printPretty(os, nullptr, *ClangParser::g_printPolicy);
            out << " = " << os.str();
        }
    }
    out << ")";
    if (nogc)
        out << " @nogc";
    out << ";" << std::endl;

    if (hasNamespace)
    {
        out << "`);" << std::endl;
        out.iless(4); // indent out from mixin
        out << "} "  << "mixin " << mixinId << ";" << std::endl;
    }

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
        auto rd = tdtype->getAsRecordDecl();
        if (rd && !rd->getIdentifier())
        {
            deanonimizeTypedef(rd, typedefName);
            onStructOrClassEnter(rd);
            onStructOrClassLeave(rd);
            // We're done here, but it also might be an option to append 
            // underscore and add original typedef'ed name as alias
            return; 
        }
    }

    out << "alias " << typedefName << " = ";
    if (functionType)
    {
        out << "extern(" << externAsString(extC) << ") ";
    }
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
        init->printPretty(os, nullptr, *ClangParser::g_printPolicy);
        out << " = ";
        if (decl->getType()->isFloatingType())
            out << _adjustVarInit(os.str());
        else
            out << os.str();
    }
    out << ";";
    out << std::endl;
}


// adjust initializer for D, for example 50.F -> 50.0f
std::string DlangBindGenerator::_adjustVarInit(const std::string &e)
{
    std::string res = e;
    if (e.rfind(".F") != std::string::npos)
    {
        res.replace(e.length() - 2, 3, ".0f");
    }
    else if (res.at(res.length() - 1) == 'F')
    {
        res.replace(res.length() - 1, 1, "f");
    }

    return res;
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
    const auto typeptr = type.getTypePtr();

    if (type->isPointerType() || type->isReferenceType())
    {
        if (type->getPointeeType()->isFunctionType())
            return toDStyle(type->getPointeeType());
        res = _wrapParens(type);
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
    else if (type->isBuiltinType())
    {
        res = _toDBuiltInType(type);
    }
    else if (const auto tsp = type->getAs<TemplateSpecializationType>()) // matches MyArray<int> field
    {
        std::string s;
        llvm::raw_string_ostream os(s);
        tsp->getTemplateName().print(os, *ClangParser::g_printPolicy);
        os << "!"
           << "(";
        //s.clear();
        const auto numArgs = tsp->getNumArgs();
        unsigned int i = 0;
        for (const auto arg : tsp->template_arguments())
        {
            if (i > 0 && i < numArgs)
                os << ", ";
            arg.print(*ClangParser::g_printPolicy, os);
            i += 1;
        }
        os << ")";
        res = os.str();
    }
    else if (type->isStructureOrClassType() || type->isEnumeralType()) // special case for stripping enum|class|struct keyword
    {
        // split on whitespace in between "class SomeClass*" and take the part on the right
        auto str = type.getAsString(*ClangParser::g_printPolicy);
        auto ws = str.find_first_of(' ', 0);
        if (ws != std::string::npos)
        {
            str = str.substr(ws + 1);
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
    else
    {
        res = type.getAsString(*ClangParser::g_printPolicy);
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
    default:
        return type.getAsString(*ClangParser::g_printPolicy);
    }
    assert(bt != nullptr && "clang::BuiltinType expected");
}


std::string DlangBindGenerator::sanitizedIdentifier(const std::string &id)
{
    if (std::find(reservedIdentifiers.begin(), reservedIdentifiers.end(), id) != reservedIdentifiers.end())
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

    if (numBases > 0)
        out << " : ";

    // some quirks with array range ret type...
    for (const auto b : decl->bases())
    {
        numBases--;
        out << toDStyle(b.getType());
        if (numBases != 0)
            out << ", ";
    }
}


void DlangBindGenerator::innerDeclIterate(const clang::RecordDecl *decl)
{
    for (const auto it : decl->decls())
    {
        if (const auto e = llvm::dyn_cast<EnumDecl>(it))
        {
            onEnum(e);
        }
        if (const auto var = llvm::dyn_cast<VarDecl>(it))
        {
            //out << "@cppsize(" << finfo.Width / 8 << ")" << " ";
            out << "static ";
            out << getAccessStr(it->getAccess(), !decl->isClass()) << " ";
            out << toDStyle(var->getType()) << " ";
            out << sanitizedIdentifier(var->getName().str()) << ";";
            out << std::endl;
        }
        if (const auto d = llvm::dyn_cast<RecordDecl>(it))
        {
            if (d == decl || !d->isCompleteDefinition())
                continue;
            onStructOrClassEnter(d);
            onStructOrClassLeave(d);
        }
    }
}


void DlangBindGenerator::fieldIterate(const clang::RecordDecl *decl)
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

        bool isForwardDecl = true;
        if (rec)
            isForwardDecl = rec->getDefinition() == nullptr;

        TypeInfo finfo;
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
            it->getBitWidth()->printPretty(os, nullptr, *ClangParser::g_printPolicy);
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

        //out << "    ";
        if (bitfield)
        {
            IndentBlock _indent(out, 4);
            if (isPrevIsBitfield) // we are still on same line
                out << "," << std::endl;
            out << toDStyle(it->getType()) << ", ";
            out << "\"" << sanitizedIdentifier(it->getName().str()) << "\""
                << ", ";
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
            out << toDStyle(it->getType()) << " ";
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


void DlangBindGenerator::setOperatorOp(std::string &str, const char *action, bool templated /* = true */)
{
    if (templated)
    {
        str.append("(string op : \"");
        str.append(action);
        str.append("\")");
    }
    else
    {
        str.append(action);
    }
}


void DlangBindGenerator::methodIterate(const clang::CXXRecordDecl *decl)
{
    clang::ASTContext *ast = &decl->getASTContext();
    const auto mangleCtx = clang::MicrosoftMangleContext::create(*ast, ast->getDiagnostics());

    for (const auto m : decl->methods())
    {
        std::string mangledName;
        std::string funcName;
        bool noRetType = false;
        bool isClass = decl->isClass();
        bool hasConstPtrToNonConst = false;
        bool isStatic = m->isStatic();
        bool moveCtor = false;
        bool copyCtor = false;
        bool isCtor = false;
        bool isDefaultCtor = false;
        bool isDtor = false;
        bool isConversionOp = false;
        bool idAssign = false;
        bool disable = false;
        bool customMangle = false;

        // TODO: implicit methods starts at decl position itself, skip anything already listed
        std::string locString = m->getLocStart().printToString(decl->getASTContext().getSourceManager());
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
                noRetType = true;
                isCtor = true;
                isDefaultCtor = ct->isDefaultConstructor();
                moveCtor = ct->isMoveConstructor();
                copyCtor = ct->isCopyConstructor();
            }
            else if (auto dt = llvm::dyn_cast<CXXDestructorDecl>(m))
            {
                funcName = "~this";
                noRetType = true;
                isDtor = true;
            }
            else if (!m->isTemplated())
            {
                llvm::raw_string_ostream ostream(mangledName);
                mangleCtx->mangleName(m, ostream);
                ostream.flush();
            }
        }

        if (const auto conv = llvm::dyn_cast<CXXConversionDecl>(m))
        {
            const auto tn = toDStyle(conv->getConversionType());
            funcName = "opCast(TType:";
            funcName.append(tn);
            funcName.append(")");
            isConversionOp = true;
        }

        if (isOperator)
        {
            // get operator name and args
            const auto op = m->getOverloadedOperator();
            const auto psize = m->param_size();
            const bool isBinary = psize == 1;
            const bool isUnary = psize == 0;

            if (isBinary)
                funcName = "opBinary";
            else if (isUnary)
                funcName = "opUnary";
            else
                funcName = "op";

            switch (op)
            {
            case OverloadedOperatorKind::OO_Plus:
                setOperatorOp(funcName, "+"); 
                break;
            case OverloadedOperatorKind::OO_Minus:
                setOperatorOp(funcName, "-");
                break;
            case OverloadedOperatorKind::OO_Star:
                setOperatorOp(funcName, "*");
                break;
            case OverloadedOperatorKind::OO_Slash:
                setOperatorOp(funcName, "/");
                break;
            case OverloadedOperatorKind::OO_Percent:
                setOperatorOp(funcName, "%");
                break;
            case OverloadedOperatorKind::OO_Caret:
                setOperatorOp(funcName, "^");
                break;
            case OverloadedOperatorKind::OO_Amp:
                setOperatorOp(funcName, "&");
                break;
            case OverloadedOperatorKind::OO_Pipe:
                setOperatorOp(funcName, "|");
                break;
            case OverloadedOperatorKind::OO_Call:
                funcName = "op";
                setOperatorOp(funcName, "Call", false);
                break;
            case OverloadedOperatorKind::OO_Subscript:
                funcName = "op";
                setOperatorOp(funcName, "Index", false);
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
                funcName = "op_unot"; customMangle = true;
                break;
            case OverloadedOperatorKind::OO_ExclaimEqual:
                funcName = "op_ne"; customMangle = true;
                break;
            case OverloadedOperatorKind::OO_PlusEqual:
                funcName = "opOpAssign"; 
                setOperatorOp(funcName, "+");
                break;
            case OverloadedOperatorKind::OO_MinusEqual:
                funcName = "opOpAssign"; 
                setOperatorOp(funcName, "-");
                break;
            case OverloadedOperatorKind::OO_StarEqual:
                funcName = "opOpAssign"; 
                setOperatorOp(funcName, "*");
                break;
            case OverloadedOperatorKind::OO_SlashEqual:
                funcName = "opOpAssign"; 
                setOperatorOp(funcName, "/");
                break;
            case OverloadedOperatorKind::OO_PipeEqual:
                funcName = "opOpAssign"; 
                setOperatorOp(funcName, "|");
                break;
            case OverloadedOperatorKind::OO_AmpEqual:
                funcName = "opOpAssign"; 
                setOperatorOp(funcName, "&");
                break;
            case OverloadedOperatorKind::OO_LessLessEqual:
                funcName = "opOpAssign"; 
                setOperatorOp(funcName, "<<");
                break;
            case OverloadedOperatorKind::OO_GreaterGreaterEqual:
                funcName = "opOpAssign"; 
                setOperatorOp(funcName, ">>");
                break;
            case OverloadedOperatorKind::OO_Equal:
                if (m->getNumParams() == 1 
                    && m->getReturnType()->isReferenceType()
                    && m->parameters()[0]->getType()->getAsRecordDecl() == m->getReturnType()->getAsRecordDecl()
                ){
                    idAssign = true;
                }
                funcName = "op";
                setOperatorOp(funcName, "Assign", false);
                break;
            case OverloadedOperatorKind::OO_EqualEqual:
                funcName = "opEquals";
                break;
            default:
                setOperatorOp(funcName, getOperatorSpelling(op));
                break;
            }
        }

        if (customMangle)
        {
            // due to many little details unfortunately it's not (yet)
            if (ast->getLangOpts().isCompatibleWithMSVC(LangOptions::MSVC2015))
            {
                out << "@pyExtract(\"" 
                    <<     decl->getNameAsString() << "::" << m->getNameAsString()
                    << "\")" << "   pragma(mangle, " << "nsgen."
                    <<     decl->getNameAsString() << "_" << funcName << ".mangleof)"
                    << std::endl;
            }
            else 
            {
                out << "pragma(mangle, \"" << mangledName << "\")" << std::endl;
            }
        }

        if (m->isDefaulted())
            out << "// (default) ";

        // default ctor for struct not allowed
        if ((!isClass && isDefaultCtor) || idAssign)
        {
            out << "// ";
        }

        if (m->isInlined() && m->hasInlineBody())
        {
            out << "/* inline */ ";
        }

        out << getAccessStr(m->getAccess(), !isClass) << " ";

        if (m->hasAttr<OverrideAttr>())
            out << "override ";

        if (isStatic)
            out << "static ";

        if (isClass && !m->isVirtual())
            out << "final ";

        if (moveCtor && m->getAccess() == AccessSpecifier::AS_private)
        {
            out << "@disable ";
        }

        if (!noRetType) // ctor or dtor for example doesn't have it
            out << toDStyle(m->getReturnType()) << " ";

        if (isOperator || m->getIdentifier() == nullptr)
            out << funcName;
        else
            out << m->getName().str();
        out << "(";
        for (const auto fp : m->parameters())
        {
            // check if is 'type * const'
            if (fp->getType()->isPointerType() && fp->getType().isConstQualified() && !fp->getType()->getPointeeType().isConstQualified())
                hasConstPtrToNonConst = true;

            out << toDStyle(fp->getType()) << " " << sanitizedIdentifier(fp->getName().str());
            if (fp != *(m->param_end() - 1))
                out << ", ";
        }
        out << ")";

        if (nogc) 
            out << " @nogc ";

        if (hasConstPtrToNonConst)
            out << "/* WINDOWS MANGLING OVERRIDE NOT YET FINISHED, ADJUST MANUALLY! */ ";
        
        // write function body
        if (m->isInlined() && m->hasInlineBody())
        {
            std::string s;
            llvm::raw_string_ostream os(s);
            std::stringstream ss;
            if (m->getBody())
            {
                m->getBody()->printPretty(os, nullptr, *ClangParser::g_printPolicy);
                ss << os.str();
                std::string line;
                while (std::getline(ss, line))
                {
                    textReplaceArrowColon(line);
                    out << line << std::endl;
                }
            }
            else 
                if (isCtor && !(decl->isTemplated() || m->isTemplated())) // this might mean all the stuff done by initializer lists
                    out << " {}";
                else // something happened, just finish the function decl
                    out << ";";
        }
        else
        {
            out << ";" << std::endl;
        }
    
        out << std::endl;
    }

    delete mangleCtx;
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