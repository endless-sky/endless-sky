/* AttributeStore.h
Copyright (c) 2023 by tibetiroka

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef ATTRIBUTE_STORE_H_
#define ATTRIBUTE_STORE_H_

#include "attribute/Attribute.h"
#include "attribute/AttributeAccessor.h"
#include "DataNode.h"
#include "DataWriter.h"
#include "Dictionary.h"

#include <cmath>
#include <functional>
#include <limits>
#include <map>
#include <string>

class AttributeStore {
public:
	// Checks whether the specified attribute is defined here.
	bool IsPresent(const AttributeAccessor attribute) const;
	template <class T>
	bool IsPresent(const T &attribute) const;
	bool IsPresent(const char *attribute) const;
	// Gets the value of the specified attribute, or 0 if not present.
	double Get(const AttributeAccessor attribute) const;
	template <class T>
	double Get(const T &attribute) const;
	double Get(const char *attribute) const;
	// Gets the attribute for the specified category, or nullptr.
	const Attribute *GetAttribute(const AttributeCategory category) const;
	Attribute *GetAttribute(const AttributeCategory category);
	const AttributeEffect *GetEffect(const AttributeAccessor access) const;
	AttributeEffect *GetEffect(const AttributeAccessor access);
	// Sets the value of the specified attribute. If the attribute is not present, it is added to this collection
	// with this value.
	void Set(const AttributeAccessor attribute, double value);
	template <class T>
	void Set(const T &attribute, double value);
	void Set(const char *attribute, double value);

	// Checks whether there are any attributes stored here.
	bool empty() const;

	// Gets the minimum allowed value of the attribute.
	double GetMinimum(const AttributeAccessor attribute) const;
	template <class T>
	double GetMinimum(const T &attribute) const;
	double GetMinimum(const char *attribute) const;

	// Loads data from the data node. This function can be called multiple times on an instance.
	void Load(const DataNode &node);
	// Writes the attributes into the data writer.
	void Save(DataWriter &writer) const;

	// Calculates how effectively the action described by the category can be done.
	// Some actions, like active cooling, have different performance under different conditions,
	// while others are either actioned or not. Returns 0 if the action cannot be executed.
	// Only supported for attribute categories that describe actions.
	double GetEfficiency(const AttributeCategory category, const bool allowFractional) const;
	// Applies the effects of the specified action to the target. If the efficiency is 0 or negative,
	// the call is a no-op. Only supported for attribute categories that describe actions.
	void ApplyEffects(const AttributeCategory category, const AttributeStore target, const double efficiency = 1.) const;

	// Determine whether the given number of instances of the given attributes can
	// be added to this instance. If not, return the std::maximum number that can be added.
	int CanAdd(const AttributeStore &other, int count) const;
	void Add(const AttributeStore &other, const int count);
	// Adds attributes the specified number of times.
	template <class A>
	void Add(const A &attribute, const AttributeStore &other, const int count);
	void Add(const char *attribute, const AttributeStore &other, const int count);
	template <class A>
	void Add(const A &attribute, const double amount);
	void Add(const char *attribute, const double amount);

	// Calls the given function on all attributes.
	void ForEach(const std::function<void(const AnyAttribute &, double)> &function) const;

private:
	template <typename A>
	int CanAdd(const A &attribute, const AttributeStore &other, int count) const;

private:
	const static std::map<std::string, double> MINIMUM_OVERRIDES;
	Dictionary textAttributes;
	std::map<AttributeCategory, Attribute> categorizedAttributes;
};



template <>
inline double AttributeStore::Get(const std::string &attribute) const
{
	return textAttributes.Get(attribute);
}



template <>
inline double AttributeStore::Get(const AnyAttribute &attribute) const
{
	return attribute.index() ? Get(std::get<1>(attribute)) : Get(std::get<0>(attribute));
}



template <>
inline void AttributeStore::Set(const std::string &attribute, double value)
{
	auto it = MINIMUM_OVERRIDES.find(attribute);
	if(it != MINIMUM_OVERRIDES.end())
		value = std::max(value, it->second);
	if(value && fabs(value) < AttributeEffect::EPS)
		value = 0.;
	textAttributes[attribute] = value;
}



template <>
inline void AttributeStore::Set(const AnyAttribute &attribute, double value)
{
	attribute.index() ? Set(std::get<1>(attribute), value) : Set(std::get<0>(attribute), value);
}



template <>
inline bool AttributeStore::IsPresent(const std::string &attribute) const
{
	return Get(attribute) != 0.;
}



template <>
inline bool AttributeStore::IsPresent(const AnyAttribute &attribute) const
{
	return attribute.index() ? IsPresent(std::get<1>(attribute)) : IsPresent(std::get<0>(attribute));
}



template <>
inline double AttributeStore::GetMinimum(const std::string &attribute) const
{
	auto it = MINIMUM_OVERRIDES.find(attribute);
	if(it != MINIMUM_OVERRIDES.end())
		return it->second;
	return 0.;
}



// Determine whether the attribute can be added to this instance 'count' times.
// If not, return the maximum number that can be added.
template <class A>
inline int AttributeStore::CanAdd(const A &attribute, const AttributeStore &other, int count) const
{
	if(count)
	{
		double minimum = GetMinimum(attribute);
		if(attribute == "required crew")
			minimum = !(IsPresent("automaton") || other.IsPresent("automaton"));

		double value = Get(attribute);
		double amount = other.Get(attribute);
		// Allow for rounding errors:
		if(value + amount * count < minimum - AttributeEffect::EPS)
			return (value - minimum) / -amount + AttributeEffect::EPS;
	}
	return count;
}



template <class A>
inline void AttributeStore::Add(const A &attribute, const AttributeStore &other, const int count)
{
	Set(attribute, Get(attribute) + other.Get(attribute) * count);
}



inline void AttributeStore::Add(const char *attribute, const AttributeStore &other, const int count)
{
	Set(attribute, Get(attribute) + other.Get(attribute) * count);
}



template<class A>
inline void AttributeStore::Add(const A &attribute, const double amount)
{
	Set(attribute, Get(attribute) + amount);
}



template <>
inline void AttributeStore::Add(const Attribute &attribute, const double amount)
{
	Attribute copy(attribute, amount);
	Attribute *current = GetAttribute(attribute.Category());
	if(current == nullptr)
		categorizedAttributes.insert({attribute.Category(), copy});
	else
		for(const auto &[type, effect] : copy.Effects())
			current->AddEffect(effect);
}



inline void AttributeStore::Add(const char *attribute, const double amount)
{
	Add(std::string(attribute), amount);
}

#endif
