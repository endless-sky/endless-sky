/* Distribution.h
Copyright (c) 2022 by DeBlister

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

#include <utility>

class Angle;



// A class which generates an angle of inaccuracy for a projectile
// given its inaccuracy value and type.
class Distribution {
public:
	enum class Type {
		Narrow = 0,
		Medium = 1,
		Wide = 2,
		Uniform = 3,
		Triangular = 4
	};


public:
	// Generate an angle that gets projectile heading
	// when combined with hardpoint aim.
	static Angle GenerateInaccuracy(double value, std::pair<Type, bool> distribution);
};
