/* FireCommand.h
Copyright (c) 2021 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef FIRE_COMMAND_H_
#define FIRE_COMMAND_H_

#include "Bitset.h"

#include <cstdint>
#include <vector>



// Class mapping key presses to specific commands / actions. The player can
// change the mappings for most of these keys in the preferences panel.
// A single Command object can represent multiple individual commands, e.g.
// everything the AI wants a ship to do, or all keys the player is holding down.
class FireCommand {
public:
	// Sets the specified amount of hardpoints desired.
	void SetHardpoints(size_t count);

	// Assigns the subset of other to this class that is no larger than
	// this command's hardpoint size.
	void AssignSubsetOf(const FireCommand &other);

	// Reset this to an empty command.
	void Clear();

	// Get or set the fire commands.
	bool HasFire(int index) const;
	void SetFire(int index);
	// Check if any weapons are firing.
	bool IsFiring() const;
	// Set the turn rate of the turret with the given weapon index. A value of
	// -1 or 1 means to turn at the full speed the turret is capable of.
	double Aim(int index) const;
	void SetAim(int index, double amount);


private:
	// The weapon commands stores whether the given weapon is active.
	Bitset weapon;
	// Turret turn rates, reduced to 8 bits to save space.
	std::vector<signed char> aim;
};



#endif
