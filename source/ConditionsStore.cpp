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



int64_t ConditionsStore::operator [] (const std::string &name) const
{
	return GetCondition(name);
}



bool ConditionsStore::HasCondition(const std::string &name) const
{
	ConditionsProvider* cp = GetRegisteredChild(name);
	if(cp != nullptr)
		return cp->HasCondition(name);
	
	// No other table matches, lets search in the internal storage.
	auto it = conditions.find(name);
	return it != conditions.end();
}



// Get a value for a condition, first by trying the children and if
// that doesn't succeed then internally in the store
int64_t ConditionsStore::GetCondition(const string &name) const
{
	ConditionsProvider* cp = GetRegisteredChild(name);
	if(cp != nullptr)
		return cp->GetCondition(name);
	
	// No other table matches, lets search in the internal storage.
	auto it = conditions.find(name);
	if(it != conditions.end())
		return it->second;
	
	// Return the default value if nothing was found.
	return 0;
}



// Set a value for a condition, first by trying the children and if
// that doesn't succeed then internally in the store.
bool ConditionsStore::SetCondition(const string &name, int64_t value)
{
	ConditionsProvider* cp = GetRegisteredChild(name);
	if(cp != nullptr)
		return cp->SetCondition(name, value);
	
	// No other table matches, lets store it in internal storage.
	conditions[name] = value;
	return true;
}



// Erase a condition completely, first by trying the children and if
// that doesn't succeed then internally in the store.
bool ConditionsStore::EraseCondition(const string &name)
{
	ConditionsProvider* cp = GetRegisteredChild(name);
	if(cp != nullptr)
		return cp->EraseCondition(name);
	
	auto it = conditions.find(name);
	if(it != conditions.end())
		conditions.erase(it);
	
	// The condition was either erased at this point, or it was not present
	// when we started. In either case the condition got succesfully removed.
	return true;
}



// Read-only access to the local (non-child) conditions of this store.
const map<string, int64_t> &ConditionsStore::Locals() const
{
	return conditions;
}



void ConditionsStore::RegisterChild(ConditionsProvider &child, const vector<string> &matchPrefixes, const vector<string> &matchExacts)
{
	// Store the pointers to the children to forward to.
	for(auto &matchPrefix: matchPrefixes)
		this->matchPrefixes[matchPrefix] = &child;
	for(auto &matchExact: matchExacts)
		this->matchExacts[matchExact] = &child;
}



void ConditionsStore::DeRegisterChild(ConditionsProvider &child)
{
	ConditionsProvider *childPtr = &child;
	// Remove the child from all matchlist entries where it was listed.
	for(auto it = matchPrefixes.begin(); it != matchPrefixes.end(); it++)
		if(it->second == childPtr)
			it = matchPrefixes.erase(it);
	
	for(auto it = matchExacts.begin(); it != matchExacts.end(); it++)
		if(it->second == childPtr)
			it = matchExacts.erase(it);
}



ConditionsProvider* ConditionsStore::GetRegisteredChild(const std::string &name) const
{
	// First check if we should set based on exact names.
	auto exIt = matchExacts.find(name);
	if(exIt != matchExacts.end())
		return exIt->second;
	
	// Then check if this is a known prefix.
	// The lower_bound function will typically end up beyond the required
	// entry, because the entries are prefixes, but we can still arrive on
	// the exact location in case of an exact match.
	if(matchPrefixes.empty())
		return nullptr;
	
	auto preIt = matchPrefixes.lower_bound(name);
	if(preIt == matchPrefixes.end())
		--preIt;
	else if(preIt->first.compare(0, preIt->first.length(), name))
		return preIt->second;
	else if(preIt == matchPrefixes.begin())
		return nullptr;
	else
		--preIt;
	
	// We should have a match for preIt at this point if there were any
	// prefix matches to be made.
	if(preIt->first.compare(0, preIt->first.length(), name))
		return preIt->second;
	
	return nullptr;
}
