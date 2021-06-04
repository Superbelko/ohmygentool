import core.stdc.string;
import std.string : toStringz, fromStringz;
import std.conv : to;

import generated;

extern(C) 
void assertFail(const char* msg)
{
	throw new Exception(fromStringz(msg).idup);
}


// ISSUE #4
unittest
{
	assert(getIdx(0) == 0);
	assert(getIdx(1) == 1);
	assert(getIdx(2) == 0);
	assert(getIdx(3) == -1);
}

unittest
{
	Derived d;
	d._default_ctor();
	assert(d.getVal() == 42);

	d = Derived(100);
	assert(d.getVal() == 100);

	d.setVal(42);
	assert(d.getVal() == 42);
}

unittest
{
	Base0!int d;
	d._default_ctor();
	assert(d.getVal() == 42);

	d.setVal(-1);
	assert(d.getVal() == -1);
}

version(none)
unittest
{
	// This test was supposed to test template inheritance with tangled structure w/ opAssign, but...
	// contravariance issues, keep FUnknown instead of MyInt for now (this kind of destroys the purpose of this test...)
	//auto myInt = FUnknownPtr!MyInt(new MyInt); // this(FUnknonw unknown) { ptr=unknown;}  Error: cannot implicitly convert expression `unknown` of type `generated.FUnknown` to `generated.MyInt`
	auto myInt = FUnknownPtr!FUnknown(new MyInt); 
	auto ptr = cast(MyInt)myInt.getInterface;
	ptr.val = 42;
	assert((cast(MyInt)myInt.getInterface).val == 42);
}

unittest
{
	import std.traits;
	static assert (BitFields.sizeof == getUDAs!(BitFields, CppClassSizeAttr)[0].size);

	static assert (is(ReturnType!(test_decltype!int) == IPtr!int));
}
