#pragma once

// Implements series of functions with following conventions
// int = -42
// uint = 42
// float/double = 1.5
// string = "hello"


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


namespace outer
{
    float ofun();

    namespace inner
    {
        double ifun();

        class Inner : public Base
        {
        public:
            void set(int val);
        protected:
            virtual void setVal(int val) override;
        };
    }
}
