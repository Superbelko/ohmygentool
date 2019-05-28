#pragma once

#include "dlang_gen.h"


class FeedbackAction
{
public:
    enum ActionKind {
        AK_OrderDependent,
        AK_AddRvalueHack,
        AK_OrderDependentLast,
        AK_AddImport
    };

    FeedbackAction(ActionKind Kind) : Kind(Kind) {}
    virtual void apply(DlangBindGenerator* generator) {}
    virtual ~FeedbackAction() {}
    ActionKind getKind() const { return Kind; }
private:
    const ActionKind Kind;
};


class AddRvalueHackAction : public FeedbackAction
{
public:
    AddRvalueHackAction(std::string declName) : FeedbackAction(AK_AddRvalueHack), declName(declName) {}

    virtual void apply(DlangBindGenerator* generator) override;

    static bool classof(const FeedbackAction* A)
    {
        return A->getKind() == AK_AddRvalueHack;
    }

    std::string declName;
};


class AddImportAction : public FeedbackAction
{
public:
    AddImportAction(std::string module) : FeedbackAction(AK_AddImport), module(module) {}

    virtual void apply(DlangBindGenerator* generator) override;

    static bool classof(const FeedbackAction* A)
    {
        return A->getKind() == AK_AddImport;
    }

    bool operator== (const AddImportAction& other) { return module == other.module; }

    std::string module;
};



class FeedbackContext
{
public:
    std::vector<std::unique_ptr<FeedbackAction>> actions;
    virtual ~FeedbackContext() {}
    virtual void addAction(std::unique_ptr<FeedbackAction> action);
};


class NullFeedbackContext : public FeedbackContext
{
public:
    virtual void addAction(std::unique_ptr<FeedbackAction> action) override {}
};