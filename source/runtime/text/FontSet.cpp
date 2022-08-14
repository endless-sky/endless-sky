/* FontSet.cpp
Copyright (c) 2014-2020 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
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
	if(!fonts.count(size))
		fonts[size].Load(path);
}



const Font &FontSet::Get(int size)
{
	return fonts[size];
}
