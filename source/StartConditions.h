/* StartConditions.h
Copyright (c) 2015 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef START_CONDITIONS_H_
#define START_CONDITIONS_H_

#include "Account.h"
#include "ConditionSet.h"
#include "Date.h"

#include <list>

class DataNode;
class Planet;
class Ship;
class System;



class StartConditions {
public:
	void Load(const DataNode &node);
	// Finish loading the ship definitions.
	void FinishLoading();
	
	Date GetDate() const;
	const Planet *GetPlanet() const;
	const System *GetSystem() const;
	const Account &GetAccounts() const;
	const ConditionSet &GetConditions() const;
	const std::list<Ship> &Ships() const;
	
	
private:
	Date date;
	const Planet *planet = nullptr;
	const System *system = nullptr;
	Account accounts;
	ConditionSet conditions;
	std::list<Ship> ships;
};



#endif
