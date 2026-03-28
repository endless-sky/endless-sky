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

#pragma once

#include "Body.h"
#include "CollisionType.h"



// Represents a collision between a projectile and a ship, asteroid, or minable.
class Collision {
public:
	// Initialize a Collision.
	Collision(Body *hit, CollisionType collisionType, double range);

	// The Body that was hit for this collision. May be a nullptr if nothing
	// was directly hit.
	Body *HitBody();
	// The type of Body that was hit.
	CollisionType GetCollisionType() const;
	// The intersection range at which the collision occurred with the Body.
	double IntersectionRange() const;

	// Compare two Collisions by their intersection range.
	bool operator<(const Collision &rhs) const;


private:
	Body *hit = nullptr;
	CollisionType collisionType = CollisionType::NONE;
	double range;
};
