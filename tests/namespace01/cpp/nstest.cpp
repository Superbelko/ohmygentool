#include <cstring>

#include "nstest.h"

// error function to let the caller know about failure
extern "C" 
void assertFail(const char* msg);


void testAssert(bool expr, const char* msg)
{
    if (!expr)
        assertFail(msg);
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


namespace outer
{
    float ofun() {return 1.5f;}

    namespace inner
    {
        double ifun() {return 1.5;}

        void Inner::set(int val)
        {
            Inner::setVal(val);
        }

        void Inner::setVal(int val)
        {
            Base::setVal(-val);
        }
    }
}