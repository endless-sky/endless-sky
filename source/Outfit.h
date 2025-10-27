/* Outfit.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include "Dictionary.h"
#include "Paragraphs.h"

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

class Body;
class ConditionsStore;
class DataNode;
class Effect;
class Sound;
class Sprite;
class Weapon;



// Class representing an outfit that can be installed in a ship. A ship's
// "attributes" are simply stored as a series of key-value pairs, and an outfit
// can add to or subtract from any of those values. Weapons also have another
// set of attributes unique to them, and outfits can also specify additional
// information like the sprite to use in the outfitter panel for selling them,
// or the sprite or sound to be used for an engine flare.
class Outfit {
public:
	// These are all the possible category strings for outfits.
	static const std::vector<std::string> CATEGORIES;

	static constexpr double DEFAULT_HYPERDRIVE_COST = 100.;
	static constexpr double DEFAULT_SCRAM_DRIVE_COST = 150.;
	static constexpr double DEFAULT_JUMP_DRIVE_COST = 200.;


public:
	// An "outfit" can be loaded from an "outfit" node or from a ship's
	// "attributes" node.
	void Load(const DataNode &node, const ConditionsStore *playerConditions);
	bool IsDefined() const;

	const std::string &TrueName() const;
	void SetTrueName(const std::string &name);
	const std::string &DisplayName() const;
	const std::string &PluralName() const;
	const std::string &Category() const;
	const std::string &Series() const;
	const int Index() const;
	std::string Description() const;
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
	// Add the licenses required by the given outfit to this outfit.
	void AddLicenses(const Outfit &outfit);
	// Modify this outfit's attributes. Note that this cannot be used to change
	// special attributes, like cost and mass.
	void Set(const char *attribute, double value);

	const std::shared_ptr<const Weapon> &GetWeapon() const;
	// Get the ammo if this is an ammo storage outfit.
	const Outfit *AmmoStored() const;
	// Get the ammo used if this is a weapon, or stored ammo if this is a storage.
	const Outfit *AmmoStoredOrUsed() const;

	// Get this outfit's engine flare sprites, if any.
	const std::vector<std::pair<Body, int>> &FlareSprites() const;
	const std::vector<std::pair<Body, int>> &ReverseFlareSprites() const;
	const std::vector<std::pair<Body, int>> &SteeringFlareSprites() const;
	const std::map<const Sound *, int> &FlareSounds() const;
	const std::map<const Sound *, int> &ReverseFlareSounds() const;
	const std::map<const Sound *, int> &SteeringFlareSounds() const;
	// Get the afterburner effect, if any.
	const std::map<const Effect *, int> &AfterburnerEffects() const;
	// Get this outfit's jump effects and sounds, if any.
	const std::map<const Effect *, int> &JumpEffects() const;
	const std::map<const Sound *, int> &HyperSounds() const;
	const std::map<const Sound *, int> &HyperInSounds() const;
	const std::map<const Sound *, int> &HyperOutSounds() const;
	const std::map<const Sound *, int> &JumpSounds() const;
	const std::map<const Sound *, int> &JumpInSounds() const;
	const std::map<const Sound *, int> &JumpOutSounds() const;
	// Get this outfit's scan sounds, if any.
	const std::map<const Sound *, int> &CargoScanSounds() const;
	const std::map<const Sound *, int> &OutfitScanSounds() const;
	// Get the sprite this outfit uses when dumped into space.
	const Sprite *FlotsamSprite() const;


private:
	// Add the license with the given name to the licenses required by this outfit, if it is not already present.
	void AddLicense(const std::string &name);


private:
	bool isDefined = false;
	std::string trueName;
	std::string displayName;
	std::string pluralName;
	std::string category;
	// The series that this outfit is a part of and its index within that series.
	// Used for sorting within shops.
	std::string series;
	int index = 0;
	Paragraphs description;
	const Sprite *thumbnail = nullptr;
	int64_t cost = 0;
	double mass = 0.;
	// Licenses needed to purchase this item.
	std::vector<std::string> licenses;

	Dictionary attributes;

	std::shared_ptr<const Weapon> weapon;
	// Non-weapon outfits can have ammo so that storage outfits
	// properly remove excess ammo when the storage is sold, instead
	// of blocking the sale of the outfit until the ammo is sold first.
	const Outfit *ammoStored = nullptr;

	// The integers in these pairs/maps indicate the number of
	// sprites/effects/sounds to be placed/played.
	std::vector<std::pair<Body, int>> flareSprites;
	std::vector<std::pair<Body, int>> reverseFlareSprites;
	std::vector<std::pair<Body, int>> steeringFlareSprites;
	std::map<const Sound *, int> flareSounds;
	std::map<const Sound *, int> reverseFlareSounds;
	std::map<const Sound *, int> steeringFlareSounds;
	std::map<const Effect *, int> afterburnerEffects;
	std::map<const Effect *, int> jumpEffects;
	std::map<const Sound *, int> hyperSounds;
	std::map<const Sound *, int> hyperInSounds;
	std::map<const Sound *, int> hyperOutSounds;
	std::map<const Sound *, int> jumpSounds;
	std::map<const Sound *, int> jumpInSounds;
	std::map<const Sound *, int> jumpOutSounds;
	std::map<const Sound *, int> cargoScanSounds;
	std::map<const Sound *, int> outfitScanSounds;
	const Sprite *flotsamSprite = nullptr;
};



// These get called a lot, so inline them for speed.
inline int64_t Outfit::Cost() const { return cost; }
inline double Outfit::Mass() const { return mass; }
inline const std::shared_ptr<const Weapon> &Outfit::GetWeapon() const { return weapon; }
