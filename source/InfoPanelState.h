/* InfoPanelState.h
Copyright (c) 2020 by Eric Xing

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

#include <memory>
#include <set>
#include <vector>

class PlayerInfo;
class Ship;



// This class is responsible for storing the state
// between PlayerInfoPanel and ShipInfoPanel so that
// things like scroll position, selection and sort are
// saved when the user is switching between the panels.
class InfoPanelState {
public:
	using ShipComparator = bool(const std::shared_ptr<Ship> &, const std::shared_ptr<Ship> &);

	explicit InfoPanelState(PlayerInfo &player);

	int SelectedIndex() const;
	void SetSelectedIndex(int newSelectedIndex);

	const std::set<int> &AllSelected() const;
	void SetSelected(std::set<int> selected);
	void Select(int index);
	void SelectOnly(int index);
	void SelectMany(int start, int end);
	bool Deselect(int index);
	void DeselectAll();
	void Disown(std::vector<std::shared_ptr<Ship>>::const_iterator it);

	bool CanEdit() const;

	int Scroll() const;
	void SetScroll(int newScroll);

	std::vector<std::shared_ptr<Ship>> &Ships();
	const std::vector<std::shared_ptr<Ship>> &Ships() const;
	bool ReorderShipsTo(int toIndex);

	ShipComparator *CurrentSort() const;
	void SetCurrentSort(ShipComparator *s);


private:
	bool ReorderShips(const std::set<int> &fromIndices, int toIndex);


private:
	PlayerInfo &player;

	// Most recent selected ship index.
	int selectedIndex = -1;

	// Indices of selected ships.
	std::set<int> allSelected;

	// A copy of PlayerInfo.ships for viewing and manipulating.
	std::vector<std::shared_ptr<Ship>> ships;

	// When the player is landed, they are able to
	// change their flagship and reorder their fleet.
	const bool canEdit = false;

	// Index of the ship at the top of the fleet listing.
	int scroll = 0;

	// Keep track of whether the ships are sorted.
	ShipComparator *currentSort = nullptr;
};
