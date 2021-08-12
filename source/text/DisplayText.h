/* DisplayText.h
Copyright (c) 2020 by OOTA, Masato

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef ES_TEXT_DISPLAYTEXT_H_
#define ES_TEXT_DISPLAYTEXT_H_

#include "layout.hpp"

#include <string>


// Class for holding a displayed text and layout.
class DisplayText {
public:
	DisplayText() = default;
	DisplayText(const char *text, Layout layout);
	DisplayText(const std::string &text, Layout layout);
	
	const std::string &GetText() const noexcept;
	const Layout &GetLayout() const noexcept;
	
	
private:
	Layout layout;
	std::string text;
};



#endif
