// llvm using some black magic
#pragma warning(push)
#pragma warning( disable: 4146 4996 )

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
#include <regex>
#include <cstdlib>

#ifdef _WIN32
#include <corecrt_wstdio.h>
#else
#include <stdio.h>
#endif


#include "rapidjson/document.h"
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
#include "llvm/Support/raw_ostream.h"


#include "gentool/abstract_generator.h"
#include "gentool/options.h"
#include "gentool/defines.h"
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
#else 
#error "C++17 Filesystem feature not detected"
#endif


static constexpr const char* HELP_MSG = 
R"(Oh My Gentool - v0.4.0
Generates D bindings from C/C++ code

USAGE:
	gentool <project.json>

	<project.json>: .json configuration file path
)";




template<typename T = IAbstractGenerator>
class [[deprecated("Deprecated. Consider using normal clang::ASTConsumer instead")]] 
	RecordDeclMatcher : public MatchFinder::MatchCallback
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
	
	AST_MATCHER(clang::Decl, topLinkageSpecDecl) {
		if (Node.getKind() != Decl::Kind::LinkageSpec)
			return false;
		return Node.getDeclContext()->isTranslationUnit() 
			|| Node.getDeclContext()->isNamespace();
	}
}



// walk on all struct/class/unions that are found on top level or in namespaces
DeclarationMatcher recordDeclMatcher = recordDecl(
	hasDeclContext( 
		anyOf(
			translationUnitDecl(), 
			namespaceDecl(),
			ext::topLinkageSpecDecl()
		)
	)
).bind("recordDecl");

// free functions
DeclarationMatcher funDeclMatcher = functionDecl(
	unless(
		hasAncestor(recordDecl())
	),
	unless(anyOf(cxxMethodDecl(), ext::isOverloadedOperator()))
).bind("funDecl");

// typedefs
DeclarationMatcher typedefDeclMatcher = typedefDecl(
	hasDeclContext( 
		anyOf(
			translationUnitDecl(), 
			namespaceDecl(),
			ext::topLinkageSpecDecl()
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
			namespaceDecl(),
			ext::topLinkageSpecDecl()
		)
	), 
	unless(anyOf(typedefNameDecl(), typedefDecl(), parmVarDecl()))
).bind("globalVarDecl");


std::string getPathForDecl(const Decl* decl, SourceManager& srcMgr)
{
	std::string path;

	auto sfile = srcMgr.getFileID(decl->getLocation());
	auto* f = srcMgr.getFileEntryForID(sfile);
	if (f)
		path = std::string(f->getName());
	else { 
		std::string s;
		llvm::raw_string_ostream os(s);
		auto sloc = sfile.isValid() ? decl->getLocation() : decl->getBeginLoc();
		srcMgr.getExpansionLoc(sloc).print(os, srcMgr);
		path = os.str();
	}

	return path;
}


bool isTopLevelDecl(Decl* decl)
{
	auto declContext = decl->getDeclContext();
	if (declContext->getDeclKind() == Decl::Kind::ExternCContext 
	|| declContext->getDeclKind() == Decl::Kind::LinkageSpec)
		declContext = declContext->getParent();
	return declContext 
		&& (declContext->getDeclKind() == Decl::Kind::TranslationUnit
			|| declContext->getDeclKind() == Decl::Kind::Namespace);
}


