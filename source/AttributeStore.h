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

#pragma once

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
	// Gets the value of the specified attribute, or 0 if not present.
	double Get(const AnyAttribute &attribute) const;
	// Gets the attribute for the specified category, or nullptr.
	const Attribute *GetAttribute(AttributeCategory category) const;
	Attribute *GetAttribute(AttributeCategory category);
	const AttributeEffect *GetEffect(AttributeAccessor access) const;
	AttributeEffect *GetEffect(AttributeAccessor access);
	// Sets the value of the specified attribute. If the attribute is not present, it is added to this collection
	// with this value.
	void Set(const AnyAttribute &attribute, double value);

	// Checks whether there are any attributes stored here.
	bool Empty() const;

	// Gets the minimum allowed value of the attribute.
	double GetMinimum(const AnyAttribute &attribute) const;

	// Loads data from the data node. This function can be called multiple times on an instance.
	void Load(const DataNode &node);
	// Writes the attributes into the data writer.
	void Save(DataWriter &writer) const;

	// Determine whether the given number of instances of the given attributes can
	// be added to this instance. If not, return the maximum number that can be added.
	int CanAdd(const AttributeStore &other, int count) const;
	// Adds every attribute from the given AttributeStore to this AttributeStore the specified number of times.
	void Add(const AttributeStore &other, int count);
	// Add a specific attribute the specified number of times.
	void Add(const AnyAttribute &attribute, const AttributeStore &other, int count);
	void Add(const AnyAttribute &attribute, double amount);

	// Calls the given function on all attributes.
	void ForEach(const std::function<void(const AnyAttribute &, double)> &function) const;

private:
	int CanAdd(const AnyAttribute &attribute, const AttributeStore &other, int count) const;
	void Add(const Attribute &attribute, double amount);

private:
	// Regular attributes are stored in text form. These can be arbitrarily defined by users,
	// and generally aren't used in flight.
	Dictionary textAttributes;
	// The attributes that describe various flight effects, i.e. what happens during shield generation, thrusting etc.
	std::map<AttributeCategory, Attribute> categorizedAttributes;
};
