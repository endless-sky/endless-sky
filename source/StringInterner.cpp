/* StringInterner.cpp
Copyright (c) 2017-2021 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "StringInterner.h"

#include <cstring>
#include <mutex>
#include <set>
#include <string>

using namespace std;



// String interning: return a pointer to a character string that matches the
// given string but has static storage duration.
const char *StringInterner::Intern(const char *key)
{
	static set<string> interned;
	static mutex m;
	
	// Just in case this function is accessed from multiple threads:
	lock_guard<mutex> lock(m);
	return interned.insert(key).first->c_str();
}



const char *StringInterner::Intern(const string key)
{
	return Intern(key.c_str());
}
