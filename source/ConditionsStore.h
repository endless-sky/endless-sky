/* ConditionsStore.h
Copyright (c) 2020 by Peter van der Meer

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef CONDITIONS_STORE_H_
#define CONDITIONS_STORE_H_

#include <initializer_list>
#include <map>
#include <string>



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
	ConditionsStore();
	ConditionsStore(std::initializer_list<std::pair<std::string, int64_t>> initialConditions);
	ConditionsStore(const std::map<std::string, int64_t> initialConditions);

	// Retrieve a "condition" flag from this provider.
	int64_t operator[](const std::string &name) const;
	bool HasCondition(const std::string &name) const;
	bool AddCondition(const std::string &name, int64_t value);
	// Add a value to a condition, set a value for a condition or erase a
	// condition completely. Returns true on success, false on failure.
	bool SetCondition(const std::string &name, int64_t value);
	bool EraseCondition(const std::string &name);
	
	// Direct (read-only) access to the stored primary conditions.
	const std::map<std::string, int64_t> &GetPrimaryConditions() const;



private:
	// Storage for the primary conditions.
	std::map<std::string, int64_t> conditions;
};



#endif
