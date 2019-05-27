#pragma once

#include "clang/Lex/Preprocessor.h"
#include "clang/Lex/PPCallbacks.h"

class DlangBindGenerator;

class PPCallbacksTracker : public clang::PPCallbacks
{
private:
	clang::Preprocessor &PP;
    clang::SourceManager* SM;
	DlangBindGenerator* Listener;

  public:
	PPCallbacksTracker(clang::Preprocessor &PP, DlangBindGenerator* Listener) 
		: PP(PP), Listener(Listener) 
	{ 
		SM = &PP.getSourceManager(); 
	}

	~PPCallbacksTracker() override {}

	bool skipPath(llvm::StringRef path);

	void dumpMacroText(const clang::MacroDirective *MacroDirective);

	void MacroDefined(
        const clang::Token &MacroNameTok, 
        const clang::MacroDirective *MacroDirective) override;

	void MacroUndefined(
		const clang::Token &MacroNameTok,
		const clang::MacroDefinition &MacroDefinition,
		const clang::MacroDirective *Undef) override;

	void InclusionDirective(
		clang::SourceLocation HashLoc,
		const clang::Token &IncludeTok,
		llvm::StringRef FileName,
		bool IsAngled,
		clang::CharSourceRange FilenameRange,
		const clang::FileEntry *File,
		llvm::StringRef SearchPath,
		llvm::StringRef RelativePath,
		const clang::Module *Imported,
		clang::SrcMgr::CharacteristicKind FileType) override;

};
