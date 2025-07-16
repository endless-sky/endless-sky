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
	: name(name), value(0), providingEntry(nullptr)
{
}



const string &ConditionEntry::Name() const
{
	return name;
}



const string ConditionEntry::NameWithoutPrefix() const
{
	// If we have a provider, and that provider has a main-entry, then we have prefix.
	if(providingEntry)
		return name.substr(providingEntry->name.length());

	// If we have no prefix, then return the name without prefix.
	return name;
}



ConditionEntry::operator int64_t() const
{
	// Prefixed provider; use the other function to get the value.
	if(providingEntry)
		return providingEntry->getFunction(*this);

	// Named provider; use the local getFunction to get the value.
	if(getFunction)
		return getFunction(*this);

	// This is not a provider, just return the value.
	return value;
}



ConditionEntry &ConditionEntry::operator=(int64_t val)
{
	// Set through prefixed provider, notify if the function is read/write.
	if(providingEntry)
	{
		if(providingEntry->setFunction)
		{
			providingEntry->setFunction(*this, val);
			NotifyUpdate(val);
		}
	}
	// Set through named provider, notify if the function is read/write.
	else if(getFunction)
	{
		if(setFunction)
		{
			setFunction(*this, val);
			NotifyUpdate(val);
		}
	}
	// Set value directly.
	else
	{
		value = val;
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
	this->getFunction = std::move(getFunction);
	this->providingEntry = this;
}



void ConditionEntry::ProvidePrefixed(function<int64_t(const ConditionEntry &)> getFunction,
	function<void(ConditionEntry &, int64_t)> setFunction)
{
	ProvidePrefixed(getFunction);
	this->setFunction = std::move(setFunction);
}



void ConditionEntry::ProvideNamed(function<int64_t(const ConditionEntry &)> getFunction)
{
	this->getFunction = std::move(getFunction);
	this->providingEntry = nullptr;
}



void ConditionEntry::ProvideNamed(function<int64_t(const ConditionEntry &)> getFunction,
	function<void(ConditionEntry &, int64_t)> setFunction)
{
	ProvideNamed(getFunction);
	this->setFunction = std::move(setFunction);
}



void ConditionEntry::NotifyUpdate(uint64_t value)
{
	// Subscribing is not implemented yet.
	return;
}
