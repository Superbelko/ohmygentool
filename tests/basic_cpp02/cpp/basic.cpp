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

int Derived::getVal()
{
    return val;
}