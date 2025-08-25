/* WormholeStrategy.h
Copyright (c) 2022 by warp-core

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

#include <cstdint>



// Strategies that a DistanceMap can use to determine which wormholes to make use of
// when plotting a course to a destination, if any.
enum class WormholeStrategy : int_fast8_t {
	// Disallow use of any wormholes.
	NONE,
	// Disallow use of wormholes which the player cannot access, such as in
	// the case of a wormhole that requires an attribute to use.
	ONLY_UNRESTRICTED,
	// Allow use of all wormholes.
	ALL,
};
