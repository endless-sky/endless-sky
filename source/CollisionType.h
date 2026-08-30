/* CollisionType.h
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

// Types of objects that projectiles are able to collide with.
// Each CollisionSet has a CollisionType that it keeps track of.
// Explosions are a special collision type that didn't result from
// a direct impact.
enum class CollisionType : int {
	EXPLOSION,
	SHIP,
	MINABLE,
	ASTEROID,
};
