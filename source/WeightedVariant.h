/* WeightedVariant.h
Copyright (c) 2021 by Amazinite

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef WEIGHTED_VARIANT_H_
#define WEIGHTED_VARIANT_H_

#include "Variant.h"


// A weighted variant consists of a variant and a weight to be used by Fleet
// or Variant in a WeightedList.
class WeightedVariant {
public:
	WeightedVariant() = default;

	WeightedVariant(Variant variant, int weight);
	WeightedVariant(const Variant *stockVariant, int weight);

	const Variant &Get() const;
	int Weight() const;

	bool operator==(const WeightedVariant &other) const;
	bool operator!=(const WeightedVariant &other) const;


private:
	Variant variant;
	const Variant *stockVariant = nullptr;
	int weight;
};



#endif
