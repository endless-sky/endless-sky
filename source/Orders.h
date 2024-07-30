/* Orders.h
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

#pragma once

#include <bitset>
#include <map>
#include <memory>

#include "Point.h"

class Minable;
class Ship;
class System;


// Orders represent commands that have been given to the player's escorts.
class Orders {
public:
	enum OrderType : int {
		HOLD_POSITION,
		// HOLD_ACTIVE is the same command as HOLD_POSITION, but it is given when a ship
		// actively needs to move back to the position it was holding.
		HOLD_ACTIVE,
		HOLD_FIRE,
		MOVE_TO,
		KEEP_STATION,
		GATHER,
		ATTACK,
		// FINISH_OFF is used to ATTACK ships that are disabled.
		FINISH_OFF,
		// MINE is for fleet targeting the asteroid for mining. ATTACK is used
		// to chase and attack the asteroid.
		MINE,
		// HARVEST is related to MINE and is for picking up flotsam after
		// ATTACK.
		HARVEST,

		// This must be last to define the size of the bitset.
		TYPES_COUNT
	};


public:
	// Set and get the active order types on this order.
	void SetHoldPosition();
	void SetHoldActive();
	void SetHoldFire();
	void SetMoveTo();
	void SetKeepStation();
	void SetGather();
	void SetAttack();
	void SetFinishOff();
	void SetMine();
	void SetHarvest();

	bool HasHoldPosition() const;
	bool HasHoldActive() const;
	bool HasHoldFire() const;
	bool HasMoveTo() const;
	bool HasKeepStation() const;
	bool HasGather() const;
	bool HasAttack() const;
	bool HasFinishOff() const;
	bool HasMine() const;
	bool HasHarvest() const;

	bool IsEmpty() const;

	// Set and get targeting information for this order.
	void SetTargetShip(std::shared_ptr<Ship> ship);
	void SetTargetAsteroid(std::shared_ptr<Minable> asteroid);
	void SetTargetPoint(const Point &point);
	void SetTargetSystem(const System *system);
	std::shared_ptr<Ship> GetTargetShip() const;
	std::shared_ptr<Minable> GetTargetAsteroid() const;
	const Point &GetTargetPoint() const;
	const System *GetTargetSystem() const;

	// Determine if this order must update itself in any way
	// given changes that have occurred to its targets.
	void UpdateOrder(const Ship *ship, const System *flagshipSystem);
	// Merge this order with another order.
	void MergeOrders(const Orders &other, bool &hasMismatch, bool &alreadyHarvesting, int &orderOperation);


private:
	// Apply the new order type to the existing orders. The operation parameter
	// determines if the new order bit should be reset, set, or flipped based on whether
	// the parameter value is 0, 1, or 2.
	// If the order bit is being changed to true, a bitmask is applied to cancel any
	// conflicting orders.
	// Returns the status of the changed order bit.
	bool ApplyOrder(OrderType newOrder, int operation = 1);


private:
	std::bitset<OrderType::TYPES_COUNT> activeOrders;
	std::weak_ptr<Ship> targetShip;
	std::weak_ptr<Minable> targetAsteroid;
	Point targetPoint;
	const System *targetSystem = nullptr;
};
