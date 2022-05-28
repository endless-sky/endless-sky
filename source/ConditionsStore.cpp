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



// Default constructor
ConditionsStore::DerivedProvider::DerivedProvider(const string &name, bool isPrefixProvider): name(name), isPrefixProvider(isPrefixProvider)
{
}



void ConditionsStore::DerivedProvider::SetGetFun(std::function<int64_t(const std::string&)> newGetFun)
{
	getFun = std::move(newGetFun);
}



void ConditionsStore::DerivedProvider::SetHasFun(std::function<bool(const std::string&)> newHasFun)
{
	hasFun = std::move(newHasFun);
}



void ConditionsStore::DerivedProvider::SetSetFun(std::function<bool(const std::string&, int64_t)> newSetFun)
{
	setFun = std::move(newSetFun);
}



void ConditionsStore::DerivedProvider::SetEraseFun(std::function<bool(const std::string&)> newEraseFun)
{
	eraseFun = std::move(newEraseFun);
}



ConditionsStore::ConditionEntry::operator int64_t() const
{
	if(!provider)
		return value;

	const string &key = fullKey.empty() ? provider->name : fullKey;
	return provider->getFun(key);
}



ConditionsStore::ConditionEntry &ConditionsStore::ConditionEntry::operator=(int64_t val)
{
	if(!provider)
		value = val;
	else
	{
		const string &key = fullKey.empty() ? provider->name : fullKey;
		provider->setFun(key, val);
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
		provider->setFun(key, provider->getFun(key) + 1);
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
		provider->setFun(key, provider->getFun(key) - 1);
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
		provider->setFun(key, provider->getFun(key) + val);
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
		provider->setFun(key, provider->getFun(key) - val);
	}
	return *this;
}



ConditionsStore::PrimariesIterator::PrimariesIterator(CondMapItType it, CondMapItType endIt) : condMapIt(it), condMapEnd(endIt)
{
	MoveToValueCondition();
};



pair<string, int64_t> ConditionsStore::PrimariesIterator::operator*() const
{
	return itVal;
}



const pair<string, int64_t> *ConditionsStore::PrimariesIterator::operator->()
{
	return &itVal;
}



ConditionsStore::PrimariesIterator &ConditionsStore::PrimariesIterator::operator++()
{
	condMapIt++;
	MoveToValueCondition();
	return *this;
};



ConditionsStore::PrimariesIterator ConditionsStore::PrimariesIterator::operator++(int)
{
	PrimariesIterator tmp = *this;
	condMapIt++;
	MoveToValueCondition();
	return tmp;
};



// Equation operators, we can just compare the upstream iterators.
bool ConditionsStore::PrimariesIterator::operator==(const ConditionsStore::PrimariesIterator& rhs) const
{
	return condMapIt == rhs.condMapIt;
}



bool ConditionsStore::PrimariesIterator::operator!=(const ConditionsStore::PrimariesIterator& rhs) const
{
	return condMapIt != rhs.condMapIt;
}



// Helper function to ensure that the primary-conditions iterator points
// to a primary (value) condition or to the end-iterator value.
void ConditionsStore::PrimariesIterator::MoveToValueCondition()
{
	while((condMapIt != condMapEnd) && (condMapIt->second).provider)
		condMapIt++;

	if(condMapIt != condMapEnd)
		itVal = make_pair(condMapIt->first, (condMapIt->second).value);
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

	return ce->provider->getFun(name);
}



bool ConditionsStore::Has(const string &name) const
{
	const ConditionEntry *ce = GetEntry(name);
	if(!ce)
		return false;

	if(!ce->provider)
		return true;

	return ce->provider->hasFun(name);
}



bool ConditionsStore::empty() const
{
	return storage.empty();
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
	return ce->provider->setFun(name, value);
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
	return ce->provider->eraseFun(name);
}



ConditionsStore::ConditionEntry &ConditionsStore::operator[](const std::string &name)
{
	// If storage in empty, then create the value condition directly.
	if(storage.empty())
		return storage[name];

	// If no candidate entries are found, then create it.
	auto it = storage.upper_bound(name);
	if(it == storage.begin())
		return storage[name];

	--it;
	// The entry is valid if we have an exact string match, but also when we have a
	// prefix entry and the prefix part matches.
	if(!(name.compare(0, it->first.length(), it->first)))
	{
		// If we have an exact match, then we have the entry we want.
		if(it->first.length() == name.length())
			return it->second;

		// If we found a matched prefixed entry provider, but no exact match for
		// the entry itself, then create a new prefixed entry based on the one we
		// found.
		DerivedProvider *provider = it->second.provider;
		if(provider && provider->isPrefixProvider)
		{
			ConditionEntry &ce = storage[name];
			ce.provider = provider;
			ce.fullKey = name;
			return ce;
		}
	}
	// If the entry is not found, then create it.
	return storage[name];
}



ConditionsStore::PrimariesIterator ConditionsStore::PrimariesBegin() const
{
	return PrimariesIterator(storage.begin(), storage.end());
}



ConditionsStore::PrimariesIterator ConditionsStore::PrimariesEnd() const
{
	return PrimariesIterator(storage.end(), storage.end());
}



ConditionsStore::PrimariesIterator ConditionsStore::PrimariesLowerBound(const string &key) const
{
	return PrimariesIterator(storage.lower_bound(key), storage.end());
}



// Build a provider for a given prefix.
ConditionsStore::DerivedProvider &ConditionsStore::GetProviderPrefixed(const string &prefix)
{
	auto it = providers.emplace(std::piecewise_construct,
		std::forward_as_tuple(prefix),
		std::forward_as_tuple(prefix, true));
	storage[prefix].provider = &(it.first->second);
	return it.first->second;
}



// Build a provider for the condition identified by the given name.
ConditionsStore::DerivedProvider &ConditionsStore::GetProviderNamed(const string &name)
{
	auto it = providers.emplace(std::piecewise_construct,
		std::forward_as_tuple(name),
		std::forward_as_tuple(name, false));
	storage[name].provider = &(it.first->second);
	return it.first->second;
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

	// Perform a single search for values, named providers, and prefixed providers.
	auto it = storage.upper_bound(name);
	if(it == storage.begin())
		return nullptr;

	--it;
	// The entry is valid if we have an exact string match, but also when we have a
	// prefix entry and the prefix part matches.
	if(!(name.compare(0, it->first.length(), it->first)))
	{
		DerivedProvider *provider = it->second.provider;
		if(it->first.length() == name.length() ||
				(provider && provider->isPrefixProvider))
			return &(it->second);
	}
	return nullptr;
}



const ConditionsStore::ConditionEntry *ConditionsStore::GetEntry(const string &name) const
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
	if(!(name.compare(0, it->first.length(), it->first)))
	{
		DerivedProvider *provider = it->second.provider;
		if(it->first.length() == name.length() ||
				(provider && provider->isPrefixProvider))
			return &(it->second);
	}
	return nullptr;
}
