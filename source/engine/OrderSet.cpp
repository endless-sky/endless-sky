/* OrderSet.cpp
Copyright (c) 2024 by TomGoodIdea

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "OrderSet.h"

#include "../Ship.h"

#include <array>

using namespace std;

namespace {
	constexpr bitset<Orders::TYPES_COUNT> HAS_TARGET_SHIP{
		(1 << Orders::KEEP_STATION) +
		(1 << Orders::GATHER) +
		(1 << Orders::FINISH_OFF)
	};

	constexpr bitset<Orders::TYPES_COUNT> HAS_TARGET_ASTEROID{
		(1 << Orders::MINE)
	};

	constexpr bitset<Orders::TYPES_COUNT> HAS_TARGET_SHIP_OR_ASTEROID{
		(1 << Orders::ATTACK)
	};

	constexpr bitset<Orders::TYPES_COUNT> HAS_TARGET_LOCATION{
		(1 << Orders::MOVE_TO)
	};

	// Orders not included in the bitset should be removed when the given order is issued.
	constexpr array<bitset<Orders::TYPES_COUNT>, Orders::TYPES_COUNT> SIMULTANEOUS{{
		{}, // HOLD_POSITION
		{}, // HOLD_ACTIVE
		{}, // MOVE_TO
		{}, // KEEP_STATION
		{}, // GATHER
		{}, // ATTACK
		{}, // FINISH_OFF
		{}, // MINE
		{}, // HARVEST
	}};
}



void OrderSet::Set(Types type) noexcept
{
	types &= SIMULTANEOUS[type];
	types.set(type);
}



void OrderSet::Reset(Types type) noexcept
{
	types.reset(type);
}



bool OrderSet::Has(Types type) const noexcept
{
	return types[type];
}



bool OrderSet::Empty() const noexcept
{
	return types.none();
}



void OrderSet::Add(const OrderSingle &newOrder, bool &hasMismatch, bool &alreadyHarvesting)
{
	// HOLD_ACTIVE cannot be given as manual order, but we make sure here
	// that any HOLD_ACTIVE order also matches when an HOLD_POSITION
	// command is given.
	if(Has(HOLD_ACTIVE))
		Set(HOLD_POSITION);

	shared_ptr<Ship> newTargetShip = newOrder.GetTargetShip();
	shared_ptr<Minable> newTargetAsteroid = newOrder.GetTargetAsteroid();
	hasMismatch |= !Has(newOrder.type)
		|| GetTargetShip() != newTargetShip
		|| GetTargetAsteroid() != newTargetAsteroid;

	if(hasMismatch)
	{
		Set(newOrder.type);
		if(newTargetAsteroid)
			alreadyHarvesting = Has(HARVEST) && newOrder.type == HARVEST;
	}
	else
	{
		// The new order is already in the old set, so it should be removed instead.
		Reset(newOrder.type);
		return;
	}

	// Update target ship and/or asteroid if it's relevant for the new order.
	if(HAS_TARGET_SHIP[newOrder.type] || HAS_TARGET_SHIP_OR_ASTEROID[newOrder.type])
		SetTargetShip(newTargetShip);
	if(HAS_TARGET_ASTEROID[newOrder.type] || HAS_TARGET_SHIP_OR_ASTEROID[newOrder.type])
		SetTargetAsteroid(newTargetAsteroid);

	// Update target system and point if it's relevant for the new order.
	if(HAS_TARGET_LOCATION[newOrder.type])
	{
		SetTargetPoint(newOrder.GetTargetPoint());
		SetTargetSystem(newOrder.GetTargetSystem());
	}
}



void OrderSet::Validate(const Ship *ship, const System *playerSystem)
{
	if(Has(MINE) && ship->Cargo().Free() && targetAsteroid.expired())
	{
		Set(HARVEST);
		return;
	}

	bool targetShipInvalid = false,
		targetAsteroidInvalid = false;
	if((types & (HAS_TARGET_SHIP | HAS_TARGET_SHIP_OR_ASTEROID)).any())
	{
		shared_ptr<Ship> tShip = GetTargetShip();
		// Check if the target ship itself is targetable, or if it is one of your ship that you targeted.
		// If there's an attack order, make sure its type is correct.
		// Finally check if the target ship is in a system where we can target. This check only checks
		// for undocked ships (that have a current system).
		targetShipInvalid = !tShip
			|| (!tShip->IsTargetable() && tShip->GetGovernment() != ship->GetGovernment())
			|| (tShip->IsDisabled() && Has(ATTACK))
			|| (ship->GetSystem() && tShip->GetSystem() != ship->GetSystem() && tShip->GetSystem() != playerSystem);
	}
	if((types & (HAS_TARGET_ASTEROID | HAS_TARGET_SHIP_OR_ASTEROID)).any())
	{
		targetAsteroidInvalid = !GetTargetAsteroid();
		// Asteroids are never out of reach since they're in the same system as flagship.
	}

	// Clear orders that no longer have a valid and reachable target.
	if(targetShipInvalid)
	{
		types &= ~HAS_TARGET_SHIP;
		if(targetAsteroidInvalid)
			types &= ~HAS_TARGET_SHIP_OR_ASTEROID;
	}
	if(targetAsteroidInvalid)
		types &= ~HAS_TARGET_ASTEROID;

	// Reset targets that are no longer needed.
	if((types & (HAS_TARGET_SHIP | HAS_TARGET_SHIP_OR_ASTEROID)).none())
		targetShip.reset();
	if((types & (HAS_TARGET_ASTEROID | HAS_TARGET_SHIP_OR_ASTEROID)).none())
		targetAsteroid.reset();
}



void OrderSet::Update(const Ship &ship)
{
	if((Has(MOVE_TO) || Has(HOLD_ACTIVE)) && ship.GetSystem() == targetSystem)
	{
		// If nearly stopped on the desired point, switch to a HOLD_POSITION order.
		if(ship.Position().Distance(targetPoint) < 20. && ship.Velocity().Length() < .001)
			Set(HOLD_POSITION);
	}
	else if(Has(HOLD_POSITION) && ship.Position().Distance(targetPoint) > 20.)
	{
		// If far from the defined target point, return via a HOLD_ACTIVE order.
		Set(HOLD_ACTIVE);
		// Ensure the system reference is maintained.
		SetTargetSystem(ship.GetSystem());
	}
}
