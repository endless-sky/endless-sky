/* OutfitSale.h
Copyright (c) 2021 by Hurleveur

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef OUTFITSALE_H_
#define OUTFITSALE_H_

#include "Outfit.h"
#include "Sold.h"
#include <map>

class DataNode;
template <class Type>
class Set;



// Class used to stock Outfits and their local changes, it has
// their corresponding custom prices or showing/sellable types in the form of Sold.
class OutfitSale : public std::map<const Outfit *, Sold> {
public:
	void Load(const DataNode &node, const Set<Outfit> &items);
	
	void Add(const OutfitSale &other);
	
	const Sold* GetSold(const Outfit *item) const;
	
	double GetCost(const Outfit *item) const;
	
	Sold::ShowSold GetShown(const Outfit *item) const;
	
	bool Has(const Outfit *item) const;
};

#endif

