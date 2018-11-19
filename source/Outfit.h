/* Outfit.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef OUTFIT_H_
#define OUTFIT_H_

#include "Weapon.h"

#include "Dictionary.h"

#include <map>
#include <string>
#include <utility>
#include <vector>

class Body;
class DataNode;
class Effect;
class Sound;
class Sprite;



// Class representing an outfit that can be installed in a ship. A ship's
// "attributes" are simply stored as a series of key-value pairs, and an outfit
// can add to or subtract from any of those values. Weapons also have another
// set of attributes unique to them, and outfits can also specify additional
// information like the sprite to use in the outfitter panel for selling them,
// or the sprite or sound to be used for an engine flare.
class Outfit : public Weapon {
public:
	// These are all the possible category strings for outfits.
	static const std::vector<std::string> CATEGORIES;
	
public:
	// An "outfit" can be loaded from an "outfit" node or from a ship's
	// "attributes" node.
	void Load(const DataNode &node);
	
	const std::string &Name() const;
	const std::string &PluralName() const;
	const std::string &Category() const;
	const std::string &Description() const;
	int64_t Cost() const;
	double Mass() const;
	// Get the licenses needed to buy or operate this ship.
	const std::vector<std::string> &Licenses() const;
	// Get the image to display in the outfitter when buying this item.
	const Sprite *Thumbnail() const;
	
	double Get(const char *attribute) const;
	double Get(const std::string &attribute) const;
	const Dictionary &Attributes() const;
	
	// Determine whether the given number of instances of the given outfit can
	// be added to a ship with the attributes represented by this instance. If
	// not, return the maximum number that can be added.
	int CanAdd(const Outfit &other, int count = 1) const;
	// For tracking a combination of outfits in a ship: add the given number of
	// instances of the given outfit to this outfit.
	void Add(const Outfit &other, int count = 1);
	// Modify this outfit's attributes. Note that this cannot be used to change
	// special attributes, like cost and mass.
	void Set(const char *attribute, double value);
	
	// Get this outfit's engine flare sprites, if any.
	const std::vector<std::pair<Body, int>> &FlareSprites() const;
	const std::map<const Sound *, int> &FlareSounds() const;
	// Get the afterburner effect, if any.
	const std::map<const Effect *, int> &AfterburnerEffects() const;
	// Get the sprite this outfit uses when dumped into space.
	const Sprite *FlotsamSprite() const;
	
	
private:
	std::string name;
	std::string pluralName;
	std::string category;
	std::string description;
	const Sprite *thumbnail = nullptr;
	int64_t cost = 0;
	double mass = 0.;
	// Licenses needed to purchase this item.
	std::vector<std::string> licenses;
	
	Dictionary attributes;
	
	std::vector<std::pair<Body, int>> flareSprites;
	std::map<const Sound *, int> flareSounds;
	std::map<const Effect *, int> afterburnerEffects;
	const Sprite *flotsamSprite = nullptr;
};



// These get called a lot, so inline them for speed.
inline int64_t Outfit::Cost() const { return cost; }
inline double Outfit::Mass() const { return mass; }



#endif
