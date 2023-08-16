/* OutfitOptimizer.h
Copyright (c) 2023 by Dave Flowers

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef OUTFIT_OPTIMIZER_H_
#define OUTFIT_OPTIMIZER_H_

#include <map>
#include <set>
#include <string>
#include <vector>

class Outfit;



// This class supports the shipyard design center by taking the set
// of all available outfits and a given attribute and returning the
// smallest (in outfit space) set of outfits that will satisfy it.
class OutfitOptimizer {
public:
	// Set targetAmount to zero to perform an exhaustive search for the highest possible amount.
	OutfitOptimizer(const std::string &attribute, double targetAmount, double spaceLimit);

	// Add outfits from the outfitters.
	void AddOutfits(const std::set<const Outfit *> &outfits);
	// Add outfits from cargo or stock.
	void AddOutfits(const std::map<const Outfit *, int> &outfits);

	// If given a target amount, returns an empty set if no solution was found.
	std::map<const Outfit *, int> Optimize();


private:
	class OutfitStats {
	public:
		OutfitStats(const Outfit *outfit, double amount, double space, int count);

		bool operator<(const OutfitStats &other) const;

		const Outfit *outfit;
		double amount;
		double space;
		double efficiency;
		int count;
	};


private:
	bool InitializeOutfitList();
	bool FindBestFit(size_t offset, double targetAmount, double spaceLimit);
	double FindBestAmount(size_t offset, double spaceLimit);


private:
	std::string attribute;
	double totalAmount;
	double totalSpace;

	std::map<const Outfit *, int> outfits;

	std::map<const Outfit *, int> results;
	std::vector<OutfitStats> outfitStats;
	std::vector<int> counts;
};



#endif
