#include <cstring>

#include "optest.h"

// error function to let the caller know about failure
extern "C" 
void assertFail(const char* msg);


void testAssert(bool expr, const char* msg)
{
    if (!expr)
        assertFail(msg);
}


template<>
void setVal<double>(double* val)
{
    *val = 1.5;
}

template<>
void setVal<int>(int* val)
{
    *val = -42;
}


template<typename T>
float Vec2<T>::lenSquared()
{
    return x*x + y*y;
}

template<typename T>
T Vec2<T>::operator[](int i)
{
    return i == 0 ? x : y;
}


Test Test::operator~() { isBinary = false; strcpy(opKind, "~"); return *this; }
bool Test::operator!() { isBinary = false; strcpy(opKind, "!"); return true; }
Test& Test::operator--() { isBinary = false; strcpy(opKind, "--"); return *this; }
Test& Test::operator++() { isBinary = false; strcpy(opKind, "++"); return *this; }
Test Test::operator--(int) { isBinary = true; strcpy(opKind, "--"); return *this; }
Test Test::operator++(int) { isBinary = true; strcpy(opKind, "++"); return *this; }
Test Test::operator-(const Test&) { isBinary = true; strcpy(opKind, "-"); return Test(); }
Test Test::operator+(const Test&) { isBinary = true; strcpy(opKind, "+"); return Test(); }
Test Test::operator*(const Test&) { isBinary = true; strcpy(opKind, "*"); return Test(); }
Test Test::operator/(const Test&) { isBinary = true; strcpy(opKind, "/"); return Test(); }
Test Test::operator&(const Test&) { isBinary = true; strcpy(opKind, "&"); return Test(); }
Test Test::operator|(const Test&) { isBinary = true; strcpy(opKind, "|"); return Test(); }
int Test::operator<(const Test&) { isBinary = true; strcpy(opKind, "<"); return 0; }
int Test::operator>(const Test&) { isBinary = true; strcpy(opKind, ">"); return 0; }
Test Test::operator+=(const Test&) { isBinary = true; strcpy(opKind, "+="); return Test(); }
Test Test::operator-=(const Test&) { isBinary = true; strcpy(opKind, "-="); return Test(); }
Test Test::operator/=(const Test&) { isBinary = true; strcpy(opKind, "/="); return Test(); }
Test Test::operator*=(const Test&) { isBinary = true; strcpy(opKind, "*="); return Test(); }
Test Test::operator&=(const Test&) { isBinary = true; strcpy(opKind, "&="); return Test(); }
Test Test::operator|=(const Test&) { isBinary = true; strcpy(opKind, "|="); return Test(); }
Test Test::operator&&(const Test&) { isBinary = true; strcpy(opKind, "&&"); return Test(); }
Test Test::operator||(const Test&) { isBinary = true; strcpy(opKind, "||"); return Test(); }
Test Test::operator<<=(const Test&) { isBinary = true; strcpy(opKind, "<<="); return Test(); }
Test Test::operator>>=(const Test&) { isBinary = true; strcpy(opKind, ">>="); return Test(); }
Test Test::operator()() { isBinary = false; strcpy(opKind, "()"); return Test(); }


void bleh()
{
    Vec2<int> a;
    auto ilen = a.lenSquared();
    a[0];

    Vec2<float> b;
    auto flen = b.lenSquared();
    b[0];

    Vec2<double> c;
    auto dlen = c.lenSquared();
    c[0];
    
}


void meh()
{
    Test a, b;
    ~a;
    !a;
    ++a;
    --a;
    a++;
    a--;
    a-b;
    a+b;
    a*b;
    a/b;
    a&b;
    a|b;
    a<b;
    a>b;
    a+=b;
    a-=b;
    a*=b;
    a/=b;
    a&=b;
    a|=b;
    a&&b;
    a||b;
    a<<=b;
    a>>=b;
    a();
}