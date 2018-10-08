import core.stdc.string;
import std.string : toStringz, fromStringz;
import std.conv : to;

import generated;

extern(C) 
void assertFail(const char* msg)
{
	throw new Exception(fromStringz(msg).idup);
}


unittest
{
	assert(ofun() == 1.5f);
	assert(ifun() == 1.5);
}


unittest
{
	auto o = new Inner();
	assert(o.getVal() == -42);
	o.set(42);
	assert(o.getVal() == 42);
}