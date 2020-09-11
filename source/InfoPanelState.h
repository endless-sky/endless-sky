/* InfoPanelState.h
Copyright (c) 2020 by Eric Xing

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef INFO_PANEL_STATE_H_
#define INFO_PANEL_STATE_H_

#include "Ship.h"

#include <memory>
#include <set>
#include <vector>

class PlayerInfo;



// This class is responsible for storing the state between PlayerInfoPanel and ShipInfoPanel
// so that things like scroll position, selection and sort are saved when the user is switching between the panels
class InfoPanelState {
public:
	using ShipComparator = bool (const std::shared_ptr <Ship>&, const std::shared_ptr <Ship>&);

	InfoPanelState(PlayerInfo &player);

	int SelectedIndex();
	void SetSelectedIndex(int);

	std::set<int> &AllSelected();

	bool CanEdit();

	int Scroll();
	void SetScroll(int);

	std::vector<std::shared_ptr<Ship>> &Ships();

	ShipComparator* CurrentSort();
	void SetCurrentSort(ShipComparator* s);


private:
	// Most recent selected ship index
	int selectedIndex = -1;

	// Indices of selected ships
	std::set<int> allSelected;

	// When the player is landed, they are able to change their flagship and reorder their fleet.
	bool canEdit = false;

	// Index of the ship at the top of the fleet listing.
	int scroll = 0;

	// A copy of PlayerInfo.ships for viewing and manipulating
	std::vector<std::shared_ptr<Ship>> ships;

	ShipComparator* currentSort = nullptr;
};



#endif
