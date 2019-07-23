/* Format.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef FORMAT_H_
#define FORMAT_H_

#include <map>
#include <string>
#include <vector>



// Collection of functions for formatting strings for display.
class Format {
public:
	// Convert the given number into abbreviated format with a suffix like
	// "M" for million, "B" for billion, or "T" for trillion. Any number
	// above 1 quadrillion is instead shown in scientific notation.
	static std::string Credits(int64_t value);
	// Convert the given number to a string, with at most one decimal place.
	// This is primarily for displaying ship and outfit attributes.
	static std::string Number(double value);
	// Format the given value as a number with exactly the given number of
	// decimal places (even if they are all 0).
	static std::string Decimal(double value, int places);
	// Convert a string into a number. As with the output of Number(), the
	// string can have suffixes like "M", "B", etc.
	static double Parse(const std::string &str);
	// Replace a set of "keys," which must be strings in the form "<name>", with
	// a new set of strings, and return the result.
	static std::string Replace(const std::string &source, const std::map<std::string, std::string> keys);
	
	// Convert a string to title caps or to lower case.
	static std::string Capitalize(const std::string &str);
	static std::string LowerCase(const std::string &str);
	
	// Split a single string into substrings with the given separator.
	static std::vector<std::string> Split(const std::string &str, const std::string &separator);
};



#endif