// Do normal generator action on decl
void handleDecl(Decl* decl, IAbstractGenerator* handler)
{
	auto path = getPathForDecl(decl, decl->getASTContext().getSourceManager());
	if (!handler->isRelevantPath(path))
		return;

	if (isa<FunctionDecl>(decl))
	{
		if (isTopLevelDecl(decl))
			handler->onFunction(cast<FunctionDecl>(decl));
	}

	else if (isa<RecordDecl>(decl))
	{
		auto rd = cast<RecordDecl>(decl);
		handler->onStructOrClassEnter(rd);
		handler->onStructOrClassLeave(rd);
	}

	else if (isa<FunctionTemplateDecl>(decl))
	{
		auto ft = cast<FunctionTemplateDecl>(decl);
		if (ft && ft->getTemplatedDecl())
			handleDecl(ft->getTemplatedDecl(), handler);
	}

	else if (isa<ClassTemplateDecl>(decl))
	{
		auto ct = cast<ClassTemplateDecl>(decl);
		if (ct && ct->getTemplatedDecl())
			handleDecl(ct->getTemplatedDecl(), handler);
	}

	else if (isa<EnumDecl>(decl))
	{
		handler->onEnum(cast<EnumDecl>(decl));
	}

	else if (isa<TypedefDecl>(decl))
	{
		handler->onTypedef(cast<TypedefDecl>(decl));
	}

	else if (isa<UsingDecl>(decl))
	{
		handler->onUsingDecl(cast<UsingDecl>(decl));
	}

	else if (isa<TypeAliasDecl>(decl))
	{
		handler->onTypeAliasDecl(cast<TypeAliasDecl>(decl));
	}

	else if (isa<VarDecl>(decl))
	{
		handler->onGlobalVar(cast<VarDecl>(decl));
	}

	else if (isa<NamespaceDecl>(decl))
	{
		auto ns = cast<NamespaceDecl>(decl);
		for (auto& d : ns->decls())
			handleDecl(d, handler);
	}

	else if (isa<LinkageSpecDecl>(decl))
	{
		auto ec = cast<LinkageSpecDecl>(decl);
		for (auto d : ec->decls())
			handleDecl(d, handler);
	}

	else if (isa<StaticAssertDecl>(decl))
	{
		auto sa = cast<StaticAssertDecl>(decl);
		handler->onStaticAssertDecl(sa);
	}

	else if (isa<UsingShadowDecl>(decl))
	{
		// do nothing
		// this is sister decl for using decl which introduces scope overloads
		// similar to D alias
		//
		// class A : B
		// {
		//   alias foo = B.foo;
		// }
	}

	else
	{
		llvm::outs() << "INFO: unhandled decl of kind '" << decl->getDeclKindName() << "' at " << path << '\n';
		decl->dumpColor();
	}
}


template<typename T>
void RecordDeclMatcher<T>::run(const MatchFinder::MatchResult &Result)
{
	SourceManager &srcMgr = Result.Context->getSourceManager();
	const clang::RecordDecl *st = Result.Nodes.getNodeAs<RecordDecl>("recordDecl");
	const clang::TypedefDecl *td = Result.Nodes.getNodeAs<TypedefDecl>("typedefDecl");
	const clang::FunctionDecl *fd = Result.Nodes.getNodeAs<FunctionDecl>("funDecl");
	const clang::EnumDecl *ed = Result.Nodes.getNodeAs<EnumDecl>("enumDecl");
	const clang::VarDecl *glob = Result.Nodes.getNodeAs<VarDecl>("globalVarDecl");
	const clang::Decl* rec; 

	// TODO: this can be compacted if all matches has same binding name
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

	handleDecl( const_cast<Decl*>(rec), &impl);
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
		throw std::runtime_error("Member '" + std::string(member) + "' not found");
}

