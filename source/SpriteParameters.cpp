/* SpriteParameters.cpp
Copyright (c) 2024 by XessWaffle

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "SpriteParameters.h"

#include "image/Sprite.h"


SpriteParameters::SpriteParameters()
{
}



SpriteParameters::SpriteParameters(const Sprite *sprite)
{
	auto spriteDetails = SpriteDetails{sprite, {}, {}};
	sprites.insert(pair<int, SpriteDetails>(0, spriteDetails));
	exposed = {};
	exposedIndex = 0;
	defaultDetails = spriteDetails;
	exposedDetails = spriteDetails;
}



void SpriteParameters::SetSprite(int index, const Sprite *sprite, AnimationParameters data,
	ConditionSet triggerConditions)
{
	auto spriteDetails = SpriteDetails{sprite, data, triggerConditions};
	sprites.insert(pair<int, SpriteDetails>(index, spriteDetails));
	if(index == 0)
	{
		exposed = data;
		exposedIndex = 0;
		defaultDetails = spriteDetails;
		exposedDetails = spriteDetails;
	}
}



const Sprite *SpriteParameters::GetSprite(int index) const
{
	if(index < 0)
		return std::get<0>(exposedDetails);

	auto it = sprites.find(index);
	if(it != sprites.end())
		return std::get<0>(it->second);

	return nullptr;
}



ConditionSet SpriteParameters::GetConditions(int index) const
{
	if(index < 0)
		return std::get<2>(exposedDetails);

	auto it = sprites.find(index);
	if(it != sprites.end())
		return std::get<2>(it->second);

	return {};
}



SpriteParameters::AnimationParameters SpriteParameters::GetParameters(int index) const
{
	if(index < 0)
		return std::get<1>(exposedDetails);

	auto it = sprites.find(index);
	if(it != sprites.end())
		return std::get<1>(it->second);

	return {};
}



int SpriteParameters::GetExposedID() const
{
	return exposedIndex;
}



int SpriteParameters::RequestTriggerUpdate(ConditionsStore &store)
{
	// 0 is the index of the default fallback sprite.
	for(auto it : sprites)
		if(it.first != 0)
		{
			ConditionSet conditions = std::get<2>(it.second);
			if(conditions.Test(store))
			{
				requestedIndex = it.first;
				return it.first;
			}
		}



	// Return to default.
	if(exposedIndex != 0)
		requestedIndex = 0;
	return 0;
}



void SpriteParameters::CompleteTriggerRequest()
{
	Expose(requestedIndex);
}



const SpriteParameters::SpriteMap *SpriteParameters::GetAllSprites() const
{
	return &sprites;
}



SpriteParameters::AnimationParameters *SpriteParameters::GetExposedParameters()
{
	return &exposed;
}



void SpriteParameters::Expose(int index)
{
	exposedIndex = index;
	auto it = sprites.find(index);
	exposed = std::get<1>(it->second);
	exposedDetails = it->second;
}
