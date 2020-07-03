/* Legality.h
Copyright (c) 2020 by Eloraam

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef LEGALITY_H_
#define LEGALITY_H_

#include "DataNode.h"

#include <map>

class Government;

// This class represents a class of legality for a cargo, outfit or mission,
// allowing different governments to respond in different ways.
class Legality
{
public:
	Legality()=default;
	// COnstruct and Load() at the same time.
	Legality(const DataNode& node);

	void Load(const DataNode& node);
	void SetNumeric(const std::string &value);

	const std::string& Name() const;
	int64_t GetFine(const Government *gov) const;
private:
	int64_t defaultFine=0;
	std::map<const Government*,int64_t> specificFines;
	std::string name;
};

#endif
