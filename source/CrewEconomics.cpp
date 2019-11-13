/* CrewEconomics.cpp
Copyright (c) 2019 by Luke Arndt

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "CrewEconomics.h"

int64_t CrewEconomics::CalculateSalaries(const Ship *flagship, const vector<shared_ptr<Ship>> ships)
{
	int64_t commanders = 0;
	int64_t officers = 0;
	int64_t totalCrew = 0;
	int64_t totalSalaries = 0;

	// Even if a ship is parked, we have to pay its crew.
	// In the future, paid shore leave will improve the ship's morale.
	for(const shared_ptr<Ship> &ship : ships)
		if(!ship->IsDestroyed()) {
			// Every ship needs one commander.
			commanders += 1;
			// We need officers to manage our regular crew.
			// If we ever support hiring more crew for escorts, we should use ship->Crew() for these.
			officers += ship->RequiredCrew() / CREW_PER_OFFICER;
			// This is easier than omitting commanders and officers as we go.
			totalCrew += ship->RequiredCrew();
		}

	// Add any extra crew from the flagship.
	if(flagship)
		totalCrew += flagship->Crew() - flagship->RequiredCrew();

	// We don't need a commander for the flagship. We command it directly.
	totalSalaries += (commanders - 1) * CREDITS_PER_COMMANDER;

	totalSalaries += officers * CREDITS_PER_OFFICER;

	// Commanders and officers are not regular crew members.
	totalSalaries += (totalCrew - commanders - officers) * CREDITS_PER_REGULAR;

	return totalSalaries;
}
