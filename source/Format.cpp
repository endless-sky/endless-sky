/* Format.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "Format.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <sstream>

using namespace std;



string Format::Number(double value)
{
	if(!value)
		return "0";
	
	int power = floor(log10(fabs(value)));
	if(power >= 15 || power <= -5)
	{
		// Fall back to scientific notation if the number is outside the range
		// we can format "nicely".
		ostringstream out;
		out.precision(3);
		out << value;
		return out.str();
	}
	
	string result;
	result.reserve(8);
	
	bool isNegative = (value < 0.);
	bool nonzero = false;
	
	if(power >= 6)
	{
		nonzero = true;
		int place = (power - 6) / 3;
		
		static const char suffix[3] = {'M', 'B', 'T'};
		static const double multiplier[3] = {1e-6, 1e-9, 1e-12};
		result += suffix[place];
		value *= multiplier[place];
		power %= 3;
	}
	
	// The number of digits to the left of the decimal is max(0, power + 1).
	// e.g. if power = 0, 10 > value >= 1.
	int left = max(0, power + 1);
	int right = max(0, 5 - left);
	if(nonzero)
		right = min(right, 3);
	nonzero |= !right;
	int rounded = round(fabs(value) * pow(10., right));
	int delimiterIndex = -1;
	
	if (left > 3)
		delimiterIndex = left - 3;
	
	while(rounded | right)
	{
		int digit = rounded % 10;
		if(nonzero | digit)
		{
			result += digit + '0';
			nonzero = true;
		}
		rounded /= 10;
		
		if(right)
		{
			--right;
			if(!right)
			{
				if(nonzero)
					result += '.';
				nonzero = true;
			}
		}
		else
		{
			--left;
			if(left == delimiterIndex)
				result += ',';
		}
	}
	
	// Add the negative sign if needed.
	if(isNegative)
		result += '-';
	
	// Reverse the string.
	reverse(result.begin(), result.end());
	
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
