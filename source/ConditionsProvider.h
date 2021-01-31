/* ConditionsProvider.h
Copyright (c) 2020 by Peter van der Meer

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef CONDITIONS_PROVIDER_H_
#define CONDITIONS_PROVIDER_H_

#include <map>
#include <string>
#include <vector>



// Class that describes the interface to retrieve and store condition-variables.
// This class is to be inherited from by any class that provides condition-
// variables.
class ConditionsProvider {
public:
	// Retrieve a "condition" flag from this provider.
	virtual int64_t GetCondition(const std::string &name) const = 0;
	virtual bool HasCondition(const std::string &name) const;
	// Add a value to a condition, set a value for a condition or erase a
	// condition completely. Returns true on success, false on failure.
	virtual bool AddCondition(const std::string &name, int64_t value);
	virtual bool SetCondition(const std::string &name, int64_t value);
	virtual bool EraseCondition(const std::string &name);


protected:
	// Register the conditions for which a child of this provider acts.
	// matchPrefixes contains the prefixes for which the child acts, for example
	// "ship: " for the child that provides ship-related conditions and "preferences: "
	// for if we also want to make the Preferences classes a conditions provider.
	// matchExacts contains the exact strings for which the child acts, for example
	// automatic conditions like the in-game date.
	// As before, this can also be handled through other means and is thus not required
	// for all parent/child relations.
	virtual void RegisterChild(ConditionsProvider &child, const std::vector<std::string> &matchPrefixes, const std::vector<std::string> &matchExacts);

	// Remove registration for a child. Required to be called if the child was registered and ceases to exist.
	virtual void DeRegisterChild(ConditionsProvider &child);
};



#endif
