/* GameAction.h
Copyright (c) 2022 by RisingLeaf

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef OUTFIT_FILTER_H_
#define OUTFIT_FILTER_H_

#include "Outfit.h"

#include <vector>
#include <string>

class DataNode;
class PlayerInfo;
class Ship;



class OutfitFilter
{
public:
	OutfitFilter() = default;
	// Construct and Load() at the same time.
	OutfitFilter(const DataNode &node);

	void Load(const DataNode &node);
	void Save(DataWriter &out);

	bool IsValid() const;

	std::vector<const Outfit *> MatchingOutfits(const Ship *ship) const;
	bool Matches(const Outfit *outfit) const;

private:
	bool isValid = false;

	std::vector<std::string> outfitTags;
	std::vector<std::string> outfitAttributes;

	std::vector<std::string> notOutfitTags;
	std::vector<std::string> notOutfitAttributes;
};



#endif
