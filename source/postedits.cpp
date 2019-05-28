#include "postedits.h"


void FeedbackContext::addAction(std::unique_ptr<FeedbackAction> action)
{
    actions.push_back(std::move(action));
}


void AddRvalueHackAction::apply(DlangBindGenerator* generator)
{
    if (auto st = generator->declLocations.find(declName); st != generator->declLocations.end())
    {
        static const std::string mixin = "mixin RvalueRef;";
        generator->bufOut.seekp(st->second.line);
        auto b = generator->bufOut.rdbuf()->str();
        b.insert(st->second.line, std::string(st->second.indent, ' ') + mixin);
        generator->bufOut.rdbuf()->str(b);
    }
}


void AddImportAction::apply(DlangBindGenerator* generator)
{
    //generator->bufOut.seekp(0);
    auto b = generator->bufOut.rdbuf()->str();
    b.insert(0, std::string("import ") + module + ";\n");
    generator->bufOut.rdbuf()->str(b);
}
