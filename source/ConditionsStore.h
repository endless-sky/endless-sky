/* ConditionsProvider.h
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

#include "ConditionsProvider.h"

#include <map>
#include <string>



// Class that describes and implements the interface to retrieve and store
// condition-variables. This class can be used as stand-alone conditions-
// provider or as basis for other classes that need the same implementation
// as is done in this class.
// Classes that want to store conditions differently than that is done in
// this class should inherit from ConditionsProvider.
class ConditionsStore : public ConditionsProvider {
public:
	// Retrieve a "condition" flag from this provider.
	int64_t operator [] (const std::string &name) const;
	virtual bool HasCondition(const std::string &name) const override;
	virtual int64_t GetCondition(const std::string &name) const override;
	// Add a value to a condition, set a value for a condition or erase a
	// condition completely. Returns true on success, false on failure.
	virtual bool SetCondition(const std::string &name, int64_t value) override;
	virtual bool EraseCondition(const std::string &name) override;
	
	// Direct (read-only) access to non-child (local to this class) "condition" flags data.
	const std::map<std::string, int64_t> &Locals() const;



protected:
	//Inherited from ConditionsProvider
	virtual void RegisterChild(ConditionsProvider &child, const std::vector<std::string> &matchPrefixes, const std::vector<std::string> &matchExacts) override;
	virtual void DeRegisterChild(ConditionsProvider &child) override;



private:
	ConditionsProvider* GetRegisteredChild(const std::string &name) const;



private:
	// Storage for the actual conditions.
	std::map<std::string, int64_t> conditions;
	// Storage of child providers by prefix and full string.
	std::map<std::string, ConditionsProvider*> matchPrefixes;
	std::map<std::string, ConditionsProvider*> matchExacts;
};



#endif
