/* FontSet.h
Michael Zahniser, 24 Oct 2013

Class for storing all the fonts that can be used.
*/

#ifndef FONT_SET_H_INCLUDED
#define FONT_SET_H_INCLUDED

#include <string>

class Font;



class FontSet {
public:
	static void Add(const std::string &path, int size);
	static const Font &Get(int size);
};



#endif
