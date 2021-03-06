#pragma once

// ISSUE #4
inline int getIdx(int i)
{
    static const int idx[4] = { 0, 1, 0, -1 };
    return idx[i&0x03];
}

// the following 2 classes models non virtual inheritance, ensure proper ctor calls are issued
// additionally special case for method whose body implemented in header but have different param names
// this and added GeomStuff class tests that various forms of contructor initialization would be properly translated

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



// -------------------------------------------------
// Templated base class & inheritance (ISSUE #15)
// (constness stripped for simplicity, since we are dealing with classes IPtr takes references)

template <class T>
class IPtr {
public:
  int someInt() { return 1; }
  inline IPtr();
  inline IPtr (T ptr, bool addRef = true);
  inline IPtr (/*const*/ IPtr& other) { ptr = other.ptr; }

  inline T operator= (T ptr) { this->ptr = ptr; return ptr; }
//protected:
  T ptr; // T* ptr;
};

template<class T>
inline IPtr<T>::IPtr() : ptr(0) 
{
}

template<class T>
inline IPtr<T>::IPtr(T ptr, bool addRef) : ptr(ptr) 
{
}

class FUnknown
{
public:
    inline virtual int get() { return 123; }
    virtual ~FUnknown() = default;
};

template <class I>
class FUnknownPtr : public IPtr<I> {
public:
  inline FUnknownPtr(FUnknown* unknown);
  inline FUnknownPtr(/*const*/ FUnknownPtr &p) : IPtr<I>(p) {} // segfault #15
  inline FUnknownPtr() {}

  inline FUnknownPtr &operator=(/*const*/ FUnknownPtr &p) {
    IPtr<I>::operator=(p);
    return *this;
  }
  inline I operator=(FUnknown *unknown);
  inline I getInterface() { return this->ptr; }
};

template <class I>
inline FUnknownPtr<I>::FUnknownPtr (FUnknown* unknown)
{
    this->ptr = unknown;
}

template <class I>
inline I FUnknownPtr<I>::operator= (FUnknown* unknown)
{	
    return IPtr<I>::operator= (unknown);
    //return IPtr<I>::operator= (0);
}


class MyInt : public FUnknown
{
public:
    int val;
    virtual ~MyInt() {}
};

// -------------------------------------------------

// The following test confirms that GeomStuff ctor has proper syntax
// and Calc() compiles
struct Vec2
{
public:
    float x;
    float y;
    inline Vec2() {}
    inline Vec2(const Vec2& other) { x = other.x; y = other.y; }
    inline Vec2(float val) { x = val; y = val; }
    inline Vec2(float x, float y) : x(x), y(y) {}

    inline Vec2 operator+ (const Vec2& r) {
        return Vec2(x + r.x, y + r.y);
    }

    inline Vec2 operator* (float scale) {
        return Vec2(x * scale, y * scale);
    }
};

struct GeomStuff
{
    Vec2 a;
    Vec2 b;
    float h;
    GeomStuff(int) : a(0), b(1.0f, -1.0f), h(0) {}
    Vec2 Calc(const Vec2& smth) 
    {
        // random junk
        Vec2 c = b * 2.0f;
        return a + c;
    }

    Vec2 WithDefault(float a, Vec2 b = Vec2(0.f))
    {
        return b * a;
    }
};


struct Mat22
{
    Vec2 col0;
    Vec2 col1;

    Mat22()
    {
    }

    Mat22(const Vec2& column0, const Vec2& column1) : col0(column0), col1(column1)
    {
    }

    const Mat22 transposed() const
    {
        // Extra case where type got written twice 'v0 = Vec2(Vec2(col0.x, col0.y))'
        // ISSUE: constness loss
        const Vec2 v0(col0.x, col1.x);
        const Vec2 v1(col0.y, col1.y);

        return Mat22(v0, v1);
    }

    static Mat22 WithLocalDecl(const Vec2& t)
    {
        // In addition to above make sure this has correct ctor call
        // for now keep vectors as locals because ref-ness in D sucks
        //Mat22 s(    Vec2(t.x,-t.y),
        //            Vec2(-t.y,t.x));

        // this version is yet another case
        auto temp0 = Vec2(t.x,-t.y);
        auto temp1 = Vec2(-t.y,t.x);
        Mat22 s(temp0, temp1);

        return s;
    }
};

typedef struct Matrix3x3 {
    float vals[3][3];
} Matrix3x3;

static constexpr Matrix3x3 identity3 = {{
    { 1.0f, 0.0f, 0.0f },
    { 0.0f, 1.0f, 0.0f },
    { 0.0f, 0.0f, 1.0f },
}};


// regular inheritance, make sure it translates properly
class VBase
{
protected:
    inline VBase(void* concreteType, int baseFlags) : _concreteType(concreteType), _baseFlags(baseFlags) {}
    inline VBase(int baseFlags) : _baseFlags(baseFlags) {}
    virtual ~VBase() {}
    void* _concreteType;
    int _baseFlags;
};

class VOther : public VBase
{
protected:
    VOther(void* concreteType, int baseFlags) : VBase(concreteType, baseFlags) {}
    VOther(int baseFlags) : VBase(baseFlags) {}
    virtual ~VOther() {}
};

struct Material;
struct ExtraSmth;

class ComplexClass
{
    Vec2 p;
    Material* m;
    ExtraSmth* ex;

    // this should have injected const casts
    void copy(const ComplexClass* other)
    {
        p = other->p;
        m = other->m; 
        ex = other->ex;
    }

    // const cast needed in D
    Material* getMaterial() const 
    {
        return m;
    }
};

struct BitFields
{
    // normal 32 bit size
    int a1 : 16;
    int a2 : 8;
    int a3 : 4;
    int a4 : 4;

    int pad1;

    // normal 64 bit size
    int b1 : 16;
    int b2 : 16;
    int b3 : 8;
    int b4 : 8;
    int b5 : 4;
    int b6 : 4;
    int b7 : 4;
    int b8 : 4;

    int pad2;

    // long 64+ bits
    int c1 : 30;
    int c2 : 30;
    int c3 : 10;

    int pad3;
};

// generic thing
template<typename T>
struct Thing
{
    T prop;
};

// specialized thing, D version should have name for specialization
template<>
struct Thing<char>
{
    char prop;
};


// -------------------------
struct FilterType
{
	enum Enum
	{
		A,
		B,
        C,
		MAX = 8,
		UNDEFINED = MAX-1
	};
};

typedef int FilterObjectAttributes;

// enum arithmetics, requires int to enum casts
inline FilterType::Enum GetFilterType(FilterObjectAttributes attr)
{
	return FilterType::Enum(attr & (FilterType::MAX-1));
}
// -------------------------

// decltype test
template<typename T>
IPtr<T> test_decltype(T t)
{
    IPtr<decltype(t)> p;
    return p;
}