// llvm using some black magic
#pragma warning(push)
#pragma warning( disable: 4146 4996 )

// CURRENT PROBLEMS & TO-DO LIST:
// - [REFACTOR] unify method/freefunc handling
// - [REFACTOR] universal indentation stack for "out" printer
// - [FEATURE] operator overload handling
// - [FEATURE] move ctor skip? "MyClass(MyClass&& other);" (@disable private postblit?)
// - [FEATURE] fields default initializers
// - [FEATURE] bit fields splitting 
// - [FEATURE] move away from eager writting, required for proper handling C style enum - typedef enum {wutever1, wutever2} enumName;
// - replace -> and :: with dot, initializer expr with macro results in <null epxr>, 

#include <memory>
#include <iostream>
#include <ostream>
#include <fstream>
#include <sstream>
#include <streambuf>
#include <string>
#include <algorithm>
#include <unordered_map>
#include <utility>
#include <list>
#include <tuple>

#ifdef _WIN32
#include <corecrt_wstdio.h>
#else
#include <stdio.h>
#endif


#include "rapidjson/document.h"
//#include <clang-c/Index.h>
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
#include "clang/Tooling/Tooling.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/Parse/ParseAST.h"
#include "clang/CodeGen/CodeGenAction.h"
#include "clang/Lex/PreprocessorOptions.h"
//#include "clang/Tooling/Core/QualTypeNames.h" // fully qualified template args lookup 
#include "llvm/Support/raw_ostream.h"


#include "gentool/abstract_generator.h"
#include "gentool/options.h"
#include "ppcallbacks.h"
#include "iohelpers.h"
#include "dlang_gen.h"

#pragma warning(pop)


using namespace clang;
using namespace clang::ast_matchers;
using namespace clang::driver;
using namespace clang::tooling;
using namespace gentool;

#if __cpp_lib_filesystem
namespace fs = std::filesystem;
#elif __cpp_lib_experimental_filesystem
namespace fs = std::experimental::filesystem;
#else error "C++17 Filesystem feature not detected"
#endif






template<typename T = IAbstractGenerator>
class RecordDeclMatcher : public MatchFinder::MatchCallback
{
public:
	virtual void run(const MatchFinder::MatchResult &Result);
	T& getImpl() { return impl; }
private:
	T impl;
};

namespace ext { 
	AST_MATCHER(clang::FunctionDecl, isOverloadedOperator) {
		return Node.isOverloadedOperator();
	}
}



// walk on all struct/class/unions that are found on top level or in namespaces
DeclarationMatcher recordDeclMatcher = recordDecl(
	isDefinition(), 
	hasDeclContext( 
		anyOf(
			translationUnitDecl(), 
			namespaceDecl()
		)
	)
).bind("recordDecl");

// free functions
DeclarationMatcher funDeclMatcher = functionDecl(
	//isDefinition(), 
	hasDeclContext( 
		anyOf(
			translationUnitDecl(), 
			namespaceDecl()
		)
	),
	unless(anyOf(cxxMethodDecl(), ext::isOverloadedOperator()))
).bind("funDecl");

// typedefs
DeclarationMatcher typedefDeclMatcher = typedefDecl(
	hasDeclContext( 
		anyOf(
			translationUnitDecl(), 
			namespaceDecl()
		)
	)
).bind("typedefDecl");

// enum & enum class
DeclarationMatcher enumDeclMatcher = enumDecl(
	hasDeclContext( 
		anyOf(
			translationUnitDecl(), 
			namespaceDecl()
		)
	)
).bind("enumDecl");

// globals
DeclarationMatcher globalVarsMatcher = varDecl(
	hasDeclContext( 
		anyOf(
			translationUnitDecl(), 
			namespaceDecl()
		)
	), 
	unless(anyOf(typedefNameDecl(), typedefDecl(), parmVarDecl()))
).bind("globalVarDecl");



template<typename T>
void RecordDeclMatcher<T>::run(const MatchFinder::MatchResult &Result)
{
	SourceManager &srcMgr = Result.Context->getSourceManager();
	const clang::RecordDecl *st = Result.Nodes.getNodeAs<RecordDecl>("recordDecl");
	const clang::TypedefDecl *td = Result.Nodes.getNodeAs<TypedefDecl>("typedefDecl");
	const clang::FunctionDecl *fd = Result.Nodes.getNodeAs<FunctionDecl>("funDecl");
	const clang::EnumDecl *ed = Result.Nodes.getNodeAs<EnumDecl>("enumDecl");
	const clang::VarDecl *glob = Result.Nodes.getNodeAs<VarDecl>("globalVarDecl");
	bool isClass = false;
	std::string path;
	const clang::Decl* rec; 

	if (st)
	{
		rec = st;
	}
	else if (td)
	{
		rec = td;
	}
	else if (fd)
	{
		rec = fd;
	}
	else if (ed)
	{
		rec = ed;
	}
	else if (glob)
	{
		rec = glob;
	}

	path = rec->getLocStart().printToString(srcMgr);
	if (!impl.isRelevantPath(path))
		return;

	if (ed)
	{
		impl.onEnum(ed);
	}
	else if (st)
	{
		if (!st->isCompleteDefinition())
			return;


		if (st->isClass() || st->isStruct())
		{
			isClass = true;
			impl.onStructOrClassEnter(st);
		}

		if (isClass)
		{
			impl.onStructOrClassLeave(st);
		}
	}
	else if (td)
	{
		impl.onTypedef(td);
	}
	else if (fd)
	{
		impl.onFunction(fd);
	}
	else if (glob)
	{
		impl.onGlobalVar(glob);
	}
}


