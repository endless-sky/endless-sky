/* ConditionEntry.cpp
Copyright (c) 2024-2025 by Peter van der Meer

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "ConditionEntry.h"


using namespace std;



ConditionEntry::ConditionEntry(const string &name)
	: name(name), value(0), provider(nullptr)
{
}



ConditionEntry::~ConditionEntry()
{
	Clear();
}



void ConditionEntry::Clear()
{
	// Remove the provider, if this is the main condition that created it.
	if(provider && ((provider->mainEntry == nullptr) || provider->mainEntry == this))
		delete provider;
	provider = nullptr;

	// Reset the value to the default.
	value = 0;
}



const string &ConditionEntry::Name() const
{
	return name;
}



const string ConditionEntry::NameWithoutPrefix() const
{
	// If we have a provider, and that provider has a main-entry, then we have prefix.
	if(provider && provider->mainEntry)
		return name.substr(provider->mainEntry->name.length());

	// If we have no prefix, then return the name without prefix.
	return name;
}



ConditionEntry::DerivedProvider::DerivedProvider(ConditionEntry *mainEntry)
	: mainEntry(mainEntry)
{
}



ConditionEntry::operator int64_t() const
{
	if(!provider)
		return value;

	return provider->getFunction(*this);
}



ConditionEntry &ConditionEntry::operator=(int64_t val)
{
	if(!provider)
	{
		value = val;
		NotifyUpdate(val);
	}
	else if(provider->setFunction)
	{
		provider->setFunction(*this, val);
		NotifyUpdate(val);
	}
	return *this;
}



ConditionEntry &ConditionEntry::operator++()
{
	// Reuse assignment and int64_t conversion operators.
	return (*this) = (*this) + 1;
}



ConditionEntry &ConditionEntry::operator--()
{
	// Reuse assignment and int64_t conversion operators.
	return (*this) = (*this) - 1;
}



ConditionEntry &ConditionEntry::operator+=(int64_t val)
{
	// Reuse assignment and int64_t conversion operators.
	return (*this) = (*this) + val;
}



ConditionEntry &ConditionEntry::operator-=(int64_t val)
{
	// Reuse assignment and int64_t conversion operators.
	return (*this) = (*this) - val;
}



void ConditionEntry::ProvidePrefixed(function<int64_t(const ConditionEntry &)> getFunction)
{
	if(!provider)
		provider = new DerivedProvider(this);
	provider->getFunction = std::move(getFunction);
}



void ConditionEntry::ProvidePrefixed(function<int64_t(const ConditionEntry &)> getFunction,
	function<void(ConditionEntry &, int64_t)> setFunction)
{
	ProvidePrefixed(getFunction);
	provider->setFunction = std::move(setFunction);
}



void ConditionEntry::ProvideNamed(function<int64_t(const ConditionEntry &)> getFunction)
{
	if(!provider)
		provider = new DerivedProvider(nullptr);
	provider->getFunction = std::move(getFunction);
}



void ConditionEntry::ProvideNamed(function<int64_t(const ConditionEntry &)> getFunction,
	function<void(ConditionEntry &, int64_t)> setFunction)
{
	ProvideNamed(getFunction);
	provider->setFunction = std::move(setFunction);
}



void ConditionEntry::NotifyUpdate(uint64_t value)
{
	// Subscribing is not implemented yet.
	return;
}
