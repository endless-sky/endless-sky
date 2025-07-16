/* ConditionsStore.cpp
Copyright (c) 2020-2025 by Peter van der Meer

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "ConditionsStore.h"

#include "ConditionEntry.h"
#include "DataNode.h"
#include "DataWriter.h"
#include "Logger.h"

#include <utility>

using namespace std;



// Constructor with loading primary conditions from datanode.
ConditionsStore::ConditionsStore(const DataNode &node)
{
	Load(node);
}



// Constructor where a number of initial manually-set values are set.
ConditionsStore::ConditionsStore(initializer_list<pair<string, int64_t>> initialConditions)
{
	for(const auto &it : initialConditions)
		Set(it.first, it.second);
}



// Constructor where a number of initial manually-set values are set.
ConditionsStore::ConditionsStore(const map<string, int64_t> &initialConditions)
{
	for(const auto &it : initialConditions)
		Set(it.first, it.second);
}



void ConditionsStore::Load(const DataNode &node)
{
	for(const DataNode &child : node)
	{
		const string &key = child.Token(0);
		if(!DataNode::IsConditionName(key))
			child.PrintTrace("Invalid condition during savegame-load:");
		Set(key, (child.Size() >= 2) ? child.Value(1) : 1);
	}
}



void ConditionsStore::Save(DataWriter &out) const
{
	out.Write("conditions");
	out.BeginChild();
	for(const auto &it : storage)
	{
		// We don't need to save derived conditions that have a provider.
		if(it.second.getFunction || it.second.providingEntry)
			continue;
		// If the condition's value is 0, don't write it at all.
		if(!it.second.value)
			continue;
		// If the condition's value is 1, don't bother writing the 1.
		if(it.second.value == 1)
			out.Write(it.first);
		else
			out.Write(it.first, it.second.value);
	}
	out.EndChild();
}



// Get a condition from the Conditions-Store. Retrieves both conditions
// that were directly set (primary conditions) as well as conditions
// derived from other data-structures (derived conditions).
int64_t ConditionsStore::Get(const string &name) const
{
	// Look for a relevant entry, either the exact entry, or a prefixed provider.
	const ConditionEntry *ce = GetEntry(name);

	// If no entry is found, then we simply don't have any relevant data.
	if(!ce)
		return 0;

	// If the name matches exactly, then access directly with explicit conversion to int64_t.
	if(ce->name == name)
		return *ce;

	// If the name doesn't match exactly, then we are dealing with a prefixed provider that doesn't have an exactly
	// matching entry. Get is const, so isn't supposed to add such an entry; use a temporary object for access.
	ConditionEntry ceAccessor(name);
	ceAccessor.providingEntry = ce;
	return ceAccessor;
}



void ConditionsStore::Add(const string &name, int64_t value)
{
	(*this)[name] += value;
}



void ConditionsStore::Set(const string &name, int64_t value)
{
	(*this)[name] = value;
}



ConditionEntry &ConditionsStore::operator[](const string &name)
{
	// Search for an exact match and return it if it exists.
	auto it = storage.find(name);
	if(it != storage.end())
		return it->second;

	// Check for a prefix provider.
	ConditionEntry *ceprov = GetEntry(name);

	// Create the entry (name is used as key, and as ConditionEntry constructor argument.
	auto emp = storage.emplace(make_pair(name, name));
	it = emp.first;

	// If a relevant prefix provider is found, then provision this entry with the provider.
	if(ceprov != nullptr)
		it->second.providingEntry = ceprov;

	// Return the entry created.
	return it->second;
}



int64_t ConditionsStore::PrimariesSize() const
{
	int64_t result = 0;
	for(const auto &it : storage)
	{
		// We only count primary conditions; conditions that don't have a provider.
		if(it.second.providingEntry || it.second.getFunction)
			continue;
		++result;
	}
	return result;
}



ConditionEntry *ConditionsStore::GetEntry(const string &name)
{
	// Avoid code-duplication between const and non-const function.
	return const_cast<ConditionEntry *>(const_cast<const ConditionsStore *>(this)->GetEntry(name));
}



const ConditionEntry *ConditionsStore::GetEntry(const string &name) const
{
	if(storage.empty())
		return nullptr;

	// Perform a single search for values, named providers, and prefixed providers.
	auto it = storage.upper_bound(name);
	if(it == storage.begin())
		return nullptr;

	--it;
	// The entry is matching if we have an exact string match.
	if(name == it->first)
		return &(it->second);

	// If we don't have an exact match, but we have a matching prefix-provider, then we return that one.
	const ConditionEntry *ceProv = it->second.providingEntry;
	if(ceProv && name.starts_with(ceProv->name))
		return ceProv;

	// And otherwise we don't have a match.
	return nullptr;
}