#include <cctype> // std::tolower
std::tuple<InputOptions, OutputOptions> readJSON(const std::string_view data)
{
	static constexpr auto parserFlags = rapidjson::kParseTrailingCommasFlag | rapidjson::kParseCommentsFlag;
	static const std::vector<std::string> targets = {"d2", "d", "dlang"};
	InputOptions inopts;
	OutputOptions outopts;
	rapidjson::Document document;
	auto r = document.Parse<parserFlags>(data.data()).HasParseError();
	if (r) throw std::runtime_error("Malformed JSON");
	
	if (!document.IsObject())
		throw std::runtime_error("JSON root expected to be an object");

	if (!(document.HasMember("input") && document.HasMember("output")))
		throw std::runtime_error("JSON config expected to have 'input' and 'output' objects");

	const auto& input = document["input"];

	if (!input.HasMember("std"))
		throw std::runtime_error("'input' expected to have 'std' property");
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

	auto opt_refs = output.FindMember("no-param-refs");
	if (opt_refs != output.MemberEnd())
	{
		auto val = opt_refs->value.GetBool();
		if (val)
			outopts.extras.push_back("no-param-refs");
	}

	auto skip_body = output.FindMember("skip-bodies");
	if (skip_body != output.MemberEnd())
	{
		auto val = skip_body->value.GetBool();
		if (val)
			outopts.extras.push_back("skip-bodies");
	}

	auto mangle_all = output.FindMember("mangle-all");
	if (mangle_all != output.MemberEnd())
	{
		auto val = mangle_all->value.GetBool();
		if (val)
			outopts.extras.push_back("mangle-all");
	}
	
	return std::make_tuple(inopts, outopts);
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

// Find substring with special pattern in form $(VARIABLE_NAME)
std::tuple<size_t, size_t> findPathVar(const std::string& path)
{
	std::regex e(R"(\$\(\w+\))");
	std::smatch m;
	std::regex_search(path, m, e);
	return std::make_tuple(m.position(), m.begin() != m.end() ? m.begin()->length() : 0);
}

// Replaces substring with environment variable (always takes value before first separator)
void replaceEnvVar(std::string& path)
{
#ifdef _WIN32
	const char sep = ';';
#else
	const char sep = ':';
#endif
	auto [pos, len] = findPathVar(path);
	if (len > 0)
	{
		// skips leading '$(' and closing ')'
		if (auto env = std::getenv(path.substr(pos+2, len-3).c_str())) 
		{
			auto stop = std::find(env, env+strlen(env), sep);
			path.replace(pos, len, stop == 0 ? env : std::string(env, stop));
		}
	}
}


void replaceEnvVars(gentool::InputOptions& input)
{
	for (auto& p: input.paths)
		replaceEnvVar(p);
	for (auto& p: input.includes)
		replaceEnvVar(p);
	for (auto& p: input.systemIncludes)
		replaceEnvVar(p);
}


std::tuple<int, bool> initGlobalLangOptions(gentool::InputOptions& input)
{
	int standardVersion = -1;
	bool isCpp = true;

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
#if (LLVM_VERSION_MAJOR < 11)
	langOptions.CPlusPlus2a = isCpp && standardVersion >= 20;
#else
	langOptions.CPlusPlus20 = isCpp && standardVersion >= 20;
#endif
	langOptions.Bool = 1;
	langOptions.RTTI = 0;
	langOptions.DoubleSquareBracketAttributes = 1;
	//#if defined(_MSC_VER)
	//langOptions.MSCompatibilityVersion = LangOptions::MSVCMajorVersion::MSVC2015;
	//langOptions.MSCompatibilityVersion = 19;
	//langOptions.MicrosoftExt = 1;
	//langOptions.MSVCCompat = !isCpp;
	//langOptions.MSBitfields = 1;
	langOptions.DeclSpecKeyword = 1;
	//langOptions.DelayedTemplateParsing = 1; // MSVC parses templates at the time of actual use
	//#endif
	
	if (DlangBindGenerator::g_printPolicy)
		delete DlangBindGenerator::g_printPolicy;
	DlangBindGenerator::g_printPolicy = new PrintingPolicy(langOptions);
	DlangBindGenerator::g_printPolicy->SuppressScope = true;
	DlangBindGenerator::g_printPolicy->Bool = true;
	DlangBindGenerator::g_printPolicy->SuppressTagKeyword = true;
	DlangBindGenerator::g_printPolicy->ConstantsAsWritten = true;   // prevent prettifying masks and other unpleasent things
	DlangBindGenerator::g_printPolicy->SuppressImplicitBase = true; // no "this->",  please

	return std::make_tuple(standardVersion, isCpp);
}


std::vector<std::string> collectInputSources(gentool::InputOptions& input)
{
	std::vector<std::string> sources;
	std::error_code _;
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
	return sources;
}


std::vector<std::string> makeCompilerCommandLineArgs(gentool::InputOptions& input, const std::string& programPath, int standardVersion, bool isCpp)
{
	std::vector<std::string> cmd;
	cmd.push_back(programPath); // must go first
	{
		auto src = collectInputSources(input);
		cmd.insert(cmd.end(), src.begin(), src.end());
	}
	cmd.push_back("--");  // special token, after which follows the compiler flags

	std::string stdVersion = "-std=";
	if (isCpp)
	{
		cmd.push_back("-x"); cmd.push_back("c++");
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
	cmd.push_back(stdVersion);

	for (const auto& flag : input.cflags)
		cmd.push_back(flag);
	for (const auto& p : input.includes)
		cmd.push_back(std::string("-I").append(p));
	for (const auto& p : input.systemIncludes)
		cmd.push_back(std::string("-I").append(p));
	for (const auto& p : input.defines)
		cmd.push_back(std::string("-D").append(p));

	return cmd;
}


void llvmOnError(void *user_data, const std::string& reason, bool gen_crash_diag)
{
	std::cout << reason << std::endl;
}

// Frontend action stuff:


// Combines preprocessor and AST callbacks 
class GentoolASTConsumer : public ASTConsumer
{
public:
	GentoolASTConsumer(CompilerInstance &CI, DlangBindGenerator* Listener) : Listener(Listener), CI(CI)
	{
		auto& PP = CI.getPreprocessor();
		// PP takes ownership.
		PP.addPPCallbacks(std::make_unique<PPCallbacksTracker>(PP, Listener));
	}

	void HandleTranslationUnit(ASTContext &Ctx) 
	{
		Listener->setSema(&CI.getSema());
		if (auto tu = Ctx.getTranslationUnitDecl())
			for(auto d : tu->decls())
				handleDecl(d, Listener);
		Listener->onEndFile(std::string_view());
	}

private:
	DlangBindGenerator* Listener;
	CompilerInstance &CI;
};

class GentoolGenAction : public ASTFrontendAction
{
public:
	GentoolGenAction(DlangBindGenerator* Listener) : Listener(Listener) {}

protected:
	std::unique_ptr<clang::ASTConsumer>
	CreateASTConsumer(CompilerInstance &CI, StringRef InFile) override
	{
		Listener->setSourceManager(&CI.getSourceManager());
		return std::make_unique<GentoolASTConsumer>(CI, Listener);
	}
private:
	DlangBindGenerator* Listener;
};

class GentoolFrontendActionFactory : public FrontendActionFactory
{
public:
	GentoolFrontendActionFactory(DlangBindGenerator* Listener) : Listener(Listener) {}

#if (LLVM_VERSION_MAJOR < 11)
	GentoolGenAction *create() override
	{
		return new GentoolGenAction(Listener);
	}
#else
	std::unique_ptr<FrontendAction> create() override
	{
		return std::make_unique<GentoolGenAction>(Listener);
	}
#endif

private:
	DlangBindGenerator* Listener;
};


static llvm::cl::OptionCategory MyToolCategory("My tool options");

extern"C" API_EXPORT int gentool_run(int compilerArgc, const char** compilerArgv, CInputOptions inOpts, COutputOptions outOpts)
{
	gentool::InputOptions input(inOpts);
	gentool::OutputOptions output(outOpts);

	initGlobalLangOptions(input);

	llvm::install_fatal_error_handler(llvmOnError);

	auto op = CommonOptionsParser::create(compilerArgc, compilerArgv, MyToolCategory); // feed in argc, argv, kind of
	if (!op)
		throw std::runtime_error{"Could not create the CommonOptionsParser! Please verify your command-line arguments."};
	ClangTool tool(op->getCompilations(), op->getSourcePathList());

	DlangBindGenerator generator;

	generator.setOptions(&input, &output);
	generator.prepare();

	GentoolFrontendActionFactory Factory(&generator);
	auto toolres = tool.run(&Factory);

	generator.finalize();

	return toolres;
}

#ifndef USE_LIB_TARGET


int main(int argc, const char **argv) 
{
	if (argc < 2)
	{
		std::cout << HELP_MSG << std::endl;
		return 1;
	}

	gentool::InputOptions input;
	gentool::OutputOptions output;
	
	try 
	{
		std::string path = argv[1];
		std::tie(input, output) = readJSON(readFile(path));
	}
	catch (const std::runtime_error& err)
	{
		std::cout << "ERROR: " << err.what() << std::endl;
		return 1;
	}

	replaceEnvVars(input);

	auto [standardVersion, isCpp] = initGlobalLangOptions(input);
	auto programPath = std::string(argv[0]);
	std::vector<std::string> cmd = makeCompilerCommandLineArgs(input, programPath, standardVersion, isCpp);
	std::vector<const char*> cmdArgv;
	// make array of string pointers
	for(const auto& p : cmd)
	{
		cmdArgv.push_back(p.c_str());
	}
	int cmdArgc = (int)cmdArgv.size();

	return gentool_run(cmdArgc, cmdArgv.data(), input.toPlainC(), output.toPlainC());
} // main()

#endif // ifndef USE_LIB_TARGET
