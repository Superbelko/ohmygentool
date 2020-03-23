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