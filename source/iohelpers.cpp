#include "iohelpers.h"


OutStreamHelper& operator<< (OutStreamHelper& s, std::ostream& (*manip)(std::ostream &))
{
	for(std::ofstream* out: s.writers)
	{
		manip(*out);
	}
	
    // add wchar as well?
	if (manip == &std::endl<char, std::char_traits<char>>)
		s.isNewline = true;
	return s;
}
