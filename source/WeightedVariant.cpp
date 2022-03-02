/* WeightedVariant.cpp
Copyright (c) 2021 by Amazinite

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "WeightedVariant.h"

using namespace std;



WeightedVariant::WeightedVariant(Variant variant, int weight)
	: variant(variant), weight(weight)
{
}



WeightedVariant::WeightedVariant(const Variant *stockVariant, int weight)
	: stockVariant(stockVariant), weight(weight)
{
}



const Variant &WeightedVariant::Get() const
{
	return stockVariant ? *stockVariant : variant;
}



int WeightedVariant::Weight() const
{
	return weight;
}



bool WeightedVariant::operator==(const WeightedVariant &other) const
{
	return other.weight == weight && other.Get() == Get();
}



bool WeightedVariant::operator!=(const WeightedVariant &other) const
{
	return !(*this == other);
}
