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
int64_t ConditionsStore::GetCondition(const std::string &name) const
{
	ConditionsProvider* cp = GetProvider(name);
	if(cp != nullptr)
		return cp->GetCondition(name);
	
	auto it = conditions.find(name);
	if(it != conditions.end())
		return it->second;
	
	// Return the default value if nothing was found.
	return 0;
}



bool ConditionsStore::HasCondition(const std::string &name) const
{
	ConditionsProvider* cp = GetProvider(name);
	if(cp != nullptr)
		return cp->HasCondition(name);
	
	auto it = conditions.find(name);
	return it != conditions.end();
}



// Add a value to a condition. Returns true on success, false on failure.
bool ConditionsStore::AddCondition(const string &name, int64_t value)
{
	// This code performes 2 lookups of the condition, once for get and
	// once for set. This might be optimized to a single lookup in a
	// later version of the code when we add derived conditions.
	
	return SetCondition(name, GetCondition(name) + value);
}



// Set a value for a condition, first by trying the children and if
// that doesn't succeed then internally in the store.
bool ConditionsStore::SetCondition(const string &name, int64_t value)
{
	ConditionsProvider* cp = GetProvider(name);
	if(cp != nullptr)
		return cp->SetCondition(name, value);
	
	conditions[name] = value;
	return true;
}



// Erase a condition completely, first by trying the children and if
// that doesn't succeed then internally in the store.
bool ConditionsStore::EraseCondition(const string &name)
{
	// We go through both derived conditions as well as the primary
	// conditions (and only return true when all erases were succesfull).
	bool success = true;
	ConditionsProvider* cp = GetProvider(name);
	if(cp != nullptr)
		success = success && cp->EraseCondition(name);
	
	auto it = conditions.find(name);
	if(it != conditions.end())
		conditions.erase(it);
	
	// The condition was either erased at this point, or it was not present
	// when we started. In either case the condition got succesfully removed.
	return success;
}



// Direct (read-only) access to the stored primary conditions.
const map<string, int64_t> &ConditionsStore::GetPrimaryConditions() const
{
	return conditions;
}



// Sets a provider for a certain prefix. Calling this function with a
// nullpointer will unset the provider for the prefix.
void ConditionsStore::SetProviderPrefixed(const string &prefix, ConditionsProvider *child)
{
	if(child != nullptr)
		providers[prefix] = make_pair(false, child);
	else
		providers.erase(prefix);
}



// Sets a provider for a certain prefix. Calling this function with a
// nullpointer will unset the provider for the prefix.
void ConditionsStore::SetProviderNamed(const string &name, ConditionsProvider *child)
{
	if(child != nullptr)
		providers[name] = make_pair(true, child);
	else
		providers.erase(name);
}



ConditionsProvider* ConditionsStore::GetProvider(const string &name) const
{
	// Perform a single search for named and prefixed providers.
	if(providers.empty())
		return nullptr;
	auto it = providers.upper_bound(name);
	if(it == providers.begin())
		return nullptr;
	--it;
	// it->second.first is true if the string needs to match exactly. The strings
	// match exactly if they are the same over the length of the first string and
	// have the same length.
	if(!(name.compare(0, it->first.length(), it->first)) && (!it->second.first || it->first.length() == name.length()))
		return it->second.second;
	return nullptr;
}
