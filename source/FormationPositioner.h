/* FormationPositioner.h
Copyright (c) 2019 by Peter van der Meer

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef FORMATION_POSITIONER_H_
#define FORMATION_POSITIONER_H_

#include "Body.h"
#include "FormationPattern.h"

#include <map>

class Ship;



// Represents an active formation for a set of spaceships. Assigns each ship
// to a position (Point) in the formation.
class FormationPositioner{
public:
	// Initializer based on the formation pattern to follow.
	FormationPositioner(const Body * formationLead, const FormationPattern * pattern): formationLead(formationLead), pattern(pattern) {}
	
	// Start/reset/initialize for a (new) round of formation position calculations
	// for a formation around the ship given as parameter.
	void Start();
	
	// Get the point for the next ship in the formation. Caller should ensure
	// that the ships are offered in the right order to the calculator.
	Point NextPosition(const Ship* ship);


private:
	class RingPositioner{
		public:
			// The scaling factors being used for this ring.
			double activeScalingFactor = 1.;
			double nextScalingFactor = 1.;

			// Values used during ship position calculation iterations.
			int ring = 0;
			int activeLine = 0;
			int lineSlot = 0;
			int lineSlots = -1;
	};


private:
	// The actual positioners based on the desired ring-numbers.
	std::map<int, RingPositioner> ringPos;

	// The body around which the formation will be formed and the pattern to follow.
	const Body * formationLead;
	const FormationPattern * pattern;
	
	// The formation facing direction.
	Angle direction;
	
	// Symmetry correction settings.
	// The amount of rotation applied to the current positioner.
	bool mirroredTransverse = false;
	bool mirroredLongitudinal = false;
};



#endif
