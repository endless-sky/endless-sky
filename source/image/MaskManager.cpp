/* MaskManager.cpp
Copyright (c) 2021 by Amazinite

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "MaskManager.h"

#include "../Logger.h"
#include "Sprite.h"

using namespace std;

namespace {
	const Point DEFAULT = Point(1., 1.);
	map<const Sprite *, bool> warned;

	string PrintScale(Point s)
	{
		return to_string(100. * s.X()) + "x" + to_string(100. * s.Y()) + "%";
	}
}



// Move the given masks at 1x scale into the manager's storage.
void MaskManager::SetMasks(const Sprite *sprite, vector<Mask> &&masks)
{
	lock_guard<mutex> lock(spriteMutex);
	auto &scales = spriteMasks[sprite];
	auto it = scales.find(DEFAULT);
	if(it != scales.end())
		it->second.swap(masks);
	else
		scales.emplace(DEFAULT, std::move(masks));
}



// Add a scale that the given sprite needs to have a mask for.
void MaskManager::RegisterScale(const Sprite *sprite, Point scale)
{
	lock_guard<mutex> lock(spriteMutex);
	auto &scales = spriteMasks[sprite];
	auto lb = scales.lower_bound(scale);
	if(lb == scales.end() || lb->first != scale)
		scales.emplace_hint(lb, scale, vector<Mask>{});
	else if(!lb->second.empty())
		Logger::Log("Collision mask for sprite \"" + sprite->Name() + "\" at scale "
			+ PrintScale(scale) + " was already generated.", Logger::Level::WARNING);
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
		for(auto &it : scales)
		{
			auto &masks = it.second;

			// Skip mask generation for scales that have already been generated previously.
			if(!masks.empty())
				continue;

			masks.reserve(baseMasks.size());
			for(auto &&mask : baseMasks)
				masks.push_back(mask * it.first);
		}
	}
}



// Get the masks for the given sprite at the given scale. If a
// sprite has no masks, an empty mask is returned.
const vector<Mask> &MaskManager::GetMasks(const Sprite *sprite, Point scale) const
{
	static const vector<Mask> EMPTY;
	const auto scalesIt = spriteMasks.find(sprite);
	if(scalesIt == spriteMasks.end())
	{
		if(warned.insert(make_pair(sprite, true)).second)
			Logger::Log("Sprite \"" + sprite->Name() + "\": no collision masks found.", Logger::Level::WARNING);
		return EMPTY;
	}

	const auto &scales = scalesIt->second;
	const auto maskIt = scales.find(scale);
	if(maskIt != scales.end() && !maskIt->second.empty())
		return maskIt->second;

	// Shouldn't happen, but just in case, print some details about the scales for this sprite (once).
	if(warned.insert(make_pair(sprite, true)).second)
	{
		string warning = "Sprite \"" + sprite->Name() + "\": collision mask not found.";
		if(scales.empty()) warning += " (No scaled masks.)";
		else if(maskIt != scales.end()) warning += " (No masks for scale " + PrintScale(scale) + ".)";
		else
		{
			warning += "\n\t" + PrintScale(scale) + " not found in known scales:";
			for(auto &&s : scales)
				warning += "\n\t\t" + PrintScale(s.first);
		}
		Logger::Log(warning, Logger::Level::WARNING);
	}
	return EMPTY;
}



bool MaskManager::Cmp::operator()(const Point &a, const Point &b) const noexcept
{
	return a.LengthSquared() < b.LengthSquared();
}
