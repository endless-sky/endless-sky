/* Distribution.cpp
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

#include "Distribution.h"

#include "Angle.h"
#include "Random.h"

#include <vector>

using namespace std;

namespace {
	double ManipulateNormal(double smoothness, bool inverted)
	{
		// Center values within [0, 1] so that fractional retention begins to accumulate
		// at the endpoints (rather than at the center) of the distribution.
		double randomFactor = Random::Normal(.5, smoothness);
		// Retain only the fractional information, to keep all absolute values within [0, 1].
		randomFactor = randomFactor - static_cast<int64_t>(randomFactor);

		// Shift negative values into [0, 1] to create redundancy at the endpoints
		if(randomFactor < 0.)
			++randomFactor;

		// Invert probabilities so that endpoints are most probable.
		if(inverted)
		{
			if(randomFactor > .5)
				randomFactor -= .5;
			else if(randomFactor < .5)
				randomFactor += .5;
		}

		// Transform from [0, 1] to [-1, 1] so that the return value can be simply used.
		randomFactor *= 2;
		--randomFactor;

		return randomFactor;
	}

	// These values are paired with Distribution::Types; any change in one should be made in the other.
	const double SMOOTHNESS_TABLE[] = {.13, .234, .314};
}



Angle Distribution::GenerateInaccuracy(double value, pair<Type, bool> distribution)
{
	// Check if there is any inaccuracy to apply
	if(!value)
		return Angle();

	switch(distribution.first)
	{
		case Type::Uniform:
			return Angle(2 * (Random::Real() - .5) * value);
		case Type::Narrow:
		case Type::Medium:
		case Type::Wide:
			return Angle(value * ManipulateNormal(SMOOTHNESS_TABLE[static_cast<int>(distribution.first)],
					distribution.second));
		case Type::Triangular:
		default:
			return Angle((Random::Real() - Random::Real()) * value);
	}
}
