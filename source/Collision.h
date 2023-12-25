/* Collision.h
Copyright (c) 2023 by Amazinite

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef COLLISION_H_
#define COLLISION_H_

#include "Body.h"
#include "CollisionType.h"

#include <list>
#include <memory>
#include <vector>



// Represents a collision between a projectile and a ship, asteroid, or minable.
// Contains information such as the distance
class Collision {
public:
	// Initialize a Collision, recording the Body that was hit, the type of
	// object that the Body is (nothing, a Ship, a Minable, or an Asteroid),
	// and the range that the Body was hit at.
	Collision(Body *hit, CollisionType collisionType, double range);

	Body *HitBody();
	CollisionType GetCollisionType() const;
	double IntersectionRange() const;

	bool operator<(const Collision &rhs) const;


private:
	Body *hit = nullptr;
	CollisionType collisionType = CollisionType::NONE;
	double range;
};



#endif
