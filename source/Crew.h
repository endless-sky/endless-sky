/* Crew.h
Copyright (c) 2019 by Luke Arndt

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef CREW_H_
#define CREW_H_

#include "Ship.h"

using namespace std;

class Crew
{
public:
	// Calculate one day's salaries for the Player's fleet
	static int64_t CalculateSalaries(
		const vector<shared_ptr<Ship>> ships,
		const bool includeExtras = true
	);

	// Calculate the total cost of the flagship's extra crew
	static int64_t CostOfExtraCrew(const vector<shared_ptr<Ship>> ships);

	// Figure out how many of a given crew member are on a ship
	static int64_t NumberOnShip(
		const Crew crew,
		const shared_ptr<Ship> ship,
		const bool isFlagship,
		const bool includeExtras = true
	);

	// Calculate one day's salaries for a ship
	static int64_t SalariesForShip(
		const shared_ptr<Ship> ship,
		const bool isFlagship,
		const bool includeExtras = true
	);

	// Load a definition for a crew economics setting.
	void Load(const DataNode &node);
	
	const bool &IsOnEscorts() const;
	const bool &IsOnFlagship() const;
	const bool &IsPaidSalaryWhileParked() const;
	const int64_t &DailySalary() const;
	const int64_t &MinimumPerShip() const;
	const int64_t &PopulationPerOccurrence() const;
	const std::string &Name() const;

private:
	bool isOnEscorts;
	bool isOnFlagship;
	bool isPaidSalaryWhileParked;
	int64_t dailySalary;
	int64_t minimumPerShip;
	int64_t populationPerOccurrence;
	std::string name;
};

#endif
