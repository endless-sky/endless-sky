/* Text.cpp
Copyright (c) 2023 by Rian Shelley

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "Text.h"

using namespace std;



// Format string
Text Text::Format(const std::string& format)
{
	// TODO: po file lookup
	return Text(format);
}



// Format string based on format and args
Text Text::Format(const std::string& format, const Args& args)
{
	string result;
	result.reserve(format.length());

	size_t start = 0;
	size_t search = start;
	while(search < format.length())
	{
		size_t left = format.find('<', search);
		if(left == string::npos)
			break;

		size_t right = format.find('>', left);
		if(right == string::npos)
			break;

		bool matched = false;
		++right;
		size_t length = right - left;
		for(const auto &it : args)
			if(!format.compare(left + 1, length - 2, it.first))
			{
				result.append(format, start, left - start);
				result.append(it.second.ToString());
				start = right;
				search = start;
				matched = true;
				break;
			}

		if(!matched)
			search = left + 1;
	}

	result.append(format, start, format.length() - start);
	return Text(result);
}



// Format either singular or plural form based on args.
Text Text::FormatN(const std::string& format_singular, const std::string& format_plural, const Args& args)
{
	// TODO: if a PO file is loaded, select among alternative plural forms using n here
	int64_t n = -1;
	for(auto& kv: args)
	{
		int64_t an = kv.second.N();
		if(an >= 0)
		{
			n = an;
			break;
		}
	}

	return Format(n == 1 ? format_singular : format_plural, args);
}