std::string diffPathPart(const std::string& a, const std::string& b)
{
	if (a.compare(b) == 0)
		return a;

	auto off = a.find_first_of(b);
	if (off != std::string::npos)
	{
		return a.substr(b.length()+1);
	}
	return a;
}


void _readJSONArrayMember(decltype(InputOptions::paths)& arr, const rapidjson::Value& v, const std::string_view member)
{
	if ( const auto it = v.FindMember(member.data()); it != v.MemberEnd() )
		for (const auto& e : it->value.GetArray())
			arr.push_back(std::string(e.GetString()));
	else 
		throw std::runtime_error("Member not found");
}

#include <cctype> // std::tolower
std::tuple<InputOptions, OutputOptions, bool> readJSON(const std::string_view data)
{
	static constexpr auto parserFlags = rapidjson::kParseTrailingCommasFlag | rapidjson::kParseCommentsFlag;
	static const std::vector<std::string> targets = {"d2", "d", "dlang"};
	InputOptions inopts;
	OutputOptions outopts;
	bool res = true;
	rapidjson::Document document;
	auto r = document.Parse<parserFlags>(data.data()).HasParseError();
	if (r) throw std::runtime_error("Malformed JSON");

	const auto& input = document["input"];
	inopts.standard = std::string(input["std"].GetString());

	_readJSONArrayMember(inopts.paths, input, "paths");
	_readJSONArrayMember(inopts.includes, input, "includes");
	_readJSONArrayMember(inopts.cflags, input, "cflags");
	_readJSONArrayMember(inopts.systemIncludes, input, "system");
	_readJSONArrayMember(inopts.defines, input, "defines");

	const auto& output = document["output"];
	auto outpath = output.FindMember("path");
	if (outpath != output.MemberEnd())
		outopts.path = std::string(outpath->value.GetString());
	else 
		outopts.path = "generated.out";

	auto outtarget = output.FindMember("target");
	if (outtarget != output.MemberEnd())
	{
		auto str = std::string(outtarget->value.GetString());
		std::transform(str.begin(), str.end(), str.begin(),
			[](unsigned char c) { return std::tolower(c); }
		);
		if ( std::find(targets.begin(), targets.end(), str) == targets.end() )
			throw std::runtime_error("Only D target supported at this moment");
	}

	auto attrnogc = output.FindMember("attr-nogc");
	if (attrnogc != output.MemberEnd())
	{
		auto nogc = attrnogc->value.GetBool();
		if (nogc)
			outopts.extras.push_back("attr-nogc");
	}
	
	return std::make_tuple(inopts, outopts, res);
}

std::string readFile(const std::string_view path)
{
	std::ifstream t(path.data());
	t.seekg(0, std::ios::end);
	size_t size = t.tellg();
	std::string buffer(size, ' ');
	t.seekg(0);
	t.read(&buffer[0], size);
	return buffer;
}


bool filterExt(const std::string& ext)
{
	static std::vector<std::string> validExts = { ".h", ".hpp", ".hxx", /*".c", ".cpp", ".cxx"*/ }; // not sure if parsing source files is good idea
	if ( std::find(validExts.begin(), validExts.end(), ext) != validExts.end() )
			return true;
	return false;
}

// useful stuff
//https://gist.github.com/kennytm/4178490 template instantiation from decl
//https://stackoverflow.com/questions/40740604/how-do-i-get-the-mangled-name-of-a-nameddecl-in-clang name mangling

void llvmOnError(void *user_data, const std::string& reason, bool gen_crash_diag)
{
	std::cout << reason << std::endl;
}

// Frontend action stuff:

namespace
{
// Consumer is responsible for setting up the callbacks.
class PPCallbackConsumer : public ASTConsumer
{
public:
	PPCallbackConsumer(Preprocessor &PP, DlangBindGenerator* Listener)
	{
		// PP takes ownership.
		PP.addPPCallbacks(llvm::make_unique<PPCallbacksTracker>(PP, Listener));
	}
};

class PPCallbackAction : public SyntaxOnlyAction
{
public:
	PPCallbackAction(DlangBindGenerator* Listener) : Listener(Listener) {}

protected:
	std::unique_ptr<clang::ASTConsumer>
	CreateASTConsumer(CompilerInstance &CI, StringRef InFile) override
	{
		Listener->setSourceManager(&CI.getSourceManager());
		return llvm::make_unique<PPCallbackConsumer>(CI.getPreprocessor(), Listener);
	}
private:
	DlangBindGenerator* Listener;
};

class PPCallbacksFrontendActionFactory : public FrontendActionFactory
{
public:
	PPCallbacksFrontendActionFactory(DlangBindGenerator* Listener) : Listener(Listener) {}

