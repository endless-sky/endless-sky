/* OutfitOptimizer.cpp
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

#include "OutfitOptimizer.h"

#include "Outfit.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <set>
#include <string>
#include <vector>

using namespace std;



OutfitOptimizer::OutfitOptimizer(const string &attribute, const double targetAmount, const double spaceLimit)
	: attribute(attribute), totalAmount(targetAmount), totalSpace(spaceLimit)
{
}



void OutfitOptimizer::AddOutfits(const set<const Outfit *> &outfitList)
{
	for(const auto &outfit : outfitList)
		if(outfit->Get(attribute) > 0.)
			outfits[outfit] = numeric_limits<int>::max();
}



void OutfitOptimizer::AddOutfits(const map<const Outfit *, int> &outfitList)
{
	for(const auto &it : outfitList)
		if(it.second > 0 && it.first->Get(attribute) > 0. && outfits[it.first] < numeric_limits<int>::max())
			outfits[it.first] += it.second;
}



map<const Outfit *, int> OutfitOptimizer::Optimize()
{
	// See if request was satisfied with zero-space outfits.
	if(InitializeOutfitList())
		return results;

	// Do the best we can with the space available.
	if(!totalAmount)
	{
		FindGreatestAmount(0, 0, totalSpace);
		for(size_t i = 0; i < counts.size(); ++i)
			if(counts[i])
				results[outfitStats[i].outfit] = counts[i];
	}
	// See if we can satisfy the requested amount.
	else if(FindBestFit(0, totalAmount, totalSpace))
	{
		for(size_t i = 0; i < counts.size(); ++i)
			if(counts[i])
				results[outfitStats[i].outfit] = counts[i];
	}
	// If not, don't include any outfits, even free ones.
	else
		results.clear();

	return results;
}



// Create the list of outfits to check (some may be winnowed out).
// Will add in "free" outfits (ones with no space requirement).
bool OutfitOptimizer::InitializeOutfitList()
{
	outfitStats.reserve(outfits.size());

	for(const auto &it : outfits)
	{
		const Outfit *outfit = it.first;
		const double amount = outfit->Get(attribute);
		const double space = -outfit->Get("outfit space");
		const int count = it.second;

		if(space > totalSpace)
			continue;
		else if(space > 0.)
		{
			// Remove (or don't add) outfits that are entirely superseded by others.
			vector<OutfitStats>::iterator oit = outfitStats.begin();
			while(oit != outfitStats.end())
			{
				if(space <= oit->space && (amount > oit->amount || (amount >= oit->amount && count > oit->count)))
					oit = outfitStats.erase(oit);
				else if(space >= oit->space && (amount < oit->amount || (amount <= oit->amount && count <= oit->count)))
					break;
				else
					++oit;
			}
			if(oit == outfitStats.end())
				outfitStats.emplace_back(outfit, amount, space, count);
		}
		else if(!totalAmount || amount * count < totalAmount)
		{
			results[outfit] = count;
			if(totalAmount)
				totalAmount -= amount * count;
			totalSpace -= space * count;
		}
		else
		{
			results[outfit] = ceil(totalAmount / amount);
			return true;
		}
	}

	sort(outfitStats.begin(), outfitStats.end());

	counts.assign(outfitStats.size(), 0);

	return false;
}



double OutfitOptimizer::FindBestFit(const size_t offset, const double targetAmount, const double spaceLimit)
{
	const OutfitStats &outfit(outfitStats[offset]);

	// Check if remaining outfits are theoretically efficient enough to improve the target.
	if(targetAmount > spaceLimit * outfit.efficiency)
		return 0.;

	// Start with enough of this outfit to fill the requirement, if possible.
	int n = ceil(targetAmount / outfit.amount);
	double bestSpace = 0.;

	if(n * outfit.space > spaceLimit || n > outfit.count)
	{
		if(offset + 1 == outfitStats.size())
			return 0.;
		else
			n = min(outfit.count, static_cast<int>(floor(spaceLimit / outfit.space))) + 1;
	}
	else if(offset + 1 == outfitStats.size())
	{
		counts[offset] = n;
		return n * outfit.space;
	}
	else
	{
		bestSpace = n * outfit.space;
		counts[offset] = n;
	}

	// Check for possibly better results with fewer of this outfit.
	vector<int> bestCounts(counts.size() - offset - 1, 0);
	for(--n; n > -1; --n)
	{
		const double space = n * outfit.space;
		const double otherSpace = FindBestFit(offset + 1, targetAmount - n * outfit.amount, spaceLimit - space);
		if(otherSpace && (!bestSpace || bestSpace > space + otherSpace))
		{
			bestSpace = space + otherSpace;
			counts[offset] = n;
			copy(counts.begin() + offset + 1, counts.end(), bestCounts.begin());
		}
	}

	if(bestSpace)
		copy(bestCounts.begin(), bestCounts.end(), counts.begin() + offset + 1);

	return bestSpace;
}



double OutfitOptimizer::FindGreatestAmount(const size_t offset, const double targetAmount, const double spaceLimit)
{
	const OutfitStats &outfit(outfitStats[offset]);

	// Check if remaining outfits are theoretically efficient enough to improve the amount.
	if(targetAmount > spaceLimit * outfit.efficiency)
		return 0;

	// Start with as many of this outfit as possible.
	int n = min(outfit.count, static_cast<int>(floor(spaceLimit / outfit.space)));
	double bestAmount = n * outfit.amount;
	counts[offset] = n;

	if(offset + 1 == outfitStats.size())
		return bestAmount;

	// Check for possibly better results with fewer of this outfit.
	vector<int> bestCounts(counts.size() - offset - 1, 0);
	for(; n > -1; --n)
	{
		const double myAmount = n * outfit.amount;
		const double amount = myAmount + FindGreatestAmount(offset + 1, bestAmount - myAmount, spaceLimit - n * outfit.space);
		if(bestAmount < amount)
		{
			bestAmount = amount;
			counts[offset] = n;
			copy(counts.begin() + offset + 1, counts.end(), bestCounts.begin());
		}
	}

	copy(bestCounts.begin(), bestCounts.end(), counts.begin() + offset + 1);
	return bestAmount;
}



OutfitOptimizer::OutfitStats::OutfitStats(const Outfit *outfit, const double amount, const double space,
		const int count)
	: outfit(outfit), amount(amount), space(space), efficiency(amount / space), count(count)
{
}



bool OutfitOptimizer::OutfitStats::operator<(const OutfitStats &other) const
{
	if(other.efficiency != efficiency)
		return other.efficiency < efficiency;
	else if(other.count != count)
		return other.count < count;
	else
		return other.amount < amount;
}
