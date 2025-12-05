/* FollowShipCamera.cpp
Copyright (c) 2024 by the Endless Sky developers

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "FollowShipCamera.h"

#include "Random.h"
#include "Ship.h"

using namespace std;

const string FollowShipCamera::name = "Follow Ship";



FollowShipCamera::FollowShipCamera()
	: lastPosition(0., 0.), lastVelocity(0., 0.)
{
}



Point FollowShipCamera::GetTarget() const
{
	auto ship = target.lock();
	if(ship && ship->GetSystem())
		return ship->Center();
	return lastPosition;
}



Point FollowShipCamera::GetVelocity() const
{
	auto ship = target.lock();
	if(ship && ship->GetSystem())
		return ship->Velocity();
	return lastVelocity;
}



void FollowShipCamera::Step()
{
	auto ship = target.lock();
	if(ship && ship->GetSystem())
	{
		lastPosition = ship->Center();
		lastVelocity = ship->Velocity();
	}
	else if(!ships.empty())
	{
		// Target lost, select a new one
		SelectRandom();
	}
}



void FollowShipCamera::SetShips(const list<shared_ptr<Ship>> &newShips)
{
	ships = newShips;

	// If we have no target, select one
	if(target.expired() && !ships.empty())
		SelectRandom();
}



const string &FollowShipCamera::ModeName() const
{
	return name;
}



string FollowShipCamera::TargetName() const
{
	auto ship = target.lock();
	if(ship)
		return ship->GivenName();
	return "";
}



void FollowShipCamera::CycleTarget()
{
	if(ships.empty())
		return;

	auto current = target.lock();
	bool foundCurrent = false;

	for(auto it = ships.begin(); it != ships.end(); ++it)
	{
		if(foundCurrent && (*it)->IsTargetable())
		{
			target = *it;
			return;
		}
		if(*it == current)
			foundCurrent = true;
	}

	// Wrap around to first targetable ship
	for(auto &ship : ships)
	{
		if(ship->IsTargetable())
		{
			target = ship;
			return;
		}
	}
}



void FollowShipCamera::SelectRandom()
{
	if(ships.empty())
		return;

	// Build list of targetable ships
	vector<shared_ptr<Ship>> targetable;
	for(auto &ship : ships)
		if(ship->IsTargetable())
			targetable.push_back(ship);

	if(!targetable.empty())
		target = targetable[Random::Int(targetable.size())];
}
