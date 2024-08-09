/* Outfit.cpp
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

#include "Outfit.h"

#include "audio/Audio.h"
#include "Body.h"
#include "DataNode.h"
#include "Effect.h"
#include "GameData.h"
#include "SpriteSet.h"

#include <algorithm>
#include <cmath>
#include <cstring>

using namespace std;

namespace {
	void AddFlareSprites(vector<pair<Body, int>> &thisFlares, const pair<Body, int> &it, int count)
	{
		auto oit = find_if(thisFlares.begin(), thisFlares.end(),
			[&it](const pair<Body, int> &flare)
			{
				return (it.first.GetSprite() == flare.first.GetSprite()
					&& it.first.Scale() == flare.first.Scale());
			}
		);

		if(oit == thisFlares.end())
			thisFlares.emplace_back(it.first, count * it.second);
		else
			oit->second += count * it.second;
	}

	// Used to add the contents of one outfit's map to another, while also
	// erasing any key with a value of zero.
	template <class T>
	void MergeMaps(map<const T *, int> &thisMap, const map<const T *, int> &otherMap, int count)
	{
		for(const auto &it : otherMap)
		{
			thisMap[it.first] += count * it.second;
			if(thisMap[it.first] == 0)
				thisMap.erase(it.first);
		}
	}
}



void Outfit::Load(const DataNode &node)
{
	if(node.Size() >= 2)
		trueName = node.Token(1);

	isDefined = true;

	for(const DataNode &child : node)
	{
		if(child.Token(0) == "display name" && child.Size() >= 2)
			displayName = child.Token(1);
		else if(child.Token(0) == "category" && child.Size() >= 2)
			category = child.Token(1);
		else if(child.Token(0) == "series" && child.Size() >= 2)
			series = child.Token(1);
		else if(child.Token(0) == "index" && child.Size() >= 2)
			index = child.Value(1);
		else if(child.Token(0) == "plural" && child.Size() >= 2)
			pluralName = child.Token(1);
		else if(child.Token(0) == "flare sprite" && child.Size() >= 2)
		{
			flareSprites.emplace_back(Body(), 1);
			flareSprites.back().first.LoadSprite(child);
		}
		else if(child.Token(0) == "reverse flare sprite" && child.Size() >= 2)
		{
			reverseFlareSprites.emplace_back(Body(), 1);
			reverseFlareSprites.back().first.LoadSprite(child);
		}
		else if(child.Token(0) == "steering flare sprite" && child.Size() >= 2)
		{
			steeringFlareSprites.emplace_back(Body(), 1);
			steeringFlareSprites.back().first.LoadSprite(child);
		}
		else if(child.Token(0) == "flare sound" && child.Size() >= 2)
			++flareSounds[Audio::Get(child.Token(1))];
		else if(child.Token(0) == "reverse flare sound" && child.Size() >= 2)
			++reverseFlareSounds[Audio::Get(child.Token(1))];
		else if(child.Token(0) == "steering flare sound" && child.Size() >= 2)
			++steeringFlareSounds[Audio::Get(child.Token(1))];
		else if(child.Token(0) == "afterburner effect" && child.Size() >= 2)
			++afterburnerEffects[GameData::Effects().Get(child.Token(1))];
		else if(child.Token(0) == "jump effect" && child.Size() >= 2)
			++jumpEffects[GameData::Effects().Get(child.Token(1))];
		else if(child.Token(0) == "hyperdrive sound" && child.Size() >= 2)
			++hyperSounds[Audio::Get(child.Token(1))];
		else if(child.Token(0) == "hyperdrive in sound" && child.Size() >= 2)
			++hyperInSounds[Audio::Get(child.Token(1))];
		else if(child.Token(0) == "hyperdrive out sound" && child.Size() >= 2)
			++hyperOutSounds[Audio::Get(child.Token(1))];
		else if(child.Token(0) == "jump sound" && child.Size() >= 2)
			++jumpSounds[Audio::Get(child.Token(1))];
		else if(child.Token(0) == "jump in sound" && child.Size() >= 2)
			++jumpInSounds[Audio::Get(child.Token(1))];
		else if(child.Token(0) == "jump out sound" && child.Size() >= 2)
			++jumpOutSounds[Audio::Get(child.Token(1))];
		else if(child.Token(0) == "cargo scan sound" && child.Size() >= 2)
			++cargoScanSounds[Audio::Get(child.Token(1))];
		else if(child.Token(0) == "outfit scan sound" && child.Size() >= 2)
			++outfitScanSounds[Audio::Get(child.Token(1))];
		else if(child.Token(0) == "flotsam sprite" && child.Size() >= 2)
			flotsamSprite = SpriteSet::Get(child.Token(1));
		else if(child.Token(0) == "thumbnail" && child.Size() >= 2)
			thumbnail = SpriteSet::Get(child.Token(1));
		else if(child.Token(0) == "weapon")
			LoadWeapon(child);
		else if(child.Token(0) == "ammo" && child.Size() >= 2)
		{
			// Non-weapon outfits can have ammo so that storage outfits
			// properly remove excess ammo when the storage is sold, instead
			// of blocking the sale of the outfit until the ammo is sold first.
			ammo = make_pair(GameData::Outfits().Get(child.Token(1)), 0);
		}
		else if(child.Token(0) == "description" && child.Size() >= 2)
		{
			description += child.Token(1);
			description += '\n';
		}
		else if(child.Token(0) == "cost" && child.Size() >= 2)
			cost = child.Value(1);
		else if(child.Token(0) == "mass" && child.Size() >= 2)
			mass = child.Value(1);
		else if(child.Token(0) == "licenses" && (child.HasChildren() || child.Size() >= 2))
		{
			// Add any new licenses that were specified "inline".
			if(child.Size() >= 2)
			{
				for(auto it = ++begin(child.Tokens()); it != end(child.Tokens()); ++it)
					AddLicense(*it);
			}
			// Add any new licenses that were specified as an indented list.
			for(const DataNode &grand : child)
				AddLicense(grand.Token(0));
		}
		else if(child.Token(0) == "jump range" && child.Size() >= 2)
		{
			// Jump range must be positive.
			attributes.Set(child.Token(0), max(0., child.Value(1)));
		}
		else
			attributes.Load(child);
	}

	if(displayName.empty())
		displayName = trueName;

	// If no plural name has been defined, append an 's' to the name and use that.
	// If the name ends in an 's' or 'z', and no plural name has been defined, print a
	// warning since an explicit plural name is always required in this case.
	// Unless this outfit definition isn't declared with the `outfit` keyword,
	// because then this is probably being done in `add attributes` on a ship,
	// so the name doesn't matter.
	if(!displayName.empty() && pluralName.empty())
	{
		pluralName = displayName + 's';
		if((displayName.back() == 's' || displayName.back() == 'z') && node.Token(0) == "outfit")
			node.PrintTrace("Warning: explicit plural name definition required, but none is provided. Defaulting to \""
					+ pluralName + "\".");
	}

	// Only outfits with the jump drive and jump range attributes can
	// use the jump range, so only keep track of the jump range on
	// viable outfits.
	if(attributes.Get("jump drive") && attributes.Get("jump range"))
		GameData::AddJumpRange(attributes.Get("jump range"));

	// Legacy support for turrets that don't specify a turn rate:
	if(IsWeapon() && attributes.Get("turret mounts") && !TurretTurn()
		&& !AntiMissile() && !TractorBeam())
	{
		SetTurretTurn(4.);
		node.PrintTrace("Warning: Deprecated use of a turret without specified \"turret turn\":");
	}
	// Convert any legacy cargo / outfit scan definitions into power & speed,
	// so no runtime code has to check for both.
	auto convertScan = [&](string &&kind) -> void
	{
		string label = kind + " scan";
		double initial = attributes.Get(label);
		if(initial)
		{
			attributes.Set(label, 0.);
			node.PrintTrace("Warning: Deprecated use of \"" + label + "\" instead of \""
					+ label + " power\" and \"" + label + " speed\":");

			// A scan value of 300 is equivalent to a scan power of 9.
			const double power = initial * initial * .0001;
			attributes.Add(label + " power", power);
			// The default scan speed of 1 is unrelated to the magnitude of the scan value.
			// It may have been already specified, and if so, should not be increased.
			if(!attributes.Get(label + " efficiency"))
				attributes.Set(label + " efficiency", 15.);
		}

		// Similar check for scan speed which is replaced with scan efficiency.
		label += " speed";
		initial = attributes.Get(label);
		if(initial)
		{
			attributes.Set(label, 0.);
			node.PrintTrace("Warning: Deprecated use of \"" + label + "\" instead of \""
					+ kind + " scan efficiency\":");
			// A reasonable update is 15x the previous value, as the base scan time
			// is 10x what it was before scan efficiency was introduced, along with
			// ships which are larger or further away also increasing the scan time.
			const double efficiency = initial * 15.;
			attributes.Add(kind + " scan efficiency", efficiency);
		}
	};
	convertScan("outfit");
	convertScan("cargo");
}



// Check if this outfit has been defined via Outfit::Load (vs. only being referred to).
bool Outfit::IsDefined() const
{
	return isDefined;
}



// When writing to the player's save, the reference name is used even if this
// outfit was not fully defined (i.e. belongs to an inactive plugin).
const string &Outfit::TrueName() const
{
	return trueName;
}



const string &Outfit::DisplayName() const
{
	return displayName;
}



void Outfit::SetName(const string &name)
{
	this->trueName = name;
}



const string &Outfit::PluralName() const
{
	return pluralName;
}



const string &Outfit::Category() const
{
	return category;
}



const string &Outfit::Series() const
{
	return series;
}



const int Outfit::Index() const
{
	return index;
}



const string &Outfit::Description() const
{
	return description;
}



// Get the licenses needed to purchase this outfit.
const vector<string> &Outfit::Licenses() const
{
	return licenses;
}



// Get the image to display in the outfitter when buying this item.
const Sprite *Outfit::Thumbnail() const
{
	return thumbnail;
}



// Determine whether the given number of instances of the given outfit can
// be added to a ship with the attributes represented by this instance. If
// not, return the maximum number that can be added.
int Outfit::CanAdd(const Outfit &other, int count) const
{
	return attributes.CanAdd(other.attributes, count);
}



// For tracking a combination of outfits in a ship: add the given number of
// instances of the given outfit to this outfit.
void Outfit::Add(const Outfit &other, int count)
{
	cost += other.cost * count;
	mass += other.mass * count;
	attributes.Add(other.attributes, count);
	for(const auto &it : other.flareSprites)
		AddFlareSprites(flareSprites, it, count);
	for(const auto &it : other.reverseFlareSprites)
		AddFlareSprites(reverseFlareSprites, it, count);
	for(const auto &it : other.steeringFlareSprites)
		AddFlareSprites(steeringFlareSprites, it, count);
	MergeMaps(flareSounds, other.flareSounds, count);
	MergeMaps(reverseFlareSounds, other.reverseFlareSounds, count);
	MergeMaps(steeringFlareSounds, other.steeringFlareSounds, count);
	MergeMaps(afterburnerEffects, other.afterburnerEffects, count);
	MergeMaps(jumpEffects, other.jumpEffects, count);
	MergeMaps(hyperSounds, other.hyperSounds, count);
	MergeMaps(hyperInSounds, other.hyperInSounds, count);
	MergeMaps(hyperOutSounds, other.hyperOutSounds, count);
	MergeMaps(jumpSounds, other.jumpSounds, count);
	MergeMaps(jumpInSounds, other.jumpInSounds, count);
	MergeMaps(jumpOutSounds, other.jumpOutSounds, count);
	MergeMaps(cargoScanSounds, other.cargoScanSounds, count);
	MergeMaps(outfitScanSounds, other.outfitScanSounds, count);
}



void Outfit::AddLicenses(const Outfit &other)
{
	for(const auto &license : other.licenses)
		AddLicense(license);
}



// Modify this outfit's attributes.
void Outfit::Set(const char *attribute, double value)
{
	attributes.Set(attribute, value);
}



// Get this outfit's engine flare sprite, if any.
const vector<pair<Body, int>> &Outfit::FlareSprites() const
{
	return flareSprites;
}



const vector<pair<Body, int>> &Outfit::ReverseFlareSprites() const
{
	return reverseFlareSprites;
}



const vector<pair<Body, int>> &Outfit::SteeringFlareSprites() const
{
	return steeringFlareSprites;
}



const map<const Sound *, int> &Outfit::FlareSounds() const
{
	return flareSounds;
}



const map<const Sound *, int> &Outfit::ReverseFlareSounds() const
{
	return reverseFlareSounds;
}



const map<const Sound *, int> &Outfit::SteeringFlareSounds() const
{
	return steeringFlareSounds;
}



// Get the afterburner effect, if any.
const map<const Effect *, int> &Outfit::AfterburnerEffects() const
{
	return afterburnerEffects;
}



// Get this outfit's jump effects and sounds, if any.
const map<const Effect *, int> &Outfit::JumpEffects() const
{
	return jumpEffects;
}



const map<const Sound *, int> &Outfit::HyperSounds() const
{
	return hyperSounds;
}



const map<const Sound *, int> &Outfit::HyperInSounds() const
{
	return hyperInSounds;
}



const map<const Sound *, int> &Outfit::HyperOutSounds() const
{
	return hyperOutSounds;
}



const map<const Sound *, int> &Outfit::JumpSounds() const
{
	return jumpSounds;
}



const map<const Sound *, int> &Outfit::JumpInSounds() const
{
	return jumpInSounds;
}



const map<const Sound *, int> &Outfit::JumpOutSounds() const
{
	return jumpOutSounds;
}



const map<const Sound *, int> &Outfit::CargoScanSounds() const
{
	return cargoScanSounds;
}



const map<const Sound *, int> &Outfit::OutfitScanSounds() const
{
	return outfitScanSounds;
}



// Get the sprite this outfit uses when dumped into space.
const Sprite *Outfit::FlotsamSprite() const
{
	return flotsamSprite;
}



// Add the license with the given name to the licenses required by this outfit, if it is not already present.
void Outfit::AddLicense(const string &name)
{
	const auto it = find(licenses.begin(), licenses.end(), name);
	if(it == licenses.end())
		licenses.push_back(name);
}
