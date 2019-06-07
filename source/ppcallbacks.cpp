#include "ppcallbacks.h"

#include <iostream>
#include <string_view>

#include "clang/Lex/PreprocessorOptions.h"

#include "dlang_gen.h"


bool PPCallbacksTracker::skipPath(llvm::StringRef path)
{
    return false;
}

void PPCallbacksTracker::dumpMacroText(const clang::MacroDirective *MacroDirective)
{
    if (!MacroDirective)
        return;

    if (auto mi = MacroDirective->getMacroInfo())
    {
        if (mi->isUsedForHeaderGuard())
            return;
        if (mi->getNumParams())
            std::cout << "(";
        for (auto p : mi->params())
        {
            std::cout << p->getName().str();
            if (p != *(mi->param_end()-1)) std::cout << ", ";
        }
        if (mi->getNumParams())
            std::cout << ")" << std::endl;

        bool prevHash = false; // is the last token was a '#'
        for (auto tok : mi->tokens())
        {
            if (tok.isAnyIdentifier())
            {
                std::cout << tok.getIdentifierInfo()->getName().str() << " "; 
            }
            else if (tok.isLiteral())
            {
                std::cout << std::string_view(tok.getLiteralData(), tok.getLength()) << " ";
            }
            else if (auto kw = clang::tok::getKeywordSpelling(tok.getKind()))
            {
                std::cout << kw << " ";
            }
            else if (auto pu = clang::tok::getPunctuatorSpelling(tok.getKind()))
            { 
                static const std::vector<clang::tok::TokenKind> wstokens = { // tokens that needs ws after it
                    clang::tok::comma, clang::tok::r_paren, clang::tok::r_brace, clang::tok::semi
                };
                bool ws = std::find(wstokens.begin(), wstokens.end(), tok.getKind()) != wstokens.end();
                std::cout << pu;
                if (ws) std::cout << " ";
            }
            prevHash = tok.getKind() == clang::tok::hash;
        }
    }
}

void PPCallbacksTracker::MacroDefined(const clang::Token &MacroNameTok, const clang::MacroDirective *MacroDirective)
{
    if (!MacroDirective)
        return;

    if (skipPath(SM->getFilename(MacroDirective->getLocation())))
        return;

    // TODO: look into details about header guard, as this doesn't seems to work
    auto mi = MacroDirective->getMacroInfo();
    if (mi && mi->isUsedForHeaderGuard()) 
        return;

    // Skip all empty defines for now, this will cut header guards as well
    //if (mi && mi->tokens_empty() )
    //    return;

    auto id = MacroNameTok.getIdentifierInfo();
    //if (id)
    //{
    //    std::cout << "define " << id->getName().str();
    //}
    //dumpMacroText(MacroDirective);

    if (Listener)
        Listener->onMacroDefine(&MacroNameTok, MacroDirective);

    // this doesn't works in same pass, ugh...
    if (id && mi && (mi->isFunctionLike() || mi->isObjectLike()) )
    {
        auto name = MacroNameTok.getIdentifierInfo()->getName();
        auto& opts = PP.getPreprocessorOpts();
        opts.addMacroUndef(name);
    }

    //std::cout << std::endl;
}

void PPCallbacksTracker::MacroUndefined(
    const clang::Token &MacroNameTok,
    const clang::MacroDefinition &MacroDefinition,
    const clang::MacroDirective *Undef)
{
}


void PPCallbacksTracker::InclusionDirective(
    clang::SourceLocation HashLoc,
    const clang::Token &IncludeTok,
    llvm::StringRef FileName,
    bool IsAngled,
    clang::CharSourceRange FilenameRange,
    const clang::FileEntry *File,
    llvm::StringRef SearchPath,
    llvm::StringRef RelativePath,
    const clang::Module *Imported,
    clang::SrcMgr::CharacteristicKind FileType)
{
    Listener->onInclude(FileName, RelativePath, SearchPath, IsAngled, FileType);
}