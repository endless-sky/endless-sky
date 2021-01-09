/* FontUtilities.cpp
Copyright (c) 2021 by OOTA, Masato

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "FontUtilities.h"

#include <array>
#include <utility>

using namespace std;

namespace {
	const array<pair<char, string>, 3> charToEscape = { make_pair('<', "lt;"),
		make_pair('>', "gt;"), make_pair('&', "amp;") };
}



namespace FontUtilities {
	string Escape(const string &rawText)
	{
		string escapedText;
		escapedText.reserve(rawText.length());
		for(char c : rawText)
			{
				escapedText += c;
				for(const auto &escape : charToEscape)
					if(c == escape.first)
					{
						escapedText.back() = '&';
						escapedText += escape.second;
						break;
					}
			}
		return escapedText;
	}
	
	
	
	string Unescape(const string &escapedText)
	{
		const size_t length = escapedText.length();
		string rawText;
		rawText.reserve(length);
		for(size_t i = 0; i < length; ++i)
		{
			const char c = escapedText[i];
			rawText += c;
			if(c == '&')
				for(const auto &escape : charToEscape)
					if(escapedText.compare(i + 1, escape.second.length(), escape.second) == 0)
					{
						rawText.pop_back();
						rawText += escape.first;
						i += escape.second.length();
						break;
					}
		}
		return rawText;
	}
}
