/* ConditionsProvider.cpp
Copyright (c) 2020 by Peter van der Meer

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "ConditionsProvider.h"

using namespace std;



// Get value for a condition. Calls the implementation as provided
// by the class implementing this interface.
int64_t ConditionsProvider::GetCondition(const std::string &name) const
{
	return GetConditionImpl(name);
}



// Check if a condition is present. Calls the implementation as provided
// by the class implementing this interface.
bool ConditionsProvider::HasCondition(const std::string &name) const
{
	return HasConditionImpl(name);
}



// Set a value for a condition. Calls the implementation as provided
// by the class implementing this interface. Implementing classes are
// expected to return false and change nothing for read-only conditions.
bool ConditionsProvider::SetCondition(const string &name, int64_t value)
{
	return SetConditionImpl(name, value);
}



// Erase a condition completely. Calls the implementation as provided
// by the class implementing this interface. Implementing classes are
// expected to return false and change nothing for read-only conditions.
bool ConditionsProvider::EraseCondition(const string &name)
{
	return EraseConditionImpl(name);
}
