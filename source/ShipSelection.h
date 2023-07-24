/* ShipSelection.h
Copyright (c) 2023 by Dave Flowers

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef SHIP_SELECTION_H_
#define SHIP_SELECTION_H_

#include <set>

class PlayerInfo;
class Ship;



// Handle ship selection.
class ShipSelection {
public:
	explicit ShipSelection(PlayerInfo &player);

	bool Has(Ship *ship) const;
	bool HasMany() const;

	// Return ship count away from current selection.
	Ship *Find(int count);
	// Select based on ship and key modifiers.
	void Select(Ship *ship);
	void Set(Ship *ship);
	// Clear selection and select first ship available.
	void Reset();

	void SetGroup(int group) const;
	void SelectGroup(int group, bool modifySelection);

	Ship *Selected() const;
	const std::set<Ship *> &AllSelected() const;


private:
	PlayerInfo &player;

	Ship *selectedShip = nullptr;
	std::set<Ship *> allSelected;
};



#endif
