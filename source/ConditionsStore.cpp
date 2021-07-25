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



// Constructor where a number of initial manually-set values are set.
ConditionsStore::ConditionsStore(initializer_list<pair<string, int64_t>> initialConditions)
{
	for(const auto &it : initialConditions)
		SetCondition(it.first, it.second);
}



// Constructor where a number of initial manually-set values are set.
ConditionsStore::ConditionsStore(const map<string, int64_t> &initialConditions)
{
	for(const auto &it : initialConditions)
		SetCondition(it.first, it.second);
}



// Get a condition from the Conditions-Store. Retrieves both conditions
// that were directly set (primary conditions) as well as conditions
//derived from other data-structures (derived conditions).
int64_t ConditionsStore::GetCondition(const std::string &name) const
{
	const ConditionEntry *ce = GetEntryConst(name);
	if(!ce)
		return 0;
	
	if(ce->type == VALUE)
		return ce->value;
	
	return (ce->provider).getFun(name);
}



bool ConditionsStore::HasCondition(const std::string &name) const
{
	const ConditionEntry *ce = GetEntryConst(name);
	if(!ce)
		return false;
	
	if(ce->type == VALUE)
		return true;
	
	return (ce->provider).hasFun(name);
}



// Add a value to a condition. Returns true on success, false on failure.
bool ConditionsStore::AddCondition(const string &name, int64_t value)
{
	// This code performes 2 lookups of the condition, once for get and
	// once for set. This might be optimized to a single lookup in a
	// later version of the code when we add derived conditions.
	
	return SetCondition(name, GetCondition(name) + value);
}



// Set a value for a condition, either for the local value, or by performing
// a set on the provider.
bool ConditionsStore::SetCondition(const string &name, int64_t value)
{
	ConditionEntry *ce = GetEntry(name);
	if(!ce)
	{
		(storage[name]).value = value;
		return true;
	}
	
	if(ce->type == VALUE)
	{
		ce->value = value;
		return true;
	}
	
	return (ce->provider).setFun(name, value);
}



// Erase a condition completely, either the local value or by performing
// an erase on the provider.
bool ConditionsStore::EraseCondition(const string &name)
{
	ConditionEntry *ce = GetEntry(name);
	if(!ce)
		return true;
	
	if(ce->type == VALUE)
	{
		storage.erase(name);
		return true;
	}
	
	return (ce->provider).eraseFun(name);
}



// Retrieve a copy of the conditions stored by value. Direct access is not
// possible because this class uses a single central storage for value and
// providers internally.
const map<string, int64_t> ConditionsStore::GetPrimaryConditions() const
{
	map<string, int64_t> priConditions;
	for(const auto &it : storage)
		if(it.second.type == StorageType::VALUE)
			priConditions[it.first] = it.second.value;
	
	return priConditions;
}



// Sets a provider for a given prefix.
void ConditionsStore::SetProviderPrefixed(const string &prefix, DerivedProvider conditionsProvider)
{
	storage[prefix].provider = conditionsProvider;
	storage[prefix].type = PREFIX_PROVIDER;
}



// Sets a provider for the condition identified by the given name.
void ConditionsStore::SetProviderNamed(const string &name, DerivedProvider conditionProvider)
{
	storage[name].provider = conditionProvider;
	storage[name].type = EXACT_PROVIDER;
}



// Helper to completely remove all data and linked condition-providers from the store.
void ConditionsStore::Clear()
{
	storage.clear();
}



ConditionsStore::ConditionEntry *ConditionsStore::GetEntry(const string &name)
{
	if(storage.empty())
		return nullptr;
	
	// Perform a single search for values, named providers and prefixed providers.
	auto it = storage.upper_bound(name);
	if(it == storage.begin())
		return nullptr;
	
	--it;
	// The entry is valid if we have an exact stringmatch, but also when we have a
	// prefix entry and the prefix part matches.
	if(!(name.compare(0, it->first.length(), it->first)) &&
			(it->second.type == PREFIX_PROVIDER || it->first.length() == name.length()))
		return &(it->second);
	
	return nullptr;
}



const ConditionsStore::ConditionEntry *ConditionsStore::GetEntryConst(const string &name) const
{
	if(storage.empty())
		return nullptr;
	
	// Perform a single search for values, named providers and prefixed providers.
	auto it = storage.upper_bound(name);
	if(it == storage.begin())
		return nullptr;
	
	--it;
	// The entry is valid if we have an exact stringmatch, but also when we have a
	// prefix entry and the prefix part matches.
	if(!(name.compare(0, it->first.length(), it->first)) &&
			(it->second.type == PREFIX_PROVIDER || it->first.length() == name.length()))
		return &(it->second);
	
	return nullptr;
}
