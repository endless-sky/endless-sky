/* WeightedVariant.cpp
Copyright (c) 2021 by Jonathan Steck

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "WeightedVariant.h"

using namespace std;



WeightedVariant::WeightedVariant(const Variant *stockVariant, int weight)
	: weight(weight), stockVariant(stockVariant)
{
}



WeightedVariant::WeightedVariant(Variant variant, int weight)
	: weight(weight), variant(variant)
{
}



int WeightedVariant::Weight() const
{
	return weight;
}



const Variant &WeightedVariant::Get() const
{
	return stockVariant ? *stockVariant : variant;
}



bool WeightedVariant::operator==(const WeightedVariant &other) const
{
	return other.weight == weight && other.Get() == Get();
}



bool WeightedVariant::operator!=(const WeightedVariant &other) const
{
	return !(*this == other);
}
