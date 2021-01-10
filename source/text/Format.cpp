/* Format.cpp
Copyright (c) 2014-2020 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "Format.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <sstream>

using namespace std;

namespace {
	// Format an integer value, inserting its digits into the given string in
	// reverse order and then reversing the string.
	void FormatInteger(int64_t value, bool isNegative, string &result)
	{
		int places = 0;
		do {
			if(places && !(places % 3))
				result += ',';
			++places;
			
			result += static_cast<char>('0' + value % 10);
			value /= 10;
		} while(value);
		
		if(isNegative)
			result += '-';
		
		reverse(result.begin(), result.end());
	}
}



// Convert the given number into abbreviated format with a suffix like
// "M" for million, "B" for billion, or "T" for trillion. Any number
// above 1 quadrillion is instead shown in scientific notation.
string Format::Credits(int64_t value)
{
	bool isNegative = (value < 0);
	int64_t absolute = abs(value);
	
	// If the value is above one quadrillion, show it in scientific notation.
	if(absolute > 1000000000000000ll)
	{
		ostringstream out;
		out.precision(3);
		out << static_cast<double>(value);
		return out.str();
	}
	
	// Reserve enough space for something like "-123.456M".
	string result;
	result.reserve(8);
	
	// Handle numbers bigger than a million.
	static const vector<char> SUFFIX = {'T', 'B', 'M'};
	static const vector<int64_t> THRESHOLD = {1000000000000ll, 1000000000ll, 1000000ll};
	for(size_t i = 0; i < SUFFIX.size(); ++i)
		if(absolute > THRESHOLD[i])
		{
			result += SUFFIX[i];
			int decimals = (absolute / (THRESHOLD[i] / 1000)) % 1000;
			for(int d = 0; d < 3; ++d)
			{
				result += static_cast<char>('0' + decimals % 10);
				decimals /= 10;
			}
			result += '.';
			absolute /= THRESHOLD[i];
			break;
		}
	
	// Convert the number to a string, adding commas if needed.
	FormatInteger(absolute, isNegative, result);
	return result;
}



// Convert a time in seconds to years/days/hours/minutes/seconds
std::string Format::PlayTime(double timeVal)
{
	string result;
	int timeValFormat = 0;
	static const array<char, 5> SUFFIX = {'s', 'm', 'h', 'd', 'y'};
	static const array<int, 4> PERIOD = {60, 60, 24, 365};

	timeValFormat = max(0., timeVal);
	// Break time into larger and larger units until the largest one, or the value is empty
	size_t i = 0;
	do {
		int period = (i < SUFFIX.size() - 1 ? timeValFormat % PERIOD[i] : timeValFormat);
		result = (i == 0 ? result + SUFFIX[i] : result + ' ' + SUFFIX[i]);
		do {
			result += static_cast<char>('0' + period % 10);
			period /= 10;
		} while(period);
		if(i < PERIOD.size())
			timeValFormat /= PERIOD[i];
		i++;
	} while (timeValFormat && i < SUFFIX.size());

	reverse(result.begin(), result.end());
	return result;
}



// Convert the given number to a string, with at most one decimal place.
// This is primarily for displaying ship and outfit attributes.
string Format::Number(double value)
{
	if(!value)
		return "0";
	
	string result;
	bool isNegative = (value < 0.);
	value = fabs(value);
	
	// Check if this is a whole number.
	double decimal = modf(value, &value);
	if(decimal)
	{
		if(decimal >= .95)
		{
			result += '0';
			++value;
		}
		else
			result += static_cast<char>('0' + static_cast<int>(round(decimal * 10.)));
		
		result += '.';
	}
	
	// Convert the number to a string, adding commas if needed.
	FormatInteger(value, isNegative, result);
	return result;
}



// Format the given value as a number with exactly the given number of
// decimal places (even if they are all 0).
string Format::Decimal(double value, int places)
{
	double integer;
	double fraction = fabs(modf(value, &integer));
	
	string result = to_string(static_cast<int>(integer)) + ".";
	while(places--)
	{
		fraction = modf(fraction * 10., &integer);
		result += ('0' + static_cast<int>(integer));
	}
	return result;
}



// Convert a string into a number. As with the output of Number(), the
// string can have suffixes like "M", "B", etc.
double Format::Parse(const string &str)
{
	double place = 1.;
	double value = 0.;
	
	string::const_iterator it = str.begin();
	string::const_iterator end = str.end();
	while(it != end && (*it < '0' || *it > '9') && *it != '.')
		++it;
	
	for( ; it != end; ++it)
	{
		if(*it == '.')
			place = .1;
		else if(*it < '0' || *it > '9')
			break;
		else
		{
			double digit = *it - '0';
			if(place < 1.)
			{
				value += digit * place;
				place *= .1;
			}
			else
			{
				value *= 10.;
				value += digit;
			}
		}
	}
	
	if(it != end)
	{
		if(*it == 'k' || *it == 'K')
			value *= 1e3;
		else if(*it == 'm' || *it == 'M')
			value *= 1e6;
		else if(*it == 'b' || *it == 'B')
			value *= 1e9;
		else if(*it == 't' || *it == 'T')
			value *= 1e12;
	}
	
	return value;
}



string Format::Replace(const string &source, const map<string, string> keys)
{
	string result;
	result.reserve(source.length());
	
	size_t start = 0;
	size_t search = start;
	while(search < source.length())
	{
		size_t left = source.find('<', search);
		if(left == string::npos)
			break;
		
		size_t right = source.find('>', left);
		if(right == string::npos)
			break;
		
		bool matched = false;
		++right;
		size_t length = right - left;
		for(const auto &it : keys)
			if(!source.compare(left, length, it.first))
			{
				result.append(source, start, left - start);
				result.append(it.second);
				start = right;
				search = start;
				matched = true;
				break;
			}
		
		if(!matched)
			search = left + 1;
	}
	
	result.append(source, start, source.length() - start);
	return result;
}



void Format::ReplaceAll(string &text, const string &target, const string &replacement)
{
	// If the searched string is an empty string, do nothing.
	if(target.empty())
		return;
	
	string newString;
	newString.reserve(text.length());
	
	// Index at which to begin searching for the target string.
	size_t start = 0;
	size_t matchLength = target.length();
	// Index at which the target string was found.
	size_t findPos = string::npos;
	while((findPos = text.find(target, start)) != string::npos)
	{
		newString.append(text, start, findPos - start);
		newString += replacement;
		start = findPos + matchLength;
	}
	
	// Add the remaining text.
	newString += text.substr(start);
	
	text.swap(newString);
}



string Format::Capitalize(const string &str)
{
	string result = str;
	bool first = true;
	for(char &c : result)
	{
		if(!isalpha(c))
			first = true;
		else
		{
			if(first && islower(c))
				c = toupper(c);
			first = false;
		}
	}
	return result;
}



string Format::LowerCase(const string &str)
{
	string result = str;
	for(char &c : result)
		c = tolower(c);
	return result;
}



// Split a single string into substrings with the given separator.
vector<string> Format::Split(const string &str, const string &separator)
{
	vector<string> result;
	size_t begin = 0;
	while(true)
	{
		size_t pos = str.find(separator, begin);
		if(pos == string::npos)
			pos = str.length();
		result.emplace_back(str, begin, pos - begin);
		begin = pos + separator.size();
		if(begin >= str.length())
			break;
	}
	return result;
}
