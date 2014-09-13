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



class Format {
public:
	static std::string Number(double value);
	// Replace a set of "keys," which must be strings in the form "<name>", with
	// a new set of strings, and return the result.
	static std::string Replace(const std::string source, const std::map<std::string, std::string> keys);
};



#endif