	PPCallbackAction *create() override
	{
		return new PPCallbackAction(Listener);
	}
private:
	DlangBindGenerator* Listener;
};

} // namespace

static llvm::cl::OptionCategory MyToolCategory("My tool options");



int main(int argc, const char **argv) 
{
	std::string path;

	if (argc > 1)
		path = argv[1];
	else
	{
		std::cout << "error: .json configuration file expected" << std::endl;
		return 1;
	}

	auto [input, output, res] = readJSON(readFile(path));
	if (!res) 
	{
		std::cout << "Error while reading JSON config." << std::endl;
		return 1;
	}

	clang::ast_matchers::MatchFinder Finder;

	RecordDeclMatcher<DlangBindGenerator> allRecords;
	Finder.addMatcher(recordDeclMatcher, &allRecords);
	Finder.addMatcher(typedefDeclMatcher, &allRecords);
	Finder.addMatcher(funDeclMatcher, &allRecords);
	Finder.addMatcher(enumDeclMatcher, &allRecords);
	Finder.addMatcher(globalVarsMatcher, &allRecords);

	bool isCpp = true;
	int standardVersion = -1;
	{
		clang::LangOptions langOptions;

		if (!input.standard.empty())
		{
			// get actual standard
			auto pp = input.standard.find("++");
			if (pp == std::string::npos)
			{
				isCpp = false;
				pp = 1;
			}
			else
			{
				pp += 2;
			}
			standardVersion = std::stoi(input.standard.substr(pp));
		}

		langOptions.CPlusPlus = isCpp;
		langOptions.C99 = !isCpp && standardVersion == 99;
		langOptions.C11 = !isCpp && standardVersion == 11;
		langOptions.C17 = !isCpp && standardVersion == 17;
		langOptions.CPlusPlus11 = isCpp && standardVersion >= 11;
		langOptions.CPlusPlus14 = isCpp && standardVersion >= 14;
		langOptions.CPlusPlus17 = isCpp && standardVersion >= 17;
		langOptions.CPlusPlus2a = isCpp && standardVersion >= 18; // TODO: actual repr for this
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
		//#endif
		
		if (DlangBindGenerator::g_printPolicy)
			delete DlangBindGenerator::g_printPolicy;
		DlangBindGenerator::g_printPolicy = new PrintingPolicy(langOptions);
		DlangBindGenerator::g_printPolicy->SuppressScope = true;
		DlangBindGenerator::g_printPolicy->Bool = true;
		DlangBindGenerator::g_printPolicy->SuppressTagKeyword = true;
		DlangBindGenerator::g_printPolicy->ConstantsAsWritten = true;   // prevent prettifying masks and other unpleasent things
		DlangBindGenerator::g_printPolicy->SuppressImplicitBase = true; // no "this->",  please
	}

	std::vector<std::string> sources;
	std::vector<const char*> ptrList;
	std::error_code _;
	sources.push_back(argv[0]); // must go first
	for (const auto& p : input.paths)
	{
		if (fs::is_directory(p))
		for (auto& it : fs::recursive_directory_iterator(fs::canonical(p, _)))
		{
			if (fs::is_directory(it))
				continue;

			if ( !it.path().has_extension() )
				continue;
			
			if ( !filterExt(it.path().extension().string()) )
				continue;

			sources.push_back(it.path().string());
		}
		else sources.push_back(p);
	}
	sources.push_back("--");  // special token, after which follows the compiler flags

	std::string stdVersion = "-std=";
	if (isCpp)
	{
		sources.push_back("-x"); sources.push_back("c++");
	}
	if (standardVersion != -1) 
	{
		stdVersion.append(input.standard);
	}
	else
	{
		// defaults to c11/c++11
		if (isCpp) 
			stdVersion.append("c++11"); 
		else 
			stdVersion.append("c11");
	}
	sources.push_back(stdVersion);

	for (const auto& flag : input.cflags)
		sources.push_back(flag);
	for (const auto& p : input.includes)
		sources.push_back(std::string("-I").append(p));
	for (const auto& p : input.systemIncludes)
		sources.push_back(std::string("-I").append(p));
	for (const auto& p : input.defines)
		sources.push_back(std::string("-D").append(p));

	for(const auto& p : sources)  // merge all
	{
		ptrList.push_back(p.c_str());
	}
	int argn = ptrList.size();

	llvm::install_fatal_error_handler(llvmOnError);
	CommonOptionsParser op(argn, ptrList.data(), MyToolCategory); // feed in argc, argv, kind of
	ClangTool tool(op.getCompilations(), op.getSourcePathList());

	allRecords.getImpl().setOptions(&input, &output);
	allRecords.getImpl().prepare();

	// Run preprocessor callbacks pass before normal pass
	PPCallbacksFrontendActionFactory Factory(&allRecords.getImpl());
	tool.run(&Factory);

	auto toolres = tool.run(newFrontendActionFactory(&Finder).get());

	allRecords.getImpl().finalize();

	return toolres;
}