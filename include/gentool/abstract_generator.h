#pragma once

#include <string_view>

namespace clang
{
    class RecordDecl;
    class FunctionDecl;
    class EnumDecl;
    class TypedefDecl;
	class UsingDecl;
	class TypeAliasDecl;
    class VarDecl;
}

namespace gentool
{
	struct InputOptions;
	struct OutputOptions;

	//
	// Common interface definition for generators/handlers
	//
	class IAbstractGenerator
	{
	public:
		virtual void prepare()=0;
		virtual void finalize()=0;
		virtual void onBeginFile(const std::string_view file)=0;
		virtual void onEndFile(const std::string_view file)=0;
		virtual void onStructOrClassEnter(const clang::RecordDecl* decl)=0;
		virtual void onStructOrClassLeave(const clang::RecordDecl* decl)=0;
		virtual void onFunction(const clang::FunctionDecl* decl)=0;
		virtual void onEnum(const clang::EnumDecl* decl)=0;
		virtual void onTypedef(const clang::TypedefDecl* decl)=0;
		virtual void onUsingDecl(const clang::UsingDecl* decl)=0;
		virtual void onTypeAliasDecl(const clang::TypeAliasDecl* decl)=0;
		virtual void onStaticAssertDecl(const clang::StaticAssertDecl* decl)=0;
		virtual void onGlobalVar(const clang::VarDecl* decl)=0;
		virtual void setOptions(const InputOptions* inOpt, const OutputOptions* outOpt)=0;
		virtual bool isRelevantPath(const std::string_view path)=0; // if true file will be parsed, otherwise ignored
	};
}