#pragma once

// Implements series of functions with following conventions
// int = -42
// uint = 42
// float/double = 1.5
// string = "hello"

template<typename T>
void setVal(T* val);


template<typename T>
struct Arr2
{
    T a,b;
};

template<typename T>
struct Arr3
{
    T a,b,c;
    // truncate our "list"
    operator Arr2() { return Arr2(a,b); }
};


template<typename T>
struct Vec2
{
    T x;
    T y;

    float lenSquared();
    Vec2 operator- ();
    Vec2 operator+ (const Vec2& other);
    T operator[](int i);
};


struct Test
{
    // writes operator kind and symbolic name after each invocation
    char opKind[4];
    bool isBinary;

    // series of "do nothing" operators
    Test operator~();
    bool operator!();
    Test& operator--();
    Test& operator++();
    Test operator--(int);
    Test operator++(int);
    Test operator-(const Test&);
    Test operator+(const Test&);
    Test operator*(const Test&);
    Test operator/(const Test&);
    Test operator&(const Test&);
    Test operator|(const Test&);
    int operator<(const Test&);
    int operator>(const Test&);
    Test operator+=(const Test&);
    Test operator-=(const Test&);
    Test operator*=(const Test&);
    Test operator/=(const Test&);
    Test operator&=(const Test&);
    Test operator|=(const Test&);
    Test operator&&(const Test&);
    Test operator||(const Test&);
    Test operator<<=(const Test&);
    Test operator>>=(const Test&);
    Test operator()();
};
