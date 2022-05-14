/* Distribution.h
Copyright (c) 2022 by DeBlister

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef DISTRIBUTION_H_
#define DISTRIBUTION_H_

#include "Angle.h"

#include <tuple>



class Distribution{
public:
	enum class Type{
		Triangular = 0,
		Tight = 1,
		Middling = 2,
		Wide = 3,
		Uniform = 4
	};


public:
	// Generate an angle that gets projectile heading
	// when combined with hardpoint aim.
	static Angle GenerateInaccuracy(double value, std::pair<Distribution::Type, bool>);
};



#endif
