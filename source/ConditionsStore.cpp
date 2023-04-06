/* ConditionsStore.cpp
Copyright (c) 2020-2022 by Peter van der Meer

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

#include "DataNode.h"
#include "DataWriter.h"
#include "Logger.h"

#include <utility>

using namespace std;



// Default constructor
ConditionsStore::DerivedProvider::DerivedProvider(const string &name, bool isPrefixProvider)
	: name(name), isPrefixProvider(isPrefixProvider)
{
}



void ConditionsStore::DerivedProvider::SetGetFunction(function<int64_t(const string &)> newGetFun)
{
	getFunction = std::move(newGetFun);
}



void ConditionsStore::DerivedProvider::SetHasFunction(function<bool(const string &)> newHasFun)
{
	hasFunction = std::move(newHasFun);
}



void ConditionsStore::DerivedProvider::SetSetFunction(function<bool(const string &, int64_t)> newSetFun)
{
	setFunction = std::move(newSetFun);
}



void ConditionsStore::DerivedProvider::SetEraseFunction(function<bool(const string &)> newEraseFun)
{
	eraseFunction = std::move(newEraseFun);
}



ConditionsStore::ConditionEntry::operator int64_t() const
{
	if(!provider)
		return value;

	const string &key = fullKey.empty() ? provider->name : fullKey;
	return provider->getFunction(key);
}



ConditionsStore::ConditionEntry &ConditionsStore::ConditionEntry::operator=(int64_t val)
{
	if(!provider)
		value = val;
	else
	{
		const string &key = fullKey.empty() ? provider->name : fullKey;
		provider->setFunction(key, val);
	}
	return *this;
}



ConditionsStore::ConditionEntry &ConditionsStore::ConditionEntry::operator++()
{
	if(!provider)
		++value;
	else
	{
		const string &key = fullKey.empty() ? provider->name : fullKey;
		provider->setFunction(key, provider->getFunction(key) + 1);
	}
	return *this;
}



ConditionsStore::ConditionEntry &ConditionsStore::ConditionEntry::operator--()
{
	if(!provider)
		--value;
	else
	{
		const string &key = fullKey.empty() ? provider->name : fullKey;
		provider->setFunction(key, provider->getFunction(key) - 1);
	}
	return *this;
}



ConditionsStore::ConditionEntry &ConditionsStore::ConditionEntry::operator+=(int64_t val)
{
	if(!provider)
		value += val;
	else
	{
		const string &key = fullKey.empty() ? provider->name : fullKey;
		provider->setFunction(key, provider->getFunction(key) + val);
	}
	return *this;
}



ConditionsStore::ConditionEntry &ConditionsStore::ConditionEntry::operator-=(int64_t val)
{
	if(!provider)
		value -= val;
	else
	{
		const string &key = fullKey.empty() ? provider->name : fullKey;
		provider->setFunction(key, provider->getFunction(key) - val);
	}
	return *this;
}



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
		Set(child.Token(0), (child.Size() >= 2) ? child.Value(1) : 1);
}



void ConditionsStore::Save(DataWriter &out) const
{
	out.Write("conditions");
	out.BeginChild();
	for(auto it = storage.begin(); it != storage.end(); ++it)
	{
		// We don't need to save derived conditions that have a provider.
		if(it->second.provider)
			continue;
		// If the condition's value is 1, don't bother writing the 1.
		else if(it->second.value == 1)
			out.Write(it->first);
		else
			out.Write(it->first, it->second.value);
	}
	out.EndChild();
}



// Get a condition from the Conditions-Store. Retrieves both conditions
// that were directly set (primary conditions) as well as conditions
// derived from other data-structures (derived conditions).
int64_t ConditionsStore::Get(const string &name) const
{
	const ConditionEntry *ce = GetEntry(name);
	if(!ce)
		return 0;

	if(!ce->provider)
		return ce->value;

	return ce->provider->getFunction(name);
}



bool ConditionsStore::Has(const string &name) const
{
	const ConditionEntry *ce = GetEntry(name);
	if(!ce)
		return false;

	if(!ce->provider)
		return true;

	return ce->provider->hasFunction(name);
}



// Returns a pair where the boolean indicates if the game has this condition set,
// and an int64_t which contains the value if the condition was set.
pair<bool, int64_t> ConditionsStore::HasGet(const string &name) const
{
	const ConditionEntry *ce = GetEntry(name);
	if(!ce)
		return make_pair(false, 0);

	if(!ce->provider)
		return make_pair(true, ce->value);

	bool has = ce->provider->hasFunction(name);
	int64_t val = 0;
	if(has)
		val = ce->provider->getFunction(name);

	return make_pair(has, val);
}



// Add a value to a condition. Returns true on success, false on failure.
bool ConditionsStore::Add(const string &name, int64_t value)
{
	// This code performers 2 lookups of the condition, once for get and
	// once for set. This might be optimized to a single lookup in a
	// later version of the code.
	return Set(name, Get(name) + value);
}



// Set a value for a condition, either for the local value, or by performing
// a set on the provider.
bool ConditionsStore::Set(const string &name, int64_t value)
{
	ConditionEntry *ce = GetEntry(name);
	if(!ce)
	{
		(storage[name]).value = value;
		return true;
	}
	if(!ce->provider)
	{
		ce->value = value;
		return true;
	}
	return ce->provider->setFunction(name, value);
}



// Erase a condition completely, either the local value or by performing
// an erase on the provider.
bool ConditionsStore::Erase(const string &name)
{
	ConditionEntry *ce = GetEntry(name);
	if(!ce)
		return true;

	if(!(ce->provider))
	{
		storage.erase(name);
		return true;
	}
	return ce->provider->eraseFunction(name);
}



ConditionsStore::ConditionEntry &ConditionsStore::operator[](const string &name)
{
	// Search for an exact match and return it if it exists.
	auto it = storage.find(name);
	if(it != storage.end())
		return it->second;

	// Check for a prefix provider.
	ConditionEntry *ceprov = GetEntry(name);
	// If no prefix provider is found, then just create a new value entry.
	if(ceprov == nullptr)
		return storage[name];

	// Found a matching prefixed entry provider, but no exact match for the entry itself,
	// let's create the exact match based on the prefix provider.
	ConditionEntry &ce = storage[name];
	ce.provider = ceprov->provider;
	ce.fullKey = name;
	return ce;
}



// Build a provider for a given prefix.
ConditionsStore::DerivedProvider &ConditionsStore::GetProviderPrefixed(const string &prefix)
{
	auto it = providers.emplace(std::piecewise_construct,
		std::forward_as_tuple(prefix),
		std::forward_as_tuple(prefix, true));
	DerivedProvider *provider = &(it.first->second);
	if(!provider->isPrefixProvider)
	{
		Logger::LogError("Error: Rewriting named provider \"" + prefix + "\" to prefixed provider.");
		provider->isPrefixProvider = true;
	}
	if(VerifyProviderLocation(prefix, provider))
	{
		storage[prefix].provider = provider;
		// Check if any matching later entries within the prefixed range use the same provider.
		auto checkIt = storage.find(prefix);
		while(checkIt != storage.end() && (0 == checkIt->first.compare(0, prefix.length(), prefix)))
		{
			ConditionEntry &ce = checkIt->second;
			if(ce.provider != provider)
			{
				ce.provider = provider;
				ce.fullKey = checkIt->first;
				throw runtime_error("Replacing condition entries matching prefixed provider \""
						+ prefix + "\".");
			}
			++checkIt;
		}
	}
	return *provider;
}



// Build a provider for the condition identified by the given name.
ConditionsStore::DerivedProvider &ConditionsStore::GetProviderNamed(const string &name)
{
	auto it = providers.emplace(std::piecewise_construct,
		std::forward_as_tuple(name),
		std::forward_as_tuple(name, false));
	DerivedProvider *provider = &(it.first->second);
	if(provider->isPrefixProvider)
		Logger::LogError("Error: Retrieving prefixed provider \"" + name + "\" as named provider.");
	else if(VerifyProviderLocation(name, provider))
		storage[name].provider = provider;
	return *provider;
}



// Helper to completely remove all data and linked condition-providers from the store.
void ConditionsStore::Clear()
{
	storage.clear();
	providers.clear();
}



// Helper for testing; check how many primary conditions are registered.
int64_t ConditionsStore::PrimariesSize() const
{
	int64_t result = 0;
	for(auto it = storage.begin(); it != storage.end(); ++it)
	{
		// We only count primary conditions; conditions that don't have a provider.
		if(it->second.provider)
			continue;
		++result;
	}
	return result;
}



ConditionsStore::ConditionEntry *ConditionsStore::GetEntry(const string &name)
{
	// Avoid code-duplication between const and non-const function.
	return const_cast<ConditionsStore::ConditionEntry *>(const_cast<const ConditionsStore *>(this)->GetEntry(name));
}



const ConditionsStore::ConditionEntry *ConditionsStore::GetEntry(const string &name) const
{
	if(storage.empty())
		return nullptr;

	// Perform a single search for values, named providers, and prefixed providers.
	auto it = storage.upper_bound(name);
	if(it == storage.begin())
		return nullptr;

	--it;
	// The entry is matching if we have an exact string match.
	if(!name.compare(it->first))
		return &(it->second);

	// The entry is also matching when we have a prefix entry and the prefix part in the provider matches.
	DerivedProvider *provider = it->second.provider;
	if(provider && provider->isPrefixProvider && !name.compare(0, provider->name.length(), provider->name))
		return &(it->second);

	// And otherwise we don't have a match.
	return nullptr;
}



// Helper function to check if we can safely add a provider with the given name.
bool ConditionsStore::VerifyProviderLocation(const string &name, DerivedProvider *provider) const
{
	auto it = storage.upper_bound(name);
	if(it == storage.begin())
		return true;

	--it;
	const ConditionEntry &ce = it->second;

	// If we find the provider we are trying to add, then it apparently
	// was safe to add the entry since it was already added before.
	if(ce.provider == provider)
		return true;

	if(!ce.provider && it->first == name)
	{
		Logger::LogError("Error: overwriting primary condition \"" + name + "\" with derived provider.");
		return true;
	}

	if(ce.provider && ce.provider->isPrefixProvider && 0 == name.compare(0, ce.provider->name.length(), ce.provider->name))
		throw runtime_error("Error: not adding provider for \"" + name + "\""
				", because it is within range of prefixed derived provider \"" + ce.provider->name + "\".");
	return true;
}
