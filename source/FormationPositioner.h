/* FormationPositioner.h
Copyright (c) 2019-2021 by Peter van der Meer

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef FORMATION_POSITIONER_H_
#define FORMATION_POSITIONER_H_

#include "Angle.h"
#include "Body.h"
#include "FormationPattern.h"

#include <map>
#include <memory>
#include <vector>

class Ship;



// Represents an active formation for a set of spaceships. Assigns each ship
// to a position (Point) in the formation.
class FormationPositioner {
public:
	// Initializer based on the formation pattern to follow.
	FormationPositioner(const Body *formationLead, const FormationPattern *pattern);
	
	// TODO: Should we replace the Start() and NextPosition() by Add(Ship*), Remove(Ship*) and GetPosition(Ship*)? (and move some calculations now done every frame to the Add function?)
	// Start/reset/initialize for a (new) round of formation position calculations
	// for a formation around the ship given as parameter.
	void Step();
	
	// Get the formation position for the ship given as parameter. If a given ship is
	// not participating yet, then it will be added.
	Point Position(const Ship *ship);


private:
	// Re-generate the list of (relative) positions for the ships in the formation.
	void CalculatePositions();
	
	// Calculate the direction the formation is facing.
	void CalculateDirection();

	// Check if a ship is actually still participating in the current formation(ring).
	bool IsActiveInFormation(unsigned int ring, const Ship *ship) const;

	// Remove a ship from the formation-ring (based on its index). The last ship
	// in the ring will take the position of the removed ship (if the removed
	// ship itself is not the last ship).
	void RemoveFromRing(unsigned int ring, unsigned int index);


private:
	// Lists of ships on the rings. Used when (re)generating positions for the ring.
	// The actual positions are stored in shipPositions.
	std::map<unsigned int, std::vector<std::weak_ptr<const Ship>>> ringShips;
	// Lookup/cache of the ship coordinates in the formation, its ring-section and
	// an indicator if it was seen since last generate loop.
	std::map<const Ship *, std::pair<Point, bool>> shipPositions;
	
	// Timer that controls the (re)generation of ship positions.
	int positionsTimer = 0;
	
	// The scaling factors as we currently have for this formation and
	// the scaling factors that we are preparing in the currently running
	// iteration for the next iteration.
	FormationPattern::ActiveFormation activeData;
	FormationPattern::ActiveFormation nextActiveData;
	
	// The body around which the formation will be formed and the pattern to follow.
	const Body *formationLead;
	const FormationPattern *pattern;
	
	// The formation facing direction.
	Angle direction;
	
	// Settings for flipping/mirroring of the pattern.
	bool flippedX = false;
	bool flippedY = false;
	
	// Status variable used to track if ships still participate in the formation.
	bool tickTock = true;
};



#endif
