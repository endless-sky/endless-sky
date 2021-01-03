/* InfoPanelState.cpp
Copyright (c) 2020 by Eric Xing

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "InfoPanelState.h"

#include "PlayerInfo.h"

using namespace std;



InfoPanelState::InfoPanelState(PlayerInfo &player)
	:ships(player.Ships()), canEdit(player.GetPlanet())
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



void InfoPanelState::SetSelected(const std::set<int> &selected)
{
	allSelected = selected;
	if(!selected.empty())
		selectedIndex = *selected.begin();
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



const std::set<int> &InfoPanelState::AllSelected() const
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



InfoPanelState::ShipComparator *InfoPanelState::CurrentSort() const
{
	return currentSort;
}



void InfoPanelState::SetCurrentSort(ShipComparator *newSort)
{
	currentSort = newSort;
}
