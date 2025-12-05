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



// Check if the target ship is still valid to follow
bool FollowShipCamera::HasValidTarget() const
{
	auto ship = target.lock();
	if(!ship || !ship->GetSystem())
		return false;
	// Stop following if ship has entered hyperspace and moved too far
	// (prevents camera from flying off into space)
	if(ship->IsHyperspacing())
	{
		double distance = (ship->Center() - lastPosition).Length();
		// If ship moved more than ~2000 pixels from last stable position, stop following
		if(distance > 2000.)
			return false;
	}
	return true;
}



Point FollowShipCamera::GetTarget() const
{
	// Return the smoothed position for cinematic drift effect
	if(hasSmoothedPosition)
		return smoothedPosition;
	return lastPosition;
}



Point FollowShipCamera::GetVelocity() const
{
	auto ship = target.lock();
	if(ship && ship->GetSystem())
	{
		// If ship is hyperspacing and too far, return zero velocity
		if(ship->IsHyperspacing())
		{
			double distance = (ship->Center() - lastPosition).Length();
			if(distance > 2000.)
				return Point();
		}
		return ship->Velocity();
	}
	// When no valid target, return zero velocity to stop camera drift
	return Point();
}



void FollowShipCamera::Step()
{
	// Decrement cooldown
	if(switchCooldown > 0)
		--switchCooldown;

	// Check if current target is still valid to follow
	if(HasValidTarget())
	{
		auto ship = target.lock();
		// Only update lastPosition when ship is not hyperspacing
		// This keeps lastPosition as the "stable" position before jump
		if(!ship->IsHyperspacing())
		{
			lastPosition = ship->Center();
			lastVelocity = ship->Velocity();
		}

		// Apply cinematic drift: smoothly interpolate camera toward target
		// Using a lerp factor of 0.08 gives a nice "lag" effect
		Point targetPos = ship->IsHyperspacing() ? lastPosition : ship->Center();
		if(!hasSmoothedPosition)
		{
			// First frame: snap to target
			smoothedPosition = targetPos;
			hasSmoothedPosition = true;
		}
		else
		{
			// Interpolate toward target position (cinematic drift)
			// Lower values = more lag/drift, higher = snappier
			constexpr double lerpFactor = 0.08;
			smoothedPosition = smoothedPosition + (targetPos - smoothedPosition) * lerpFactor;
		}
		return;
	}

	// Target is invalid (gone, too far, or hyperspacing beyond threshold)
	// Try to find a new one immediately (no cooldown for first attempt)
	if(switchCooldown == 0)
	{
		// Clear the old target before selecting new one
		target.reset();
		SelectRandom();
		// Set cooldown to prevent rapid switching if selection fails
		switchCooldown = 30;
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
	// Only show name if we have a valid target we're actually following
	if(!HasValidTarget())
		return "";
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
		if(foundCurrent && IsValidTarget(*it))
		{
			target = *it;
			return;
		}
		if(*it == current)
			foundCurrent = true;
	}

	// Wrap around to first valid ship
	for(auto &ship : ships)
	{
		if(IsValidTarget(ship))
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

	// Build list of valid ships (targetable, in system, not jumping)
	vector<shared_ptr<Ship>> candidates;
	for(auto &ship : ships)
		if(IsValidTarget(ship))
			candidates.push_back(ship);

	if(!candidates.empty())
		target = candidates[Random::Int(candidates.size())];
	// If no valid candidates, keep current target (even if invalid)
	// to prevent rapid cycling. Camera will use lastPosition.
}



bool FollowShipCamera::IsValidTarget(const shared_ptr<Ship> &ship) const
{
	if(!ship)
		return false;
	// Must be targetable (visible, not cloaked, etc.)
	if(!ship->IsTargetable())
		return false;
	// Must be in the system
	if(!ship->GetSystem())
		return false;
	// Exclude ships that are entering hyperspace or already hyperspacing
	if(ship->IsEnteringHyperspace() || ship->IsHyperspacing())
		return false;
	return true;
}



shared_ptr<Ship> FollowShipCamera::GetObservedShip() const
{
	if(HasValidTarget())
		return target.lock();
	return nullptr;
}
