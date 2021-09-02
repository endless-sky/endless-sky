/* MaskManager.cpp
Copyright (c) 2021 by Jonathan Steck

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "MaskManager.h"

#include "Files.h"
#include "Mask.h"
#include "Sprite.h"

using namespace std;



// Move the given masks at 1x scale into the manager's storage.
// The given vector will be cleared.
void MaskManager::AddMasks(const Sprite *sprite, std::vector<Mask> &masks)
{
	spriteMasks[sprite].emplace(1., move(masks));
	masks.clear();
}



// Add a scale that the given sprite needs to have a mask for.
void MaskManager::AddScale(const Sprite *sprite, double scale)
{
	spriteMasks[sprite][scale].clear();
}



// Create the scaled versions of all masks from the 1x versions. 
void MaskManager::ScaleMasks()
{
	for(auto &spriteScales : spriteMasks)
	{
		map<double, vector<Mask>> &scales = spriteScales.second;
		auto baseIt = scales.find(1.);
		if(baseIt == scales.end() || baseIt->second.empty())
			continue;
		
		const auto &baseMasks = baseIt->second;
		for(auto it = scales.begin(); it != baseIt; ++it)
		{
			auto &masks = it->second;
			masks.reserve(baseMasks.size());
			for(auto &&mask : baseMasks)
				masks.push_back(mask * it->first);
		}
		for(auto it = next(baseIt); it != scales.end(); ++it)
		{
			auto &masks = it->second;
			masks.reserve(baseMasks.size());
			for(auto &mask : baseMasks)
				masks.push_back(mask * it->first);
		}
	}
}



// Get the masks for the given sprite at the given scale. If a
// sprite has no masks, an empty mask is returned.
const std::vector<Mask> &MaskManager::GetMasks(const Sprite *sprite, double scale) const
{
	static const vector<Mask> EMPTY;
	const auto scalesIt = spriteMasks.find(sprite);
	if(scalesIt == spriteMasks.end())
	{
		Files::LogError("Failed to find collision mask for \"" + sprite->Name() + "\"");
		return EMPTY;
	}
	
	const auto &scales = scalesIt->second;
	const auto &maskIt = scales.find(scale);
	if(maskIt == scales.end() || maskIt->second.empty())
	{
		Files::LogError("Failed to find collision mask for \"" + sprite->Name() + "\" at scale " + to_string(scale));
		return EMPTY;
	}
	
	return maskIt->second;
}
