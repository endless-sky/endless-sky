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

#ifndef ATTRIBUTES_H_
#define ATTRIBUTES_H_

#include "Attribute.h"
#include "DataNode.h"
#include "DataWriter.h"
#include "Dictionary.h"

#include <functional>
#include <map>
#include <set>
#include <string>

class AttributeStore {
public:
	// Checks whether the specified attribute is defined here.
	template <class A>
	bool IsPresent(const A &attribute) const;
	bool IsPresent(const char *attribute) const;
	// Gets the value of the specified attribute, or 0 if not present.
	template <class A>
	double Get(const A &attribute) const;
	double Get(const char *attribute) const;
	// Sets the value of the specified attribute. If the attribute is not present, it is added to this collection
	// with this value.
	template <class A>
	void Set(const A &attribute, double value);
	void Set(const char *attribute, double value);

	// Checks whether there are any attributes stored here.
	bool empty() const;

	// Gets the minimum allowed value of the attribute.
	template <class A>
	double GetMinimum(const A &attribute) const;
	double GetMinimum(const char *attribute) const;

	// Loads data from the data node. This function can be called multiple times on an instance.
	void Load(const DataNode &node, const bool isWeapon = false, const Attribute parent =
			Attribute(static_cast<AttributeCategory>(-1), static_cast<AttributeEffect>(-1)));
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
	// be added to this instance. If not, return the maximum number that can be added.
	int CanAdd(const AttributeStore &other, int count) const;

	// Adds attributes the specified number of times.
	void Add(const AttributeStore &other, const int count);
	template <class A>
	void Add(const A &attribute, const AttributeStore &other, const int count);
	template <class A>
	void Add(const A &attribute, const double amount);
	void Add(const char *attribute, const double amount);

	// Calls the given function on all attributes.
	void ForEach(const std::function<void(const std::tuple<std::string,Attribute*,double>)> &function) const;

private:
	template <class A>
	int CanAdd(const A &attribute, const AttributeStore &other, int count) const;
	// Saves an attribute. Also gets the set of already saved attributes and the last saved attribute.
	void Save(DataWriter &writer, const Attribute &attribute, std::set<Attribute> &written, Attribute &previous) const;

private:
	Dictionary textAttributes;
	std::map<Attribute, double> categorizedAttributes;
};



#endif
