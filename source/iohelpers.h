#pragma once

#include <ostream>
#include <fstream>
#include <string>
#include <vector>


struct OutStreamHelper
{
	OutStreamHelper(std::ofstream* a = nullptr, std::ofstream* b = nullptr)
	{
		if (a)
		writers.push_back(a);
		if (b)
		writers.push_back(b);
	}

	OutStreamHelper& indent(size_t i)
	{
		_indent = i;
		return *this;
	}

    OutStreamHelper& imore(size_t i)
	{
		_indent += i;
		return *this;
	}
	OutStreamHelper& iless(size_t i)
	{
		_indent -= i;
		if (_indent == (size_t)-1)
			_indent = 0;
		return *this;
	}

	size_t _indent = 0;
	bool isNewline = true;
	std::vector<std::ofstream*> writers;
};

/*
struct IndentHelper
{
	size_t width;
};

auto indent(size_t width)
{
	IndentHelper i = {width};
	return i;
}


OutStreamHelper& operator<< (OutStreamHelper& s, const IndentHelper& indent)
{
	for(std::ofstream* out: s.writers)
		for(size_t i = 0; i < indent.width; i++)
			*out << ' ';
	return s;
}
*/

template <typename T>
OutStreamHelper& operator<< (OutStreamHelper& s, const T& t)
{
	for(std::ofstream* out: s.writers)
	{
		if (s.isNewline)
			for(size_t i = 0; i < s._indent; i++) { *out << ' '; }
		*out << t;
	}
	s.isNewline = false;
	return s;
}

//also std::_Smanip<std::streamsize> for more manipulators?
OutStreamHelper& operator<< (OutStreamHelper& s, std::ostream& (*manip)(std::ostream &));


struct IndentBlock
{
	OutStreamHelper* _writer;
	int _indent;
	IndentBlock(OutStreamHelper& writer, int indent) 
	{
		_writer = &writer;
		_indent = indent;
		if (_indent > 0)
		writer.imore(_indent);
		else
		writer.iless(_indent);
	}
	~IndentBlock()
	{
		if (_indent > 0)
		_writer->iless(_indent);
		else 
		_writer->imore(_indent);
		_writer = nullptr;
	}
};