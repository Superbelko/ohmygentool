#pragma once

// ISSUE #4
inline int getIdx(int i)
{
    static const int idx[4] = { 0, 1, 0, -1 };
    return idx[i&0x03];
}

// the following 2 classes models non virtual inheritance, ensure proper ctor calls are issued
// additionally special case for method whose body implemented in header but have different param names

class Base
{
public:
    int val;

    Base() : val(42) {}
    Base(int v) : val(v) {}

    // note that parameter name are different than that are used in definition
    inline void setVal(int newValue);
};

class Derived : public Base
{
public:
    Derived(int v) : Base(v) {}
    int getVal();

};


void Base::setVal(int value)
{
    val = value;
}


// same, but with templates and partial specialization

template <typename T, int dummy>
class TBase
{
public:
    T val;

    TBase(int v) : val(v) {}

    // again, different parameter name
    void setVal(T newValue);
    T getVal() { return val; }
};

template<typename T>
class Base0 : public TBase<T, 0>
{
public:
    typedef TBase<int, 0> CBase;
    Base0() : CBase(42) {}
};

template<typename T, int dummy>
void TBase<T, dummy>::setVal(T value)
{
    val = value;
}

// Anonymous struct using typedef
typedef struct {
    int a;
    float b;
    char c;
} TypedefAnonStruct;

// Named struct using typedef (different names)
typedef struct TypedefStruct {
    int a;
    float b;
    char c;
} TypedefStruct_;
