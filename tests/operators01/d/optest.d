import core.stdc.string;
import std.string : toStringz, fromStringz;
import std.conv : to;
import std.math : approxEqual = isClose;

import generated;

extern(C) 
void assertFail(const char* msg)
{
	throw new Exception(fromStringz(msg).idup);
}


unittest
{
	int a;
	setVal(&a);
	assert(a == -42);

	double b;
	setVal(&b);
	assert(b == 1.5);

	Vec2!int ivec;
	setVal(&ivec.x);
	setVal(&ivec.y);
	assert(ivec.x == ivec.y && ivec.x == -42);
	assert(approxEqual(ivec.lenSquared, 42*42f + 42*42f));

	auto fvec = Vec2!float(1f,1f);
	assert(approxEqual(fvec.lenSquared, 2f));
	assert(fvec[0] == 1f);
}


unittest
{
	Test a,b;
	~a;
	assert(a.isBinary == false && strcmp(cast(char*)&a.opKind[0], "~")==0);
	a.op_not(); // !a
	assert(a.isBinary == false && strcmp(cast(char*)&a.opKind[0], "!")==0);
	a--;
	assert(a.isBinary == false && strcmp(cast(char*)&a.opKind[0], "--")==0);
	a++;
	assert(a.isBinary == false && strcmp(cast(char*)&a.opKind[0], "++")==0);

	a+b;
	assert(a.isBinary == true && strcmp(cast(char*)&a.opKind[0], "+")==0);
	a-b;
	assert(a.isBinary == true && strcmp(cast(char*)&a.opKind[0], "-")==0);
	auto r1 = a*b;
	assert(a.isBinary == true && strcmp(cast(char*)&a.opKind[0], "*")==0);
	a/b;
	assert(a.isBinary == true && strcmp(cast(char*)&a.opKind[0], "/")==0);
	a|b;
	assert(a.isBinary == true && strcmp(cast(char*)&a.opKind[0], "|")==0);
	auto lt = a.op_lt(b); // a<b
	assert(a.isBinary == true && strcmp(cast(char*)&a.opKind[0], "<")==0);
	auto gt = a.op_gt(b); // a>b
	assert(a.isBinary == true && strcmp(cast(char*)&a.opKind[0], ">")==0);
	a+=b;
	assert(a.isBinary == true && strcmp(cast(char*)&a.opKind[0], "+=")==0);
	a-=b;
	assert(a.isBinary == true && strcmp(cast(char*)&a.opKind[0], "-=")==0);
	a*=b;
	assert(a.isBinary == true && strcmp(cast(char*)&a.opKind[0], "*=")==0);
	a/=b;
	assert(a.isBinary == true && strcmp(cast(char*)&a.opKind[0], "/=")==0);
	a&=b;
	assert(a.isBinary == true && strcmp(cast(char*)&a.opKind[0], "&=")==0);
	a|=b;
	assert(a.isBinary == true && strcmp(cast(char*)&a.opKind[0], "|=")==0);
	a.op_and(b); // a&&b
	assert(a.isBinary == true && strcmp(cast(char*)&a.opKind[0], "&&")==0);
	a.op_or(b); // a||b
	assert(a.isBinary == true && strcmp(cast(char*)&a.opKind[0], "||")==0);
	a<<=b;
	assert(a.isBinary == true && strcmp(cast(char*)&a.opKind[0], "<<=")==0);
	a>>=b;
	assert(a.isBinary == true && strcmp(cast(char*)&a.opKind[0], ">>=")==0);
}