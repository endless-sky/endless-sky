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



// Purely virtual interface class that describes the interface to retrieve
// and store derived condition-variables. This interace is to be implemented
// by any class that provides such derived condition-variables.
class ConditionsProvider {
public:
	// Retrieve a "condition" flag from this provider.
	virtual int64_t GetCondition(const std::string &name) const = 0;
	virtual bool HasCondition(const std::string &name) const = 0;
	// Set a value for a condition or erase a condition completely. Returns
	// true on success, false on failure.
	virtual bool SetCondition(const std::string &name, int64_t value) = 0;
	virtual bool EraseCondition(const std::string &name) = 0;
};



#endif
