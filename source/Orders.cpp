/* Orders.cpp
Copyright (c) 2024 by Amazinite

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "Orders.h"

#include "Ship.h"

using namespace std;

namespace {
	using OrderSet = bitset<Orders::OrderType::TYPES_COUNT>;

	// Bit mask to figure out which orders are canceled if their target
	// ceases to be targetable or present.
	const OrderSet REQUIRES_TARGET_SHIP(
		(1 << Orders::OrderType::KEEP_STATION) +
		(1 << Orders::OrderType::GATHER) +
		(1 << Orders::OrderType::ATTACK) +
		(1 << Orders::OrderType::FINISH_OFF));

	const OrderSet REQUIRES_TARGET_ASTEROID(
		(1 << Orders::OrderType::MINE));

	// Bit masks that determine which orders may be given in conjunction with
	// the key order. If an order isn't present in the mask of the key order
	// then it will be set to false when the key order is given to a ship.
	const map<Orders::OrderType, OrderSet> ORDER_MASKS = {
		{Orders::HOLD_POSITION, OrderSet(1 << Orders::HOLD_FIRE)},
		{Orders::HOLD_ACTIVE, OrderSet(1 << Orders::HOLD_FIRE)},
		{Orders::HOLD_FIRE, OrderSet(
				(1 << Orders::HOLD_POSITION) +
				(1 << Orders::HOLD_ACTIVE) +
				(1 << Orders::MOVE_TO) +
				(1 << Orders::KEEP_STATION) +
				(1 << Orders::GATHER) +
				(1 << Orders::HARVEST))},
		{Orders::MOVE_TO, OrderSet(1 << Orders::HOLD_FIRE)},
		{Orders::KEEP_STATION, OrderSet(1 << Orders::HOLD_FIRE)},
		{Orders::GATHER, OrderSet(1 << Orders::HOLD_FIRE)},
		{Orders::ATTACK, OrderSet(0)},
		{Orders::FINISH_OFF, OrderSet(0)},
		{Orders::MINE, OrderSet(0)},
		{Orders::HARVEST, OrderSet(1 << Orders::HOLD_FIRE)},
	};
}



void Orders::SetHoldPosition()
{
	ApplyOrder(OrderType::HOLD_POSITION);
}



void Orders::SetHoldActive()
{
	ApplyOrder(OrderType::HOLD_ACTIVE);
}



void Orders::SetHoldFire()
{
	ApplyOrder(OrderType::HOLD_FIRE);
}



void Orders::SetMoveTo()
{
	ApplyOrder(OrderType::MOVE_TO);
}



void Orders::SetKeepStation()
{
	ApplyOrder(OrderType::KEEP_STATION);
}



void Orders::SetGather()
{
	ApplyOrder(OrderType::GATHER);
}



void Orders::SetAttack()
{
	ApplyOrder(OrderType::ATTACK);
}



void Orders::SetFinishOff()
{
	ApplyOrder(OrderType::FINISH_OFF);
}



void Orders::SetMine()
{
	ApplyOrder(OrderType::MINE);
}



void Orders::SetHarvest()
{
	ApplyOrder(OrderType::HARVEST);
}



bool Orders::HasHoldPosition() const
{
	return activeOrders.test(OrderType::HOLD_POSITION);
}



bool Orders::HasHoldActive() const
{
	return activeOrders.test(OrderType::HOLD_ACTIVE);
}



bool Orders::HasHoldFire() const
{
	return activeOrders.test(OrderType::HOLD_FIRE);
}



bool Orders::HasMoveTo() const
{
	return activeOrders.test(OrderType::MOVE_TO);
}



bool Orders::HasKeepStation() const
{
	return activeOrders.test(OrderType::KEEP_STATION);
}



bool Orders::HasGather() const
{
	return activeOrders.test(OrderType::GATHER);
}



bool Orders::HasAttack() const
{
	return activeOrders.test(OrderType::ATTACK);
}



bool Orders::HasFinishOff() const
{
	return activeOrders.test(OrderType::FINISH_OFF);
}



bool Orders::HasMine() const
{
	return activeOrders.test(OrderType::MINE);
}



bool Orders::HasHarvest() const
{
	return activeOrders.test(OrderType::HARVEST);
}



bool Orders::IsEmpty() const
{
	return activeOrders.none();
}



void Orders::SetTargetShip(shared_ptr<Ship> ship)
{
	targetShip = ship;
}



void Orders::SetTargetAsteroid(shared_ptr<Minable> asteroid)
{
	targetAsteroid = asteroid;
}



void Orders::SetTargetPoint(const Point &point)
{
	targetPoint = point;
}



shared_ptr<Ship> Orders::GetTargetShip() const
{
	return targetShip.lock();
}



void Orders::SetTargetSystem(const System *system)
{
	targetSystem = system;
}



shared_ptr<Minable> Orders::GetTargetAsteroid() const
{
	return targetAsteroid.lock();
}



const Point &Orders::GetTargetPoint() const
{
	return targetPoint;
}



const System *Orders::GetTargetSystem() const
{
	return targetSystem;
}



void Orders::UpdateOrder(const Ship *orderedShip, const System *flagshipSystem)
{
	if(HasMine() && orderedShip->Cargo().Free() && targetAsteroid.expired())
		SetHarvest();
	else if((activeOrders & REQUIRES_TARGET_SHIP).any())
	{
		// Check if the target ship itself is targetable, or if it is one of your ships that you targeted.
		shared_ptr<Ship> ship = GetTargetShip();
		bool invalidTarget = !ship
				|| (!ship->IsTargetable() && orderedShip->GetGovernment() != ship->GetGovernment())
				|| (ship->IsDisabled() && HasAttack());

		// Check if the target ship is in a system where we can target.
		// This check only checks for undocked ships (that have a current system).
		const System *orderSystem = orderedShip->GetSystem();
		const System *shipSystem = ship ? ship->GetSystem() : nullptr;
		bool targetOutOfReach = !ship || (orderSystem && shipSystem != orderSystem
				&& shipSystem != flagshipSystem);

		// Cancel any orders that required a target.
		if(invalidTarget || targetOutOfReach)
			activeOrders &= ~REQUIRES_TARGET_SHIP;
	}
	else if((activeOrders & REQUIRES_TARGET_ASTEROID).any() && !GetTargetAsteroid())
	{
		activeOrders &= ~REQUIRES_TARGET_ASTEROID;
	}
}



void Orders::MergeOrders(const Orders &other, bool &hasMismatch, bool &alreadyHarvesting, int &orderOperation)
{
	// HOLD_ACTIVE cannot be given as a manual order, but we make sure here
	// that any HOLD_ACTIVE order also matches when a HOLD_POSITION
	// command is given.
	if(HasHoldActive())
		ApplyOrder(HOLD_POSITION);

	bool newTargetShip = (GetTargetShip() != other.GetTargetShip());
	bool newTargetAsteroid = (GetTargetAsteroid() != other.GetTargetAsteroid());
	hasMismatch |= newTargetShip || newTargetAsteroid;

	// For each bit of the other order that is flipped, apply the flip to this order.
	for(size_t i = 0; i < other.activeOrders.size(); ++i)
		if(other.activeOrders.test(i))
		{
			bool alreadyActive = activeOrders.test(i);
			hasMismatch |= !alreadyActive;
			// If the existing order had a target and the new order is of the same type
			// but with a different target, then no change needs to be made to this bit.
			// Only run this check if the order operation is 2, as that means that this
			// is the first order that is being evaluated and will set the order
			// operation for all subsequent orders.
			if(orderOperation == 2 && alreadyActive)
			{
				if(REQUIRES_TARGET_SHIP.test(i) && newTargetShip)
					continue;
				if(REQUIRES_TARGET_ASTEROID.test(i) && newTargetAsteroid)
					continue;
			}
			orderOperation = ApplyOrder(static_cast<OrderType>(i), orderOperation);
		}

	// Skip giving any new orders if the fleet is already in harvest mode and the player has selected a new
	// asteroid.
	if(hasMismatch && other.GetTargetAsteroid())
		alreadyHarvesting = (HasHarvest() && other.HasHarvest());

	targetShip = other.targetShip;
	targetAsteroid = other.targetAsteroid;
	targetPoint = other.targetPoint;
	targetSystem = other.targetSystem;
}



bool Orders::ApplyOrder(OrderType newOrder, int operation)
{
	if(operation > 0)
		if(activeOrders.any() && !activeOrders.test(newOrder))
			activeOrders &= ORDER_MASKS.find(newOrder)->second;

	if(operation == 0)
		activeOrders.reset(newOrder);
	else if(operation == 1)
		activeOrders.set(newOrder);
	else if(operation == 2)
		activeOrders.flip(newOrder);
	return activeOrders.test(newOrder);
}
