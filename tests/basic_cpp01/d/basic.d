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
	assert(strcmp(getString(), "hello") == 0);

	float x;
	setFloat(&x);
	assert(x == 1.5f);

	int i;
	setInt(&i);
	assert(i == -42);

	uint u;
	setUInt(&u);
	assert(u == 42);

	TestData data;
	setStruct(&data);
	assert(data.a == 1.5);
	assert(data.b == -42);
	assert(data.c == 1.5f);
	assert(strcmp(cast(char*)&data.str[0], "hello") == 0);

	// this isn't the safest way but for test case should work
	TestData*[] holder = [new TestData(1.5, -42, 1.5, (cast(byte*)(toStringz("hello")))[0..32])];
	compareNestedPtr(holder.ptr);
	const(TestData)*[] cholder = cast(const(TestData)*[])holder;
	nestedConstPtr(cholder.ptr);

	x = 0; i = 0;
	setIntFloat(&i, &x);
	assert(i == -42);
	assert(x == 1.5f);

	TestData dataRef;
	modByRef(dataRef);
	assert(dataRef.a == 1.5);
	assert(dataRef.b == -42);
	assert(dataRef.c == 1.5f);
	assert(strcmp(cast(char*)&dataRef.str[0], "hello") == 0);

	passByVal(data);

	assert(ofun() == 1.5f);
	assert(ifun() == 1.5);
}


unittest
{
	auto b = new Base();
	assert(b);
	assert(b.getVal() == -42);
	destroy(b);
	assert(b.getVal() == 0);

	auto d = new Derived();
	assert(d);
	assert(d.getVal() == -42);
	d.virtualSet(-42);
	assert(d.getVal() == -42*2);
	d.set(8);
	assert(d.getVal() == 8*2);
	destroy(d);
	assert(d.getVal() == 0);
}


unittest
{
	auto ns = new Inner();
	ns.set(42);
	assert(ns.getVal() == -42);
	destroy(ns);
}