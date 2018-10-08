#include <cstring>

#include "basic.h"

// error function to let the caller know about failure
extern "C" 
void assertFail(const char* msg);


void testAssert(bool expr, const char* msg)
{
    if (!expr)
        assertFail(msg);
}


//---- VERY BASIC FUNCTIONS

const char* getString()
{
    return "hello";
}

void setFloat(float* val)
{
    *val = 1.5f;
}

void setInt(int* val)
{
    *val = -42;
}

void setUInt(unsigned* val)
{
    *val = 42;
}

void setStruct(TestData* data)
{
    data->a = 1.5;
    data->b = -42;
    data->c = 1.5f;
    strcpy(data->str, "hello");
}

void compareNestedPtr(TestData** p)
{
    const auto* data = *p;
    testAssert(data->a == 1.5, "value not equal 1.5");
    testAssert(data->b == -42, "value not equal -42");
    testAssert(data->c == 1.5f, "value not equal 1.5");
    testAssert(strcmp(data->str, "hello") == 0, "value not equal 'hello'");
}

void nestedConstPtr(TestData const** p)
{
    const auto* data = *p;
    testAssert(data->a == 1.5, "value not equal 1.5");
    testAssert(data->b == -42, "value not equal -42");
    testAssert(data->c == 1.5f, "value not equal 1.5");
    testAssert(strcmp(data->str, "hello") == 0, "value not equal 'hello'");
}


//---- MULTIPLE PARAMS 

void setIntFloat(int* x, float* y)
{
    *x = -42; *y = 1.5f;
}


//---- REF/BY-VAL

void modByRef(TestData& data)
{
    data.a = 1.5;
    data.b = -42;
    data.c = 1.5f;
    strcpy(data.str, "hello");
}

void passByVal(TestData data)
{
    testAssert(data.a == 1.5, "value not equal 1.5");
    testAssert(data.b == -42, "value not equal -42");
    testAssert(data.c == 1.5f, "value not equal 1.5");
    testAssert(strcmp(data.str, "hello") == 0, "value not equal 'hello'");
}





Base::Base() 
{
    val = -42;
}

Base::~Base()
{
    val = 0;
}

void Base::setVal(int val)
{
    this->val=val;
}

/*
Derived::~Derived()
{
    Base::setVal(-1);
}
*/



void Derived::setVal(int val)
{
    Base::setVal(val*2);
}

void Derived::set(int val)
{
    Derived::setVal(val);
}

void Derived::virtualSet(int val)
{
    Derived::setVal(val);
}