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

#include "Angle.h"
#include "FormationPattern.h"
#include "Point.h"
#include "Ship.h"

#include <map>
#include <vector>



// Represents an active formation for a set of spaceships. Assigns each ship
// to a position (Point) in the formation.
class FormationPositioner{
public:
	// Initializer based on the formation pattern to follow
	FormationPositioner();
	
	// Start/reset/initialize for a (new) round of formation position calculations
	// for a formation around the ship given as parameter.
	void Start(Ship &ship);
	
	// Get the point for the next ship in the formation. Caller should ensure
	// that the ships are offered in the right order to the calculator.
	// Ship pointer is used to get the scaling factor (in the next round of
	// position calculations).
	Point NextPosition(Ship &ship);
	
	
private:
	class Iter {
	public:
		Iter(const FormationPattern &pattern);

		Point NextPosition(Ship &ship);

	public:
		const FormationPattern &pattern;
		
		// The scaling factor currently being used.
		double activeScalingFactor = 80.;
		// The scaling factor that is to be used for the next round.
		double nextScalingFactor = 0.;
		
		// Values used during ship position calculation iterations.
		int iteration = 0;
		int activeLine = 0;
		int posOnLine = 0;
		int positionsOnLine = 0;
	};
	
	
private:
	// Anchor position and direction of the formation(s) being positioned.
	Point anchor;
	Angle direction;
	
	// Formation patterns that this positioner should assign ships to.
	std::map<const FormationPattern *, Iter> patterns;
};



#endif
