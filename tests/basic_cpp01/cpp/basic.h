#pragma once
#include <memory>
#include <cstring>

// Tests delimiter picking
// ISSUE #9 - pass macro + backtick leads to unescaped EOF error
#define PASS(a) a
#define NASTY PASS('`')

// Implements series of functions with following conventions
// int = -42
// uint = 42
// float/double = 1.5
// string = "hello"

struct TestData
{
    double a;
    int b;
    float c;
    char str[32];
};


class Base 
{
public:
    Base();
    virtual ~Base();
    inline int getVal() const { return this->val; }
protected:
    virtual void setVal(int val);
private:
    int val;
};


class Derived : public Base
{
public:
    virtual ~Derived();
    void set(int val);
    virtual void virtualSet(int val);
protected:
    virtual void setVal(int val) override;
private:
    float a;
};


// Extra struct with memset in body (test if non-virtual struct has correct 'this' pointer)

struct TestStructThis {
public:
    int a;
    float b;
    char c;
    inline void reset() { a = 42; b = 1.5; c = 'a'; }
    inline void zero() { memset(this, 0, sizeof(TestStructThis)); }
};


//---- VERY BASIC FUNCTIONS

const char* getString();

void setFloat(float* val);

void setInt(int* val);

void setUInt(unsigned* val);

void setStruct(TestData* data);

//---- JUST BASIC

void compareNestedPtr(TestData** p);

void nestedConstPtr(TestData const** p);


//---- MULTIPLE PARAMS 

void setIntFloat(int* x, float* y);


//---- REF/BY-VAL

void modByRef(TestData& data);

void passByVal(TestData data);