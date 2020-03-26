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