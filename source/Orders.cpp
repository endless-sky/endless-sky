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
	// Bit mask to figure out which orders are canceled if their target
	// ceases to be targetable or present.
	const bitset<Orders::ORDER_COUNT> REQUIRES_TARGET(
		(1 << Orders::OrderType::KEEP_STATION) +
		(1 << Orders::OrderType::GATHER) +
		(1 << Orders::OrderType::ATTACK) +
		(1 << Orders::OrderType::FINISH_OFF) +
		(1 << Orders::OrderType::MINE));
}



void Orders::SetHoldPosition()
{
	ApplyOrder(OrderType::HOLD_POSITION);
}



void Orders::SetHoldActive()
{
	ApplyOrder(OrderType::HOLD_ACTIVE);
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
	else if((activeOrders & REQUIRES_TARGET).any())
	{
		shared_ptr<Ship> ship = GetTargetShip();
		shared_ptr<Minable> asteroid = GetTargetAsteroid();
		// Check if the target ship itself is targetable, or if it is one of your ships that you targeted.
		bool invalidTarget = !ship
				|| (!ship->IsTargetable() && orderedShip->GetGovernment() != ship->GetGovernment())
				|| (ship->IsDisabled() && HasAttack());
		// Alternatively, if an asteroid is targeted, then it is not an invalid target.
		invalidTarget &= !asteroid;

		// Check if the target ship is in a system where we can target.
		// This check only checks for undocked ships (that have a current system).
		const System *orderSystem = orderedShip->GetSystem();
		const System *shipSystem = ship ? ship->GetSystem() : nullptr;
		bool targetOutOfReach = !ship || (orderSystem && shipSystem != orderSystem
				&& shipSystem != flagshipSystem);
		// Asteroids are never out of reach since they're in the same system as the flagship.
		targetOutOfReach &= !asteroid;

		// Cancel any orders that required a target.
		if(invalidTarget || targetOutOfReach)
			activeOrders &= ~REQUIRES_TARGET;
	}
}



void Orders::MergeOrders(const Orders &other, bool &hasMismatch, bool &alreadyHarvesting, int &orderOperation)
{
	// HOLD_ACTIVE cannot be given as a manual order, but we make sure here
	// that any HOLD_ACTIVE order also matches when a HOLD_POSITION
	// command is given.
	if(HasHoldActive())
		ApplyOrder(HOLD_POSITION);

	// For each bit of the other order that is flipped, apply the flip to this order.
	for(size_t i = 0; i < other.activeOrders.size(); ++i)
		if(other.activeOrders.test(i))
		{
			hasMismatch |= !activeOrders.test(i);
			orderOperation = ApplyOrder(static_cast<OrderType>(i), orderOperation);
		}

	hasMismatch |= (GetTargetShip() != other.GetTargetShip());
	hasMismatch |= (GetTargetAsteroid() != other.GetTargetAsteroid());
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
	// Bit masks that determine which orders may be given in conjunction with
	// the key order. If an order isn't present in the mask of the key order
	// then it will be set to false when the key order is given to a ship.
	static const map<OrderType, bitset<ORDER_COUNT>> ORDER_MASKS = {
		{HOLD_POSITION, bitset<ORDER_COUNT>(0)},
		{HOLD_ACTIVE, bitset<ORDER_COUNT>(0)},
		{MOVE_TO, bitset<ORDER_COUNT>(0)},
		{KEEP_STATION, bitset<ORDER_COUNT>(0)},
		{GATHER, bitset<ORDER_COUNT>(0)},
		{ATTACK, bitset<ORDER_COUNT>(0)},
		{FINISH_OFF, bitset<ORDER_COUNT>(0)},
		{MINE, bitset<ORDER_COUNT>(0)},
		{HARVEST, bitset<ORDER_COUNT>(0)},
	};

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
