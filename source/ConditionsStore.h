/* ConditionsStore.h
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

#pragma once

#include "ConditionEntry.h"

#include <cstdint>
#include <functional>
#include <initializer_list>
#include <map>

class DataNode;
class DataWriter;



// Class that contains storage for conditions. Those conditions can be
// set directly in internal storage of this class (primary conditions)
// and those conditions can also be provided from other locations in the
// code (derived conditions).
//
// The derived conditions are typically provided "on demand" from outside
// this storage class. Some of those derived conditions could be read-only
// and in a number of cases the conditions might be converted from other
// data types than int64_t (for example double, float or even complex
// formulae).
class ConditionsStore {
public:
	// Constructors to initialize this class.
	ConditionsStore() = default;
	explicit ConditionsStore(const DataNode &node);
	explicit ConditionsStore(std::initializer_list<std::pair<std::string, int64_t>> initialConditions);
	explicit ConditionsStore(const std::map<std::string, int64_t> &initialConditions);

	ConditionsStore(const ConditionsStore &) = delete;
	ConditionsStore &operator=(const ConditionsStore &) = delete;
	ConditionsStore(ConditionsStore &&) = delete;
	ConditionsStore &operator=(ConditionsStore &&) = default;

	// Serialization support for this class.
	void Load(const DataNode &node);
	void Save(DataWriter &out) const;

	/// Retrieve a "condition" flag from this store (directly or from a connected provider).
	int64_t Get(const std::string &name) const;

	/// Adds a value to the given condition. Can (silently) fail to apply when the condition is a readonly derived
	/// condition, or when a readwrite derived condition doesn't accept the new value.
	void Add(const std::string &name, int64_t value);
	/// Sets a condition to the given value. Can (silently) fail to apply when the condition is a readonly derived
	/// condition, or when a readwrite derived condition doesn't accept the new value.
	void Set(const std::string &name, int64_t value);

	/// Direct access to a specific condition (using the ConditionEntry as proxy).
	ConditionEntry &operator[](const std::string &name);

	// Helper for testing; check how many primary conditions are registered.
	int64_t PrimariesSize() const;


private:
	// Retrieve a condition entry based on a condition name, the entry doesn't
	// get created if it doesn't exist yet (the Set function will handle
	// creation if required).
	ConditionEntry *GetEntry(const std::string &name);
	const ConditionEntry *GetEntry(const std::string &name) const;


private:
	// Storage for both the primary conditions as well as the providers.
	std::map<std::string, ConditionEntry> storage;
};
