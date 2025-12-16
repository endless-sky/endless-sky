/* StringInterner.h
Copyright (c) 2017-2022 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include <string>



// This class stores a set of interned strings. Interning can be a slow operation during string creation/interning, but
// it will allow fast char-pointer based comparisons when comparing two interned strings (because interning ensures that
// each interned string only appears once in the set). Full string compares will still be needed when comparing interned
// strings to non-interned strings.
class StringInterner {
public:
	static const char *Intern(const char *key);
	static const char *Intern(const std::string key);
};
