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
#include "PlayerInfoPanel.h"

#include <memory>
#include <set>
#include <vector>

using namespace std;

class PlayerInfo;
class Ship;
class InfoPanelState;



InfoPanelState::InfoPanelState(PlayerInfo &player)
:ships(player.Ships()), canEdit(player.GetPlanet())
{
	
}


int InfoPanelState::SelectedIndex()
{
	return selectedIndex;
}

void InfoPanelState::SetSelectedIndex(int newSelectedIndex)
{
	selectedIndex = newSelectedIndex;
}



set<int> &InfoPanelState::AllSelected()
{
	return allSelected;
}



bool InfoPanelState::CanEdit()
{
	return canEdit;
}


int InfoPanelState::Scroll()
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



InfoPanelState::ShipComparator* InfoPanelState::CurrentSort()
{
	return currentSort;
}

void InfoPanelState::SetCurrentSort(ShipComparator* newSort)
{
	currentSort = newSort;
}
