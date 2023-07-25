/* ShipSelection.cpp
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

#include "ShipSelection.h"

#include "PlayerInfo.h"
#include "Planet.h"
#include "Ship.h"

#include <SDL2/SDL.h>

using namespace std;

namespace {
	bool CanShowInSidebar(const Ship &ship, const Planet *here)
	{
		return ship.GetPlanet() == here;
	}
}



ShipSelection::ShipSelection(PlayerInfo &player)
	: player(player), selectedShip(player.Flagship())
{
	if(selectedShip)
		allSelected.insert(selectedShip);
}



bool ShipSelection::Has(Ship *ship) const
{
	return allSelected.count(ship);
}



bool ShipSelection::HasMany() const
{
	return allSelected.size() > 1;
}



Ship *ShipSelection::Find(int count)
{
	if(!selectedShip)
	{
		Set(player.Flagship());
		return nullptr;
	}

	// Find the currently selected ship in the list.
	vector<shared_ptr<Ship>>::const_iterator it = player.Ships().begin();
	for( ; it != player.Ships().end(); ++it)
		if(it->get() == selectedShip)
			break;

	const Planet *here = player.GetPlanet();
	// When counting, wrap the ship list so you can go around the ends.
	if(count < 0)
	{
		while(count)
		{
			if(it == player.Ships().begin())
				it = player.Ships().end();
			--it;

			if(CanShowInSidebar(**it, here))
				++count;
		}
	}
	else
	{
		while(count)
		{
			++it;
			if(it == player.Ships().end())
				it = player.Ships().begin();

			if(CanShowInSidebar(**it, here))
				--count;
		}
	}

	return &**it;
}



void ShipSelection::Select(Ship *ship)
{
	const Uint16 mod = SDL_GetModState();
	const bool shift = mod & KMOD_SHIFT;
	const bool control = mod & (KMOD_CTRL | KMOD_GUI);

	// Only select a range if we have both endpoints.
	if(shift && selectedShip)
	{
		bool started = false;
		const Planet *here = player.GetPlanet();
		for(const shared_ptr<Ship> &other : player.Ships())
		{
			// Skip any ships that are "absent" for whatever reason.
			if(!CanShowInSidebar(*other, here))
				continue;

			if(other.get() == ship || other.get() == selectedShip)
			{
				if(started)
					break;
				else
					started = true;
			}
			else if(started)
				allSelected.insert(other.get());
		}
	}
	else if(!control)
		allSelected.clear();
	// If control is down, try and deselect the ship (otherwise we're selecting it).
	else if(allSelected.erase(ship))
	{
		if(selectedShip == ship)
			selectedShip = allSelected.empty() ? nullptr : *allSelected.begin();
		return;
	}

	selectedShip = ship;
	allSelected.insert(ship);
}



void ShipSelection::Set(Ship *ship)
{
	allSelected.clear();
	selectedShip = ship;
	if(ship)
		allSelected.insert(ship);
}



void ShipSelection::Reset()
{
	Ship *newShip = nullptr;
	const Planet *here = player.GetPlanet();
	for(const shared_ptr<Ship> &ship : player.Ships())
		if(CanShowInSidebar(*ship, here))
		{
			newShip = &*ship;
			break;
		}
	Set(newShip);
}



void ShipSelection::SetGroup(const int group) const
{
	player.SetGroup(group, &allSelected);
}



void ShipSelection::SelectGroup(const int group, bool modifySelection)
{
	const Planet *here = player.GetPlanet();
	const set<Ship *> groupShips = player.GetGroup(group);

	if(modifySelection)
		for(Ship *ship : groupShips)
			modifySelection &= allSelected.erase(ship) || !CanShowInSidebar(*ship, here);
	else
		allSelected.clear();

	// If modifying and every displayable ship in this group was removed, don't reselect them.
	if(!modifySelection)
		for(Ship *ship : groupShips)
			if(CanShowInSidebar(*ship, here))
				allSelected.insert(ship);

	if(!Has(selectedShip))
		selectedShip = allSelected.empty() ? nullptr : *allSelected.begin();
}



Ship *ShipSelection::Selected() const
{
	return selectedShip;
}



const std::set<Ship *> &ShipSelection::AllSelected() const
{
	return allSelected;
}
