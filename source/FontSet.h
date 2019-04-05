/* FontSet.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef FONT_SET_H_
#define FONT_SET_H_

#include <string>

class Font;



// Class for getting the Font object for a given point size.
class FontSet {
public:
	static void Add(const std::string &fontsDir);
	static const Font &Get(int size);
	
	// Set the font description. "desc" is a comma separated list of font families.
	static void SetFontDescription(const std::string &desc);
	// Set the reference font description which a line spacing is based on.
	static void SetLayoutReference(const std::string &desc);
	// Set the language for layouting.
	static void SetLanguage(const std::string &lang);
};



#endif
