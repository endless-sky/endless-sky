/* Outfit.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "Outfit.h"

#include "Audio.h"
#include "DataNode.h"
#include "Effect.h"
#include "GameData.h"
#include "SpriteSet.h"

#include <cmath>

using namespace std;

namespace {
	static const double EPS = 0.0000000001;
}



void Outfit::Load(const DataNode &node)
{
	if(node.Size() >= 2)
		name = node.Token(1);
	
	for(const DataNode &child : node)
	{
		if(child.Token(0) == "category" && child.Size() >= 2)
			category = child.Token(1);
		else if(child.Token(0) == "flare sprite" && child.Size() >= 2)
		{
			flareSprites.emplace_back(Animation(), 1);
			flareSprites.back().first.Load(child);
		}
		else if(child.Token(0) == "flare sound" && child.Size() >= 2)
			++flareSounds[Audio::Get(child.Token(1))];
		else if(child.Token(0) == "afterburner effect" && child.Size() >= 2)
			++afterburnerEffects[GameData::Effects().Get(child.Token(1))];
		else if(child.Token(0) == "thumbnail" && child.Size() >= 2)
			thumbnail = SpriteSet::Get(child.Token(1));
		else if(child.Token(0) == "weapon")
			LoadWeapon(child);
		else if(child.Token(0) == "description" && child.Size() >= 2)
		{
			description += child.Token(1);
			description += '\n';
		}
		else if(child.Size() >= 2)
			attributes[child.Token(0)] = child.Value(1);
		else
			child.PrintTrace("Skipping unrecognized attribute:");
	}
}



const string &Outfit::Name() const
{
	return name;
}



const string &Outfit::Category() const
{
	return category;
}



const string &Outfit::Description() const
{
	return description;
}



int64_t Outfit::Cost() const
{
	return Get("cost");
}



// Get the image to display in the outfitter when buying this item.
const Sprite *Outfit::Thumbnail() const
{
	return thumbnail;
}



double Outfit::Get(const string &attribute) const
{
	auto it = attributes.find(attribute);
	return (it == attributes.end()) ? 0. : it->second;
}



const map<string, double> &Outfit::Attributes() const
{
	return attributes;
}



// Determine whether the given number of instances of the given outfit can
// be added to a ship with the attributes represented by this instance. If
// not, return the maximum number that can be added.
int Outfit::CanAdd(const Outfit &other, int count) const
{
	for(const auto &at : other.attributes)
	{
		double value = Get(at.first);
		// Allow for rounding errors:
		if(value + at.second * count < -EPS)
			count = value / -at.second + EPS;
	}
	
	return count;
}



// For tracking a combination of outfits in a ship: add the given number of
// instances of the given outfit to this outfit.
void Outfit::Add(const Outfit &other, int count)
{
	for(const auto &at : other.attributes)
	{
		attributes[at.first] += at.second * count;
		if(fabs(attributes[at.first]) < EPS)
			attributes[at.first] = 0.;
	}
	
	for(const auto &it : other.flareSprites)
	{
		auto oit = flareSprites.begin();
		for( ; oit != flareSprites.end(); ++oit)
			if(oit->first.GetSprite() == it.first.GetSprite())
				break;
		
		if(oit == flareSprites.end())
			flareSprites.emplace_back(it.first, count * it.second);
		else
			oit->second += count * it.second;
	}
	for(const auto &it : other.flareSounds)
		flareSounds[it.first] += count * it.second;
	for(const auto &it : other.afterburnerEffects)
		afterburnerEffects[it.first] += count * it.second;
}



// Modify this outfit's attributes.
void Outfit::Add(const string &attribute, double value)
{
	attributes[attribute] += value;
	if(fabs(attributes[attribute]) < EPS)
		attributes[attribute] = 0.;
}



// Modify this outfit's attributes.
void Outfit::Reset(const string &attribute, double value)
{
	attributes[attribute] = value;
}


	
// Get this outfit's engine flare sprite, if any.
const vector<pair<Animation, int>> &Outfit::FlareSprites() const
{
	return flareSprites;
}



const map<const Sound *, int> &Outfit::FlareSounds() const
{
	return flareSounds;
}



// Get the afterburner effect, if any.
const map<const Effect *, int> &Outfit::AfterburnerEffects() const
{
	return afterburnerEffects;
}
