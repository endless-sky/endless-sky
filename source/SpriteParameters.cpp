/* SpriteParameters.cpp
Copyright (c) 2022 by XessWaffle

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

#include "Sprite.h"

SpriteParameters::SpriteParameters()
{
}



SpriteParameters::SpriteParameters(const Sprite *sprite)
{
	AnimationParameters initDefault;
	static ConditionSet empty;
	auto tuple = SpriteParameters::SpriteDetails{sprite, initDefault, empty};
	this->sprites.insert(std::pair<int, SpriteParameters::SpriteDetails>(0, tuple));
	this->exposed = initDefault;
	this->exposedIndex = 0;
	this->defaultDetails = tuple;
	this->exposedDetails = tuple;
}



void SpriteParameters::SetSprite(int index, const Sprite *sprite,
									SpriteParameters::AnimationParameters params, ConditionSet triggerConditions)
{
	auto tuple = SpriteParameters::SpriteDetails{sprite, params, triggerConditions};
	this->sprites.insert(std::pair<int, SpriteParameters::SpriteDetails>(index, tuple));
	if(index == DEFAULT)
	{
		this->exposed = params;
		this->exposedIndex = 0;
		this->defaultDetails = tuple;
		this->exposedDetails = tuple;
	}
}



const Sprite *SpriteParameters::GetSprite(int index) const
{
	if(index < 0)
		return std::get<0>(this->exposedDetails);
	else
	{
		auto it = this->sprites.find(index);
		if(it != this->sprites.end())
			return std::get<0>(it->second);
	}
	return NULL;
}



ConditionSet SpriteParameters::GetConditions(int index) const
{
	static ConditionSet empty;
	if(index < 0)
		return std::get<2>(this->exposedDetails);
	else
	{
		auto it = this->sprites.find(index);
		if(it != this->sprites.end())
			return std::get<2>(it->second);
	}
	return empty;
}



SpriteParameters::AnimationParameters SpriteParameters::GetParameters(int index) const
{
	SpriteParameters::AnimationParameters def;
	if(index < 0)
		return std::get<1>(this->exposedDetails);
	else
	{
		auto it = this->sprites.find(index);
		if(it != this->sprites.end())
			return std::get<1>(it->second);
	}
	return def;
}



const int SpriteParameters::GetExposedID() const
{
	return this->exposedIndex;
}



const int SpriteParameters::RequestTriggerUpdate(ConditionsStore &store)
{
	for(auto it = this->sprites.begin(); it != this->sprites.end(); it++)
		if(it->first != DEFAULT)
		{
			ConditionSet conditions = std::get<2>(it->second);
			if(conditions.Test(store))
			{
				this->requestedIndex = it->first;
				return it->first;
			}

		}
	// Return to default
	if(this->exposedIndex != DEFAULT)
		this->requestedIndex = DEFAULT;
	return DEFAULT;
}



void SpriteParameters::CompleteTriggerRequest()
{
	this->Expose(this->requestedIndex);
}



const SpriteParameters::SpriteMap *SpriteParameters::GetAllSprites() const
{
	return &this->sprites;
}



void SpriteParameters::Expose(int index)
{
	this->exposedIndex = index;
	auto it = this->sprites.find(index);
	this->exposed = std::get<1>(it->second);
	this->exposedDetails = it->second;
}
