/* Distribution.cpp
Copyright (c) 2022 by DeBlister

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "Distribution.h"

#include "Random.h"


namespace {
	double ManipulateNormal(double smoothness, bool invert)
	{
		// The incoming value is always guaranteed to be an int, casted from a Distribution::Type.
		// The 4.634 manipulation causes the Middling value to result in a standard spread and
		// the 2 is to account for the initial range of the distribution ([0, 1] instead of [-1, 1]).
		smoothness /= 4.634 * 2;
		// Center values within [0, 1] so that fractional retention begins to accumulate
		// at the endpoints (rather than at the center) of the distribution.
		double randomFactor = Random::BMNormal(0.5, smoothness);
		// Retain only the fractional information, to keep all absolute values within [0, 1].
		// Might be possible to get away with int32_t here, not sure.
		randomFactor = randomFactor - static_cast<int64_t>(randomFactor);

		// Shift negative values into [0, 1] to create redundancy at the endpoints
		if(randomFactor < 0.)
			randomFactor++;

		// Invert probabilities so that endpoints are most probable.
		if(invert)
		{
			if(randomFactor > 0.5)
				randomFactor -= 0.5;
			else if(randomFactor < 0.5)
				randomFactor += 0.5;
		}

		// Transform from [0, 1] to [-1, 1] so that the return value can be simply used.
		randomFactor *= 2;
		randomFactor--;

		return randomFactor;
	}
}



Angle Distribution::GenerateInaccuracy(double value, std::pair<Distribution::Type, bool> distribution)
{
	// Check if there is any inaccuracy to apply
	if(value)
	{
		switch(distribution.first)
		{
			case Distribution::Type::Uniform:
				return Angle(2 * (Random::Real() - 0.5) * value);
			case Distribution::Type::Tight:
			case Distribution::Type::Middling:
			case Distribution::Type::Wide:
				return Angle(value * ManipulateNormal(static_cast<double>(distribution.first), distribution.second));
			case Distribution::Type::Triangular:
			default:
				return Angle((Random::Real() - Random::Real()) * value);;
		}
	}
	else
		return Angle();
}
