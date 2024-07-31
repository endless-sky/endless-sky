/* FormationPositioner.h
Copyright (c) 2019-2022 by Peter van der Meer

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include "Angle.h"

#include <map>
#include <memory>
#include <vector>

class Body;
class FormationPattern;
class Ship;



// Represents an active formation for a set of spaceships. Assigns each ship
// to a position (Point) in the formation.
class FormationPositioner {
public:
	// Initializer based on the formation pattern to follow.
	FormationPositioner(const Body *formationLead, const FormationPattern *pattern);

	// Start/reset/initialize for a (new) round of formation position calculations.
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
	bool IsActiveInFormation(const Ship *ship) const;

	// Remove a ship from the formation (based on its index). The last ship
	// in the formation will take the position of the removed ship (if the removed
	// ship itself is not the last ship).
	void Remove(unsigned int index);


private:
	// Lists of ships on the rings. Used when (re)generating positions for the ring.
	// The actual positions are stored in shipPositions.
	std::vector<std::weak_ptr<const Ship>> shipsInFormation;
	// Lookup/cache of the ship coordinates in the formation, its ring-section and
	// an indicator if it was seen since last generate loop.
	std::map<const Ship *, std::pair<Point, bool>> shipPositions;

	// Timer that controls the (re)generation of ship positions.
	int positionsTimer = 0;

	// The scaling factor as we currently have for this formation for the ship or
	// other body around which this formation is formed.
	double centerBodyRadius = 150;

	// The body around which the formation will be formed and the pattern to follow.
	const Body *formationLead;
	const FormationPattern *pattern;

	// The formation facing direction.
	Angle direction;

	// Settings for flipping/mirroring of the pattern.
	bool flippedX = false;
	bool flippedY = false;

	// Status variable used to track if ships still participate in the formation.
	// TODO: This method of tracking shouldn't be needed; ships themselves should have a state to show what they are doing.
	bool tickTock = true;
};
