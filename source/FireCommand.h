/* FireCommand.h
Copyright (c) 2021 by Michael Zahniser

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

#include "Bitset.h"

#include <cstdint>
#include <vector>



// Class that represents firing commands from ships, which includes
// whether a weapon is currently firing and its turn (if any).
class FireCommand {
public:
	// Sets the specified amount of hardpoints desired.
	void SetHardpoints(size_t count);

	// Assigns the subset of other to this class that is no larger than
	// this command's hardpoint size.
	void UpdateWith(const FireCommand &other) noexcept;

	// Reset this to an empty command.
	void Clear();

	// Get or set the fire commands.
	bool HasFire(int index) const noexcept;
	void SetFire(int index) noexcept;
	// Check if any weapons are firing.
	bool IsFiring() const noexcept;
	// Gets the current turn rate of the turret at the given weapon index.
	double Aim(int index) const noexcept;
	// Set the turn rate of the turret with the given weapon index. A value of
	// -1 or 1 means to turn at the full speed the turret is capable of.
	void SetAim(int index, double amount) noexcept;


private:
	bool IsIndexValid(int index) const noexcept;


private:
	// The weapon commands stores whether the given weapon is active.
	Bitset weapon;
	// Turret turn rates, reduced to 8 bits to save space.
	std::vector<signed char> aim;
};
