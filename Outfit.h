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

#include "Animation.h"
#include "Weapon.h"

#include <map>
#include <string>

class DataNode;
class Outfit;
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
	// An "outfit" can be loaded from an "outfit" node or from a ship's
	// "attributes" node.
	void Load(const DataNode &node);
	
	const std::string &Name() const;
	const std::string &Category() const;
	const std::string &Description() const;
	int64_t Cost() const;
	// Get the image to display in the outfitter when buying this item.
	const Sprite *Thumbnail() const;
	
	double Get(const std::string &attribute) const;
	const std::map<std::string, double> &Attributes() const;
	
	// Determine whether the given number of instances of the given outfit can
	// be added to a ship with the attributes represented by this instance. If
	// not, return the maximum number that can be added.
	int CanAdd(const Outfit &other, int count = 1) const;
	// For tracking a combination of outfits in a ship: add the given number of
	// instances of the given outfit to this outfit.
	void Add(const Outfit &other, int count = 1);
	// Modify this outfit's attributes.
	void Add(const std::string &attribute, double value = 1.);
	void Reset(const std::string &attribute, double value = 0.);
	
	// Get this outfit's engine flare sprite, if any.
	const Animation &FlareSprite() const;
	const Sound *FlareSound() const;
	
	
private:
	std::string name;
	std::string category;
	std::string description;
	const Sprite *thumbnail = nullptr;
	
	std::map<std::string, double> attributes;
	
	Animation flare;
	const Sound *flareSound = nullptr;
};



#endif
