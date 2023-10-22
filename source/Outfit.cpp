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

#include "Audio.h"
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
	const double EPS = 0.0000000001;

	// A mapping of attribute names to specifically-allowed minimum values. Based on the
	// specific usage of the attribute, the allowed minimum value is chosen to avoid
	// disallowed or undesirable behaviors (such as dividing by zero).
	const auto MINIMUM_OVERRIDES = map<string, double>{
		// Attributes which are present and map to zero may have any value.
		{"shield energy", 0.},
		{"shield fuel", 0.},
		{"shield heat", 0.},
		{"hull energy", 0.},
		{"hull fuel", 0.},
		{"hull heat", 0.},
		{"hull threshold", 0.},
		{"energy generation", 0.},
		{"energy consumption", 0.},
		{"fuel generation", 0.},
		{"fuel consumption", 0.},
		{"fuel energy", 0.},
		{"fuel heat", 0.},
		{"heat generation", 0.},
		{"flotsam chance", 0.},

		{"thrusting shields", 0.},
		{"thrusting hull", 0.},
		{"thrusting energy", 0.},
		{"thrusting fuel", 0.},
		{"thrusting heat", 0.},
		{"thrusting discharge", 0.},
		{"thrusting corrosion", 0.},
		{"thrusting ion", 0.},
		{"thrusting leakage", 0.},
		{"thrusting burn", 0.},
		{"thrusting disruption", 0.},
		{"thrusting slowing", 0.},

		{"turning shields", 0.},
		{"turning hull", 0.},
		{"turning energy", 0.},
		{"turning fuel", 0.},
		{"turning heat", 0.},
		{"turning discharge", 0.},
		{"turning corrosion", 0.},
		{"turning ion", 0.},
		{"turning leakage", 0.},
		{"turning burn", 0.},
		{"turning disruption", 0.},
		{"turning slowing", 0.},

		{"reverse thrusting shields", 0.},
		{"reverse thrusting hull", 0.},
		{"reverse thrusting energy", 0.},
		{"reverse thrusting fuel", 0.},
		{"reverse thrusting heat", 0.},
		{"reverse thrusting discharge", 0.},
		{"reverse thrusting corrosion", 0.},
		{"reverse thrusting ion", 0.},
		{"reverse thrusting leakage", 0.},
		{"reverse thrusting burn", 0.},
		{"reverse thrusting disruption", 0.},
		{"reverse thrusting slowing", 0.},

		{"afterburner shields", 0.},
		{"afterburner hull", 0.},
		{"afterburner energy", 0.},
		{"afterburner fuel", 0.},
		{"afterburner heat", 0.},
		{"afterburner discharge", 0.},
		{"afterburner corrosion", 0.},
		{"afterburner ion", 0.},
		{"afterburner leakage", 0.},
		{"afterburner burn", 0.},
		{"afterburner disruption", 0.},
		{"afterburner slowing", 0.},

		{"cooling energy", 0.},
		{"discharge resistance energy", 0.},
		{"discharge resistance fuel", 0.},
		{"discharge resistance heat", 0.},
		{"corrosion resistance energy", 0.},
		{"corrosion resistance fuel", 0.},
		{"corrosion resistance heat", 0.},
		{"ion resistance energy", 0.},
		{"ion resistance fuel", 0.},
		{"ion resistance heat", 0.},
		{"scramble resistance energy", 0.},
		{"scramble resistance fuel", 0.},
		{"scramble resistance heat", 0.},
		{"leak resistance energy", 0.},
		{"leak resistance fuel", 0.},
		{"leak resistance heat", 0.},
		{"burn resistance energy", 0.},
		{"burn resistance fuel", 0.},
		{"burn resistance heat", 0.},
		{"disruption resistance energy", 0.},
		{"disruption resistance fuel", 0.},
		{"disruption resistance heat", 0.},
		{"slowing resistance energy", 0.},
		{"slowing resistance fuel", 0.},
		{"slowing resistance heat", 0.},
		{"crew equivalent", 0.},

		// "Protection" attributes appear in denominators and are incremented by 1.
		{"shield protection", -0.99},
		{"hull protection", -0.99},
		{"energy protection", -0.99},
		{"fuel protection", -0.99},
		{"heat protection", -0.99},
		{"piercing protection", -0.99},
		{"force protection", -0.99},
		{"discharge protection", -0.99},
		{"drag reduction", -0.99},
		{"corrosion protection", -0.99},
		{"inertia reduction", -0.99},
		{"ion protection", -0.99},
		{"scramble protection", -0.99},
		{"leak protection", -0.99},
		{"burn protection", -0.99},
		{"disruption protection", -0.99},
		{"slowing protection", -0.99},

		// "Multiplier" attributes appear in numerators and are incremented by 1.
		{"hull multiplier", -1. },
		{"hull repair multiplier", -1.},
		{"hull energy multiplier", -1.},
		{"hull fuel multiplier", -1.},
		{"hull heat multiplier", -1.},
		{"shield multiplier", -1. },
		{"shield generation multiplier", -1.},
		{"shield energy multiplier", -1.},
		{"shield fuel multiplier", -1.},
		{"shield heat multiplier", -1.},
		{"acceleration multiplier", -1.},
		{"turn multiplier", -1.}
	};

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
			auto isNewLicense = [](const vector<string> &c, const string &val) noexcept -> bool {
				return find(c.begin(), c.end(), val) == c.end();
			};
			// Add any new licenses that were specified "inline".
			if(child.Size() >= 2)
			{
				for(auto it = ++begin(child.Tokens()); it != end(child.Tokens()); ++it)
					if(isNewLicense(licenses, *it))
						licenses.push_back(*it);
			}
			// Add any new licenses that were specified as an indented list.
			for(const DataNode &grand : child)
				if(isNewLicense(licenses, grand.Token(0)))
					licenses.push_back(grand.Token(0));
		}
		else if(child.Token(0) == "jump range" && child.Size() >= 2)
		{
			// Jump range must be positive.
			attributes[child.Token(0)] = max(0., child.Value(1));
		}
		else if(child.Size() >= 2)
			attributes[child.Token(0)] = child.Value(1);
		else
			child.PrintTrace("Skipping unrecognized attribute:");
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
	if(IsWeapon() && attributes.Get("turret mounts") && !TurretTurn() && !AntiMissile())
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
			attributes[label] = 0.;
			node.PrintTrace("Warning: Deprecated use of \"" + label + "\" instead of \""
					+ label + " power\" and \"" + label + " speed\":");

			// A scan value of 300 is equivalent to a scan power of 9.
			attributes[label + " power"] += initial * initial * .0001;
			// The default scan speed of 1 is unrelated to the magnitude of the scan value.
			// It may have been already specified, and if so, should not be increased.
			if(!attributes.Get(label + " efficiency"))
				attributes[label + " efficiency"] = 15.;
		}

		// Similar check for scan speed which is replaced with scan efficiency.
		label += " speed";
		initial = attributes.Get(label);
		if(initial)
		{
			attributes[label] = 0.;
			node.PrintTrace("Warning: Deprecated use of \"" + label + "\" instead of \""
					+ kind + " scan efficiency\":");
			// A reasonable update is 15x the previous value, as the base scan time
			// is 10x what it was before scan efficiency was introduced, along with
			// ships which are larger or further away also increasing the scan time.
			attributes[kind + " scan efficiency"] += initial * 15.;
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



double Outfit::Get(const char *attribute) const
{
	return attributes.Get(attribute);
}



double Outfit::Get(const string &attribute) const
{
	return Get(attribute.c_str());
}



const Dictionary &Outfit::Attributes() const
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
		// The minimum allowed value of most attributes is 0. Some attributes
		// have special functionality when negative, though, and are therefore
		// allowed to have values less than 0.
		double minimum = 0.;
		auto it = MINIMUM_OVERRIDES.find(at.first);
		if(it != MINIMUM_OVERRIDES.end())
		{
			minimum = it->second;
			// An override of exactly 0 means the attribute may have any value.
			if(!minimum)
				continue;
		}

		// Only automatons may have a "required crew" of 0.
		if(!strcmp(at.first, "required crew"))
			minimum = !(attributes.Get("automaton") || other.attributes.Get("automaton"));

		double value = Get(at.first);
		// Allow for rounding errors:
		if(value + at.second * count < minimum - EPS)
			count = (value - minimum) / -at.second + EPS;
	}

	return count;
}



// For tracking a combination of outfits in a ship: add the given number of
// instances of the given outfit to this outfit.
void Outfit::Add(const Outfit &other, int count)
{
	cost += other.cost * count;
	mass += other.mass * count;
	for(const auto &at : other.attributes)
	{
		attributes[at.first] += at.second * count;
		if(fabs(attributes[at.first]) < EPS)
			attributes[at.first] = 0.;
	}

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
}



// Modify this outfit's attributes.
void Outfit::Set(const char *attribute, double value)
{
	attributes[attribute] = value;
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



// Get the sprite this outfit uses when dumped into space.
const Sprite *Outfit::FlotsamSprite() const
{
	return flotsamSprite;
}
