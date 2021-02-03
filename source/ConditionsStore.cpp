/* ConditionsStore.cpp
Copyright (c) 2020 by Peter van der Meer

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "ConditionsStore.h"

using namespace std;



// Empty constructor, just results in an emtpy store.
ConditionsStore::ConditionsStore()
{
}



// Constructor where a number of initial manually-set values are set.
ConditionsStore::ConditionsStore(initializer_list<pair<string, int64_t>> initialConditions)
{
	for(auto it : initialConditions)
		SetCondition(it.first, it.second);
}



// Constructor where a number of initial manually-set values are set.
ConditionsStore::ConditionsStore(const map<string, int64_t> initialConditions)
{
	for(auto it : initialConditions)
		SetCondition(it.first, it.second);
}



// Get a condition from the Conditions-Store. Retrieves both conditions
// that were directly set (primary conditions) as well as conditions
//derived from other data-structures (derived conditions).
int64_t ConditionsStore::operator[](const std::string &name) const
{
	// When we add support for derived conditions, then we can
	// lookup the derived condition here before searching the
	// primary conditions.
	
	auto it = conditions.find(name);
	if(it != conditions.end())
		return it->second;
	
	// Return the default value if nothing was found.
	return 0;
}



bool ConditionsStore::HasCondition(const std::string &name) const
{
	// When we add support for derived conditions, then we can
	// lookup the derived condition here before searching the
	// manually set conditions.
	
	auto it = conditions.find(name);
	return it != conditions.end();
}



// Add a value to a condition. Returns true on success, false on failure.
bool ConditionsStore::AddCondition(const string &name, int64_t value)
{
	// This code performes 2 lookups of the condition, once for get and
	// once for set. This might be optimized to a single lookup in a
	// later version of the code when we add derived conditions.
	
	return SetCondition(name, (*this)[name] + value);
}



// Set a value for a condition, first by trying the children and if
// that doesn't succeed then internally in the store.
bool ConditionsStore::SetCondition(const string &name, int64_t value)
{
	// When we add support for derived conditions, then we can
	// lookup if a derived condition matches before we set in
	// the map for primary conditions conditions.
	
	conditions[name] = value;
	return true;
}



// Erase a condition completely, first by trying the children and if
// that doesn't succeed then internally in the store.
bool ConditionsStore::EraseCondition(const string &name)
{
	// When we add support for derived conditions, then we should
	// go through both derived conditions as well as the primary
	// conditions (and only return true when all erases were
	// succesfull).
	
	auto it = conditions.find(name);
	if(it != conditions.end())
		conditions.erase(it);
	
	// The condition was either erased at this point, or it was not present
	// when we started. In either case the condition got succesfully removed.
	return true;
}



// Direct (read-only) access to the stored primary conditions.
const map<string, int64_t> &ConditionsStore::GetPrimaryConditions() const
{
	return conditions;
}
