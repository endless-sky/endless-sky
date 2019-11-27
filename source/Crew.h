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

class Crew
{
public:
	// Calculate one day's salaries for the Player's fleet
	static int64_t CalculateSalaries(const std::vector<std::shared_ptr<Ship>> &ships, const Ship * flagship, const bool includeExtras = true);
	
	// Build a list of all crew members and how many are on the ship
	static const std::map<const std::string, int64_t> CrewManifest(const std::shared_ptr<Ship> &ship, bool isFlagship, bool includeExtras = true);

	// Calculate the total cost of the flagship's extra crew
	static int64_t CostOfExtraCrew(const std::vector<std::shared_ptr<Ship>> &ships, const Ship * flagship);

	// Figure out how many of a given crew member are on a ship
	static int64_t NumberOnShip(const Crew &crew, const std::shared_ptr<Ship> &ship, const bool isFlagship, const bool includeExtras = true);

	// Calculate one day's salaries for a ship
	static int64_t SalariesForShip(const std::shared_ptr<Ship> &ship, const bool isFlagship, const bool includeExtras = true);

	// Load a definition for a crew member.
	void Load(const DataNode &node);
	
	bool AvoidsEscorts() const;
	bool AvoidsFlagship() const;
	int64_t ParkedSalary() const;
	int64_t PopulationPerMember() const;
	int64_t Salary() const;
	const std::string &Id() const;
	const std::string &Name() const;
	const std::vector<int64_t> &PlaceAt() const;

private:
	// If true, the crew member will not appear on escorts
	bool avoidsEscorts = false;
	// If true, the crew member will not appear on the flagship
	bool avoidsFlagship = false;
	// The number of credits paid daily while parked (minimum 0)
	int64_t parkedSalary = 0;
	// Every nth crew member on the ship will be this crew member
	int64_t populationPerMember = 0;
	// The number of credits paid daily (minimum 0)
	int64_t salary = 100;
	// The id that the crew member is stored against in GameData::Crews()
	std::string id;
	// The display name for this kind of crew members (plural, Title Case)
	std::string name;
	// The crew member will be placed at these crew member numbers if possible
	// Note: if multiple crew definitions claim the same crew positions,
	// we can end up paying for more crew than we expect to.
	// To avoid this, don't place different crew members in the same spots.
	// Example usage: "place at" 1 3 5 7 13
	std::vector<int64_t> placeAt;
};

#endif
