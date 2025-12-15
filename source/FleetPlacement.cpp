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
#include "Date.h"
#include "Random.h"
#include "Ship.h"

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
			if(position.has_value() || orbit.has_value())
			{
				position.reset();
				orbit.reset();
				child.PrintTrace("distance, orbit, and position nodes are mutually exclusive. Using the distance.");
			}
			distance = max(0., child.Value(1));
			if(child.Size() >= 3)
				angle = Angle(child.Value(2));
		}
		else if(key == "orbit" && child.Size() >= 3)
		{
			if(position.has_value() || distance.has_value())
			{
				position.reset();
				distance.reset();
				angle.reset();
				child.PrintTrace("distance, orbit, and position nodes are mutually exclusive. Using the orbit.");
			}
			orbit = Orbit(max(0., child.Value(1)), max(0., child.Value(2)),
				child.Size() >= 4 ? child.Value(3) : 0.);
		}
		else if(key == "position" && child.Size() >= 3)
		{
			if(distance.has_value() || orbit.has_value())
			{
				distance.reset();
				angle.reset();
				orbit.reset();
				child.PrintTrace("distance, orbit, and position nodes are mutually exclusive. Using the position.");
			}
			position = Point(child.Value(1), child.Value(2));
		}
		else if(key == "velocity" && child.Size() >= 3)
			velocity = child.Value(1) * Angle(child.Value(2)).Unit();
		else if(key == "spread")
			spread = max(0., child.Value(1));
		else
			child.PrintTrace("Skipping unrecognized attribute:");
	}
}



void FleetPlacement::Place(const std::list<std::shared_ptr<Ship>> &ships, const Date &date, bool isEntering) const
{
	if(!loaded)
		return;
	optional<Point> center;
	if(position.has_value())
		center = *position;
	else if(distance.has_value())
		center = *distance * angle.value_or(Angle::Random()).Unit();
	else if(orbit.has_value())
		center = orbit->Position(date.DaysSinceEpoch()).first;

	bool first = true;
	DamageProfile damage = DamageProfile(weapon);
	for(auto &ship : ships)
	{
		// Deal damage to these ships if a weapon was loaded.
		if(weapon.IsLoaded())
		{
			ship->TakeDamage(damage.CalculateDamage(*ship), nullptr);
			ship->SetSkipRecharging();
		}
		// Skip over ships that are landed or that don't have a system.
		// Skip NPCs with the "entering" personality, as these ships are jumping into the system.
		if(ship->GetPlanet() || !ship->GetSystem() || isEntering)
			continue;
		// Place these ships at a particular location in the system.
		if(center.has_value())
		{
			ship->SetIsPlaced();
			// The first ship gets placed exactly at the center of the placement location.
			// All other ships are randomly spread around that point.
			if(first)
			{
				ship->SetPosition(*center);
				first = false;
			}
			else
				ship->SetPosition(*center + Angle::Random().Unit() * Random::Real() * spread);
			// Set the velocity of placed ships to 0, as otherwise they can get flung out of formation quicker than
			// the player can realize they were even intentionally placed.
			ship->SetVelocity(Point());
		}
		// Set the velocity of these ships.
		if(velocity.has_value())
		{
			ship->SetIsPlaced();
			ship->SetVelocity(*velocity);
			ship->SetFacing(Angle(*velocity));
		}
	}
}
