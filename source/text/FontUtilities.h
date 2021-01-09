/* FontUtilities.h
Copyright (c) 2021 by OOTA, Masato

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef ES_TEXT_FONTUTILITIES_H_
#define ES_TEXT_FONTUTILITIES_H_

#include <string>

namespace FontUtilities {
	// Escape/Unescape special characters of Pango markups.
	// Any texts from a player should be drawn after escaping all special characters, '&', '<', and '>'.
	std::string Escape(const std::string &rawText);
	std::string Unescape(const std::string &escapedText);
}

#endif
