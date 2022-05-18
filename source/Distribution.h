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
		Tight = 0,
		Middling = 1,
		Wide = 2,
		Uniform = 3,
		Triangular = 4
	};


public:
	// Generate an angle that gets projectile heading
	// when combined with hardpoint aim.
	static Angle GenerateInaccuracy(double value, std::pair<Type, bool>);
};



#endif
