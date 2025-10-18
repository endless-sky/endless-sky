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
	constexpr bitset<static_cast<size_t>(Orders::Types::TYPES_COUNT)> HAS_TARGET_SHIP{
		(1 << static_cast<int>(Orders::Types::KEEP_STATION)) +
		(1 << static_cast<int>(Orders::Types::GATHER)) +
		(1 << static_cast<int>(Orders::Types::FINISH_OFF))
	};

	constexpr bitset<static_cast<size_t>(Orders::Types::TYPES_COUNT)> HAS_TARGET_ASTEROID{
		(1 << static_cast<int>(Orders::Types::MINE))
	};

	constexpr bitset<static_cast<size_t>(Orders::Types::TYPES_COUNT)> HAS_TARGET_SHIP_OR_ASTEROID{
		(1 << static_cast<int>(Orders::Types::ATTACK))
	};

	constexpr bitset<static_cast<size_t>(Orders::Types::TYPES_COUNT)> HAS_TARGET_LOCATION{
		(1 << static_cast<int>(Orders::Types::MOVE_TO))
	};

	// Orders not included in the bitset should be removed when the given order is issued.
	constexpr array<bitset<static_cast<size_t>(Orders::Types::TYPES_COUNT)>,
		static_cast<size_t>(Orders::Types::TYPES_COUNT)> SIMULTANEOUS{{
		{(1 << static_cast<int>(Orders::Types::HOLD_FIRE))}, // HOLD_POSITION
		{(1 << static_cast<int>(Orders::Types::HOLD_FIRE))}, // HOLD_ACTIVE
		{(1 << static_cast<int>(Orders::Types::HOLD_FIRE))}, // MOVE_TO
		{(1 << static_cast<int>(Orders::Types::HOLD_FIRE))}, // KEEP_STATION
		{(1 << static_cast<int>(Orders::Types::HOLD_FIRE))}, // GATHER
		{}, // ATTACK
		{}, // FINISH_OFF
		{
			(1 << static_cast<int>(Orders::Types::HOLD_POSITION)) +
			(1 << static_cast<int>(Orders::Types::HOLD_ACTIVE)) +
			(1 << static_cast<int>(Orders::Types::MOVE_TO)) +
			(1 << static_cast<int>(Orders::Types::KEEP_STATION)) +
			(1 << static_cast<int>(Orders::Types::GATHER)) +
			(1 << static_cast<int>(Orders::Types::HARVEST))
		}, // HOLD_FIRE
		{}, // MINE
		{(1 << static_cast<int>(Orders::Types::HOLD_FIRE))}, // HARVEST
	}};
}



bool OrderSet::Has(Types type) const noexcept
{
	return types[static_cast<size_t>(type)];
}



bool OrderSet::Empty() const noexcept
{
	return types.none();
}



void OrderSet::Add(const OrderSingle &newOrder, bool *hasMismatch, bool *alreadyHarvesting)
{
	// HOLD_ACTIVE cannot be given as manual order, but is used internally by ship AI.
	// Set HOLD_POSITION here, so that it's possible for the player to unset the order.
	if(Has(Types::HOLD_ACTIVE))
		Set(Types::HOLD_POSITION);

	shared_ptr<Ship> newTargetShip = newOrder.GetTargetShip();
	bool newTargetShipRelevant = HAS_TARGET_SHIP[static_cast<size_t>(newOrder.type)]
		|| HAS_TARGET_SHIP_OR_ASTEROID[static_cast<size_t>(newOrder.type)];
	shared_ptr<Minable> newTargetAsteroid = newOrder.GetTargetAsteroid();
	bool newTargetAsteroidRelevant = HAS_TARGET_ASTEROID[static_cast<size_t>(newOrder.type)]
		|| HAS_TARGET_SHIP_OR_ASTEROID[static_cast<size_t>(newOrder.type)];

	bool individualHasMismatch = !Has(newOrder.type)
		|| (newTargetShipRelevant && GetTargetShip() != newTargetShip)
		|| (newTargetAsteroidRelevant && GetTargetAsteroid() != newTargetAsteroid);
	if(hasMismatch)
		*hasMismatch |= individualHasMismatch;

	if(hasMismatch ? *hasMismatch : individualHasMismatch)
	{
		Set(newOrder.type);
		if(alreadyHarvesting && newTargetAsteroid)
			*alreadyHarvesting = Has(Types::HARVEST) && newOrder.type == Types::HARVEST;
	}
	else if(hasMismatch || individualHasMismatch)
	{
		// The new order is already in the old set, so it should be removed instead.
		Reset(newOrder.type);
		return;
	}

	// Update target ship and/or asteroid if it's relevant for the new order.
	if(newTargetShipRelevant)
		SetTargetShip(newTargetShip);
	if(newTargetAsteroidRelevant)
		SetTargetAsteroid(newTargetAsteroid);

	// Update target system and point if it's relevant for the new order.
	if(HAS_TARGET_LOCATION[static_cast<size_t>(newOrder.type)])
	{
		SetTargetPoint(newOrder.GetTargetPoint());
		SetTargetSystem(newOrder.GetTargetSystem());
	}
}



void OrderSet::Validate(const Ship *ship, const System *playerSystem)
{
	if(Has(Types::MINE) && ship->Cargo().Free() && targetAsteroid.expired())
	{
		Set(Types::HARVEST);
		return;
	}

	bool targetShipInvalid = false;
	bool targetAsteroidInvalid = false;
	if((types & (HAS_TARGET_SHIP | HAS_TARGET_SHIP_OR_ASTEROID)).any())
	{
		shared_ptr<Ship> tShip = GetTargetShip();
		// Check if the target ship itself is targetable, or if it is one of your ship that you targeted.
		// If there's an attack order, make sure its type is correct.
		// Finally check if the target ship is in a system where we can target. This check only checks
		// for undocked ships (that have a current system).
		targetShipInvalid = !tShip
			|| (!tShip->IsTargetable() && tShip->GetGovernment() != ship->GetGovernment())
			|| (tShip->IsDisabled() && Has(Types::ATTACK))
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
	if((Has(Types::MOVE_TO) || Has(Types::HOLD_ACTIVE)) && ship.GetSystem() == targetSystem)
	{
		// If nearly stopped on the desired point, switch to a HOLD_POSITION order.
		if(ship.Position().Distance(targetPoint) < 20. && ship.Velocity().Length() < .001)
			Set(Types::HOLD_POSITION);
	}
	else if(Has(Types::HOLD_POSITION) && ship.Position().Distance(targetPoint) > 20.)
	{
		// If far from the defined target point, return via a HOLD_ACTIVE order.
		Set(Types::HOLD_ACTIVE);
		// Ensure the system reference is maintained.
		SetTargetSystem(ship.GetSystem());
	}
}



void OrderSet::Set(Types type) noexcept
{
	types &= SIMULTANEOUS[static_cast<size_t>(type)];
	types.set(static_cast<size_t>(type));
}



void OrderSet::Reset(Types type) noexcept
{
	types.reset(static_cast<size_t>(type));
}
