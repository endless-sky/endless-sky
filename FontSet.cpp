/* FontSet.cpp
Michael Zahniser, 24 Oct 2013

Function definitions for the FontSet class.
*/

#include "FontSet.h"

#include "Font.h"

#include <map>

using namespace std;

namespace {
	map<int, Font> fonts;
}



void FontSet::Add(const string &path, int size)
{
	if(fonts.find(size) == fonts.end())
		fonts[size].Load(path);
}



const Font &FontSet::Get(int size)
{
	return fonts[size];
}
