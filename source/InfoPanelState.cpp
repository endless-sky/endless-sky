/* InfoPanelState.cpp
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

#include "InfoPanelState.h"

#include "PlayerInfo.h"

using namespace std;



InfoPanelState::InfoPanelState(PlayerInfo &player)
	: player(player), ships(player.Ships()), canEdit(player.GetPlanet()), visibleColumns({
		"ship",
		"model",
		"system",
		"shields",
		"hull",
		"fuel",
		"crew",
	})
{
}



int InfoPanelState::SelectedIndex() const
{
	return allSelected.empty() ? -1 : selectedIndex;
}



void InfoPanelState::SetSelectedIndex(int newSelectedIndex)
{
	selectedIndex = newSelectedIndex;
	allSelected.insert(newSelectedIndex);
}



void InfoPanelState::SetSelected(set<int> selected)
{
	allSelected = std::move(selected);
	if(!allSelected.empty())
		selectedIndex = *allSelected.begin();
}



void InfoPanelState::Select(int index)
{
	allSelected.insert(index);
	if(selectedIndex == -1)
		selectedIndex = index;
}



void InfoPanelState::SelectOnly(int index)
{
	allSelected.clear();
	SetSelectedIndex(index);
}



void InfoPanelState::SelectMany(int start, int end)
{
	for(int i = start; i < end; ++i)
		allSelected.insert(i);

	if(selectedIndex == -1)
		selectedIndex = *allSelected.begin();
}



bool InfoPanelState::Deselect(int index)
{
	bool erased = allSelected.erase(index);
	// Select the closest ship to this.
	if(index == selectedIndex && !allSelected.empty())
	{
		const auto &ind = allSelected.upper_bound(index);
		selectedIndex = ind == allSelected.end() ? *allSelected.rbegin() : *ind;
	}
	return erased;
}



void InfoPanelState::DeselectAll()
{
	allSelected.clear();
	selectedIndex = -1;
}



void InfoPanelState::Disown(vector<shared_ptr<Ship>>::const_iterator it)
{
	ships.erase(it);
}



const set<int> &InfoPanelState::AllSelected() const
{
	return allSelected;
}



bool InfoPanelState::CanEdit() const
{
	return canEdit;
}



int InfoPanelState::Scroll() const
{
	return scroll;
}



void InfoPanelState::SetScroll(int newScroll)
{
	scroll = newScroll;
}



vector<shared_ptr<Ship>> &InfoPanelState::Ships()
{
	return ships;
}



const vector<shared_ptr<Ship>> &InfoPanelState::Ships() const
{
	return ships;
}



bool InfoPanelState::ReorderShipsTo(int toIndex)
{
	bool success = ReorderShips(allSelected, toIndex);
	if(success)
		player.SetShipOrder(ships);
	return success;
}



// If the move accesses invalid indices, no moves are done.
bool InfoPanelState::ReorderShips(const set<int> &fromIndices, int toIndex)
{
	if(fromIndices.empty() || static_cast<unsigned>(toIndex) >= ships.size())
		return false;

	// When shifting ships up in the list, move to the desired index. If
	// moving down, move after the selected index.
	int direction = (*fromIndices.begin() < toIndex) ? 1 : 0;

	// Remove the ships from last to first, so that each removal leaves all the
	// remaining indices in the set still valid.
	vector<shared_ptr<Ship>> removed;
	for(set<int>::const_iterator it = fromIndices.end(); it != fromIndices.begin(); )
	{
		// The "it" pointer doesn't point to the beginning of the list, so it is
		// safe to decrement it here.
		--it;

		// Bail out if any invalid indices are encountered.
		if(static_cast<unsigned>(*it) >= ships.size())
			return false;

		removed.insert(removed.begin(), ships[*it]);
		ships.erase(ships.begin() + *it);
		// If this index is before the insertion point, removing it causes the
		// insertion point to shift back one space.
		if(*it < toIndex)
			--toIndex;
	}
	// Make sure the insertion index is within the list.
	toIndex = min<int>(toIndex + direction, ships.size());
	ships.insert(ships.begin() + toIndex, removed.begin(), removed.end());

	// Change the selected indices so they still refer to the block of ships
	// that just got moved.
	int lastIndex = toIndex + allSelected.size();
	DeselectAll();
	SelectMany(toIndex, lastIndex);

	// The ships are no longer sorted.
	SetCurrentSort(nullptr);
	return true;
}



InfoPanelState::ShipComparator *InfoPanelState::CurrentSort() const
{
	return currentSort;
}



void InfoPanelState::SetCurrentSort(ShipComparator *newSort)
{
	currentSort = newSort;
}



set<const string> InfoPanelState::VisibleColumns() const
{
	return visibleColumns;
}



void InfoPanelState::ShowColumn(const string key)
{
	visibleColumns.insert(key);
}



void InfoPanelState::HideColumn(const string key)
{
	visibleColumns.erase(key);
}



void InfoPanelState::ToggleColumn(const string key)
{
	visibleColumns.find(key) == visibleColumns.end() ?
		ShowColumn(key) : HideColumn(key);
}
