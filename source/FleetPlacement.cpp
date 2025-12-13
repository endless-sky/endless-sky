/* FleetPlacement.cpp
Copyright (c) 2025 by Amazinite

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "FleetPlacement.h"

#include "DamageDealt.h"
#include "DamageProfile.h"
#include "DataNode.h"
#include "Ship.h"
#include "Visual.h"

#include <algorithm>


using namespace std;



void FleetPlacement::Load(const DataNode &node)
{
	loaded = true;
	for(const DataNode &child : node)
	{
		const string &key = child.Token(0);
		if(key == "weapon" && child.HasChildren())
			weapon.Load(child);
		else if(child.Size() < 2)
			child.PrintTrace("Expected key to have a value:");
		else if(key == "distance")
		{
			if(setPosition)
			{
				setPosition = false;
				child.PrintTrace("\"placement\" nodes cannot have both a distance and a position. "
					"Using the distance.");
			}
			setDistance = true;
			distance = max(0., child.Value(1));
		}
		else if(key == "position" && child.Size() >= 3)
		{
			if(setDistance)
			{
				setDistance = false;
				child.PrintTrace("\"placement\" nodes cannot have both a distance and a position. "
					"Using the position.");
			}
			setPosition = true;
			position = Point(child.Value(1), child.Value(2));
		}
		else if(key == "spread")
			spread = max(0., child.Value(1));
		else
			child.PrintTrace("Skipping unrecognized attribute:");
	}
}



void FleetPlacement::Place(const std::list<std::shared_ptr<Ship>> &ships, bool isEntering) const
{
	if(!loaded)
		return;
	Point center = setPosition ? position : (setDistance ? Angle::Random().Unit() * distance : Point());
	DamageProfile damage = DamageProfile(weapon);
	// Weapons loaded by FleetPlacement don't support creating visuals, but we still need a dummy vector to pass
	// to Ship::TakeDamage.
	vector<Visual> visuals;
	bool first = true;
	for(auto &ship : ships)
	{
		// Deal damage to these ships if a weapon was loaded.
		if(weapon.IsLoaded())
			ship->TakeDamage(visuals, damage.CalculateDamage(*ship), nullptr);
		// Place these ships at a particular location in the system.
		// Skip over ships that are landed or that don't have a system.
		// Also skip NPCs with the "entering" personality, as these ships are jumping into the system.
		if(!isEntering && (setPosition || setDistance) && !ship->GetPlanet() && ship->GetSystem())
		{
			// The first ship gets placed exactly at the center of the placement location.
			// All other ships are randomly spread around that point.
			if(first)
			{
				ship->SetPosition(center);
				first = false;
			}
			else
				ship->SetPosition(center + Angle::Random().Unit() * spread);
		}
	}
}
