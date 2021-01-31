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



bool ConditionsProvider::HasCondition(const std::string &name) const
{
	return false;
}



// Add a value to a condition. Returns true on success, false on failure.
// Actual implementers of this interface can choose to override this method
// and provide a more efficient implementation.
bool ConditionsProvider::AddCondition(const string &name, int64_t value)
{
	return SetCondition(name, GetCondition(name) + value);
}



// Set a value for a condition. The default implementation returns false
// to indicate a read-only variable. Implementers of this interface can
// override if they support variable modifications.
bool ConditionsProvider::SetCondition(const string &name, int64_t value)
{
	return false;
}



// Erase a condition completely. The default implementation returns false
// to indicate a read-only variable. Implementers of this interface can
// override if they support variable modifications.
bool ConditionsProvider::EraseCondition(const string &name)
{
	return false;
}



void ConditionsProvider::RegisterChild(ConditionsProvider &child, const vector<string> &matchPrefixes, const vector<string> &matchExacts)
{
	//Not all providers use this mechanism, so default to an empty implementation.
	return;
}



void ConditionsProvider::DeRegisterChild(ConditionsProvider &child)
{
	//Not all providers use this mechanism, so default to an empty implementation.
	return;
}
