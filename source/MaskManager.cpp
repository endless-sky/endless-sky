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
#include "Sprite.h"

using namespace std;

namespace {
	constexpr double DEFAULT = 1.;
	map<const Sprite *, bool> warned;
	
	string PrintScale(double s) {
		return to_string(100. * s) + "%";
	}
}



// Move the given masks at 1x scale into the manager's storage.
void MaskManager::SetMasks(const Sprite *sprite, std::vector<Mask> &&masks)
{
	auto &scales = spriteMasks[sprite];
	auto it = scales.find(DEFAULT);
	if(it != scales.end())
		it->second.swap(masks);
	else
		scales.emplace(DEFAULT, move(masks));
}



// Add a scale that the given sprite needs to have a mask for.
void MaskManager::RegisterScale(const Sprite *sprite, double scale)
{
	auto &scales = spriteMasks[sprite];
	auto lb = scales.lower_bound(scale);
	if(lb == scales.end() || lb->first != scale)
		scales.emplace_hint(lb, scale, vector<Mask>{});
	else if(!lb->second.empty())
		Files::LogError("Collision mask for sprite \"" + sprite->Name() + "\" at scale " + PrintScale(scale) + " was already generated.");
}



// Create the scaled versions of all masks from the 1x versions.
void MaskManager::ScaleMasks()
{
	for(auto &spriteScales : spriteMasks)
	{
		auto &scales = spriteScales.second;
		auto baseIt = scales.find(DEFAULT);
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
			for(auto &&mask : baseMasks)
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
		if(warned.insert(make_pair(sprite, true)).second)
			Files::LogError("Warning: sprite \"" + sprite->Name() + "\": no collision masks found.");
		return EMPTY;
	}
	
	const auto &scales = scalesIt->second;
	const auto maskIt = scales.find(scale);
	if(maskIt != scales.end() && !maskIt->second.empty())
		return maskIt->second;
	
	// Shouldn't happen, but just in case, print some details about the scales for this sprite (once).
	if(warned.insert(make_pair(sprite, true)).second)
	{
		string warning = "Warning: sprite \"" + sprite->Name() + "\": collision mask not found.";
		if(scales.empty()) warning += " (No scaled masks.)";
		else if(maskIt != scales.end()) warning += " (No masks for scale " + PrintScale(scale) + ".)";
		else
		{
			warning += "\n\t" + PrintScale(scale) + " not found in known scales:";
			for(auto &&s : scales)
				warning += "\n\t\t" + PrintScale(s.first);
		}
		Files::LogError(warning);
	}
	return EMPTY;
}
