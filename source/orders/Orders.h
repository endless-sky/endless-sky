/* Orders.h
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

#pragma once

#include "../Point.h"

#include <memory>

class Minable;
class Ship;
class System;



// This is a base class for handling ship orders. It contains members common
// for OrderSet and OrderSingle: information about targets and the list
// of possible order types.
class Orders {
public:
	enum class Types {
		HOLD_POSITION,
		// Hold active is the same command as hold position, but it is given when a ship
		// actively needs to move back to the position it was holding.
		HOLD_ACTIVE,
		MOVE_TO,
		KEEP_STATION,
		GATHER,
		ATTACK,
		FINISH_OFF,
		HOLD_FIRE,
		// MINE is for fleet targeting the asteroid for mining. ATTACK is used
		// to chase and attack the asteroid.
		MINE,
		// HARVEST is related to MINE and is for picking up flotsam after
		// ATTACK.
		HARVEST,
		// The last element needed to determine the size of the enum.
		TYPES_COUNT
	};


public:
	void SetTargetShip(std::shared_ptr<Ship> ship);
	void SetTargetAsteroid(std::shared_ptr<Minable> asteroid);
	void SetTargetPoint(const Point &point);
	void SetTargetSystem(const System *system);
	std::shared_ptr<Ship> GetTargetShip() const;
	std::shared_ptr<Minable> GetTargetAsteroid() const;
	const Point &GetTargetPoint() const;
	const System *GetTargetSystem() const;


protected:
	std::weak_ptr<Ship> targetShip;
	std::weak_ptr<Minable> targetAsteroid;
	Point targetPoint;
	const System *targetSystem = nullptr;
};
