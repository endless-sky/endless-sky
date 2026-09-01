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
#include "image/SpriteSet.h"
#include "Weapon.h"

#include <algorithm>
#include <cmath>
#include <cstring>

using namespace std;

namespace {
	// Attributes are stored as integers but used as doubles. This is the factor by which to convert
	// attribute values from doubles to integers and back. Using a base 10 scaling factor instead of
	// a base 2 scaling factor so that values provided in game data text files match the values that
	// get used as closely as possible. Commonly used attribute values get cached anyway, so we care
	// more about accuracy to what the content creator provided than speed of converting values.
	constexpr int ATTRIBUTE_PRECISION = 10000;

	// A mapping of attribute names to specifically-allowed minimum values. Based on the
	// specific usage of the attribute, the allowed minimum value is chosen to avoid
	// disallowed or undesirable behaviors (such as dividing by zero).
	const auto MINIMUM_OVERRIDES = map<string, optional<int64_t>>{
		// Attributes which are present and map to nullopt may have any value.
		{"shield energy", std::nullopt},
		{"shield fuel", std::nullopt},
		{"shield heat", std::nullopt},
		{"hull energy", std::nullopt},
		{"hull fuel", std::nullopt},
		{"hull heat", std::nullopt},
		{"hull threshold", std::nullopt},
		{"energy generation", std::nullopt},
		{"energy consumption", std::nullopt},
		{"fuel generation", std::nullopt},
		{"fuel consumption", std::nullopt},
		{"fuel energy", std::nullopt},
		{"fuel heat", std::nullopt},
		{"heat generation", std::nullopt},
		{"flotsam chance", std::nullopt},

		{"thrusting shields", std::nullopt},
		{"thrusting hull", std::nullopt},
		{"thrusting energy", std::nullopt},
		{"thrusting fuel", std::nullopt},
		{"thrusting heat", std::nullopt},
		{"thrusting discharge", std::nullopt},
		{"thrusting corrosion", std::nullopt},
		{"thrusting ion", std::nullopt},
		{"thrusting leakage", std::nullopt},
		{"thrusting burn", std::nullopt},
		{"thrusting disruption", std::nullopt},
		{"thrusting slowing", std::nullopt},

		{"turning shields", std::nullopt},
		{"turning hull", std::nullopt},
		{"turning energy", std::nullopt},
		{"turning fuel", std::nullopt},
		{"turning heat", std::nullopt},
		{"turning discharge", std::nullopt},
		{"turning corrosion", std::nullopt},
		{"turning ion", std::nullopt},
		{"turning leakage", std::nullopt},
		{"turning burn", std::nullopt},
		{"turning disruption", std::nullopt},
		{"turning slowing", std::nullopt},

		{"reverse thrusting shields", std::nullopt},
		{"reverse thrusting hull", std::nullopt},
		{"reverse thrusting energy", std::nullopt},
		{"reverse thrusting fuel", std::nullopt},
		{"reverse thrusting heat", std::nullopt},
		{"reverse thrusting discharge", std::nullopt},
		{"reverse thrusting corrosion", std::nullopt},
		{"reverse thrusting ion", std::nullopt},
		{"reverse thrusting leakage", std::nullopt},
		{"reverse thrusting burn", std::nullopt},
		{"reverse thrusting disruption", std::nullopt},
		{"reverse thrusting slowing", std::nullopt},

		{"afterburner shields", std::nullopt},
		{"afterburner hull", std::nullopt},
		{"afterburner energy", std::nullopt},
		{"afterburner fuel", std::nullopt},
		{"afterburner heat", std::nullopt},
		{"afterburner discharge", std::nullopt},
		{"afterburner corrosion", std::nullopt},
		{"afterburner ion", std::nullopt},
		{"afterburner leakage", std::nullopt},
		{"afterburner burn", std::nullopt},
		{"afterburner disruption", std::nullopt},
		{"afterburner slowing", std::nullopt},

		{"cooling energy", std::nullopt},
		{"discharge resistance energy", std::nullopt},
		{"discharge resistance fuel", std::nullopt},
		{"discharge resistance heat", std::nullopt},
		{"corrosion resistance energy", std::nullopt},
		{"corrosion resistance fuel", std::nullopt},
		{"corrosion resistance heat", std::nullopt},
		{"ion resistance energy", std::nullopt},
		{"ion resistance fuel", std::nullopt},
		{"ion resistance heat", std::nullopt},
		{"scramble resistance energy", std::nullopt},
		{"scramble resistance fuel", std::nullopt},
		{"scramble resistance heat", std::nullopt},
		{"leak resistance energy", std::nullopt},
		{"leak resistance fuel", std::nullopt},
		{"leak resistance heat", std::nullopt},
		{"burn resistance energy", std::nullopt},
		{"burn resistance fuel", std::nullopt},
		{"burn resistance heat", std::nullopt},
		{"disruption resistance energy", std::nullopt},
		{"disruption resistance fuel", std::nullopt},
		{"disruption resistance heat", std::nullopt},
		{"slowing resistance energy", std::nullopt},
		{"slowing resistance fuel", std::nullopt},
		{"slowing resistance heat", std::nullopt},
		{"crew equivalent", std::nullopt},

		{"cloaking energy", std::nullopt},
		{"cloaking fuel", std::nullopt},
		{"cloaking heat", std::nullopt},
		{"cloaking hull", std::nullopt},
		{"cloaking repair delay", std::nullopt},
		{"cloaking shields", std::nullopt},
		{"cloaking shield delay", std::nullopt},
		{"cloaked firing", std::nullopt},

		// "Protection" attributes appear in denominators and are incremented by 1.
		{"shield protection", -0.99 * ATTRIBUTE_PRECISION},
		{"hull protection", -0.99 * ATTRIBUTE_PRECISION},
		{"energy protection", -0.99 * ATTRIBUTE_PRECISION},
		{"fuel protection", -0.99 * ATTRIBUTE_PRECISION},
		{"heat protection", -0.99 * ATTRIBUTE_PRECISION},
		{"piercing protection", -0.99 * ATTRIBUTE_PRECISION},
		{"force protection", -0.99 * ATTRIBUTE_PRECISION},
		{"discharge protection", -0.99 * ATTRIBUTE_PRECISION},
		{"drag reduction", -0.99 * ATTRIBUTE_PRECISION},
		{"corrosion protection", -0.99 * ATTRIBUTE_PRECISION},
		{"inertia reduction", -0.99 * ATTRIBUTE_PRECISION},
		{"ion protection", -0.99 * ATTRIBUTE_PRECISION},
		{"scramble protection", -0.99 * ATTRIBUTE_PRECISION},
		{"leak protection", -0.99 * ATTRIBUTE_PRECISION},
		{"burn protection", -0.99 * ATTRIBUTE_PRECISION},
		{"disruption protection", -0.99 * ATTRIBUTE_PRECISION},
		{"slowing protection", -0.99 * ATTRIBUTE_PRECISION},

		// "Multiplier" attributes appear in numerators and are incremented by 1.
		{"hull multiplier", -1. * ATTRIBUTE_PRECISION},
		{"hull repair multiplier", -1. * ATTRIBUTE_PRECISION},
		{"hull energy multiplier", -1. * ATTRIBUTE_PRECISION},
		{"hull fuel multiplier", -1. * ATTRIBUTE_PRECISION},
		{"hull heat multiplier", -1. * ATTRIBUTE_PRECISION},
		{"cloaked repair multiplier", -1. * ATTRIBUTE_PRECISION},
		{"shield multiplier", -1. * ATTRIBUTE_PRECISION},
		{"shield generation multiplier", -1. * ATTRIBUTE_PRECISION},
		{"shield energy multiplier", -1. * ATTRIBUTE_PRECISION},
		{"shield fuel multiplier", -1. * ATTRIBUTE_PRECISION},
		{"shield heat multiplier", -1. * ATTRIBUTE_PRECISION},
		{"cloaked regen multiplier", -1. * ATTRIBUTE_PRECISION},
		{"acceleration multiplier", -1. * ATTRIBUTE_PRECISION},
		{"turn multiplier", -1. * ATTRIBUTE_PRECISION},
		{"turret turn multiplier", -1. * ATTRIBUTE_PRECISION},
	};

	void AddFlareSprites(vector<pair<Drawable, int>> &thisFlares, const pair<Drawable, int> &it, int count)
	{
		auto oit = find_if(thisFlares.begin(), thisFlares.end(),
			[&it](const pair<Drawable, int> &flare)
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
	template<class T, class N>
	void MergeMaps(map<const T *, N> &thisMap, const map<const T *, N> &otherMap, int count)
	{
		for(const auto &it : otherMap)
		{
			thisMap[it.first] += count * it.second;
			if(thisMap[it.first] == 0)
				thisMap.erase(it.first);
		}
	}
}



optional<double> Outfit::LowerLimit(const string &attribute)
{
	optional<int64_t> precise = LowerLimitPrecise(attribute);
	if(precise.has_value())
		return precise.value() / ATTRIBUTE_PRECISION;
	// No minimum override means a minimum of 0.
	return 0.;
}



optional<int64_t> Outfit::LowerLimitPrecise(const string &attribute)
{
	auto it = MINIMUM_OVERRIDES.find(attribute);
	if(it != MINIMUM_OVERRIDES.end())
		return it->second;
	// No minimum override means a minimum of 0.
	return 0;
}



Outfit::AttributeIterator::AttributeIterator(const Outfit &outfit, Dictionary<int64_t>::const_iterator start)
	: outfit(outfit), it(start)
{
}



pair<string, double> Outfit::AttributeIterator::operator*() const
{
	return make_pair(it->first, outfit.Get(it->first));
}



Outfit::AttributeIterator &Outfit::AttributeIterator::operator++()
{
	if(it != outfit.attributes.end())
		it = next(it);
	return *this;
}



bool Outfit::AttributeIterator::operator==(const AttributeIterator &other) const
{
	return this->it == other.it;
}



bool Outfit::AttributeIterator::operator!=(const AttributeIterator &other) const
{
	return !(*this == other);
}



bool Outfit::AttributeIterator::operator<(const AttributeIterator &other) const
{
	return this->it < other.it;
}



bool Outfit::AttributeIterator::operator>(const AttributeIterator &other) const
{
	return this->it > other.it;
}



void Outfit::Load(const DataNode &node, const ConditionsStore *playerConditions)
{
	if(node.Size() >= 2)
		trueName = node.Token(1);

	isDefined = true;

	for(const DataNode &child : node)
	{
		const string &key = child.Token(0);
		bool hasValue = child.Size() >= 2;

		if(key == "display name" && hasValue)
			displayName = child.Token(1);
		else if(key == "category" && hasValue)
			category = child.Token(1);
		else if(key == "series" && hasValue)
			series = child.Token(1);
		else if(key == "index" && hasValue)
			index = child.Value(1);
		else if(key == "plural" && hasValue)
			pluralName = child.Token(1);
		else if(key == "flare sprite" && hasValue)
		{
			flareSprites.emplace_back(Body(), 1);
			flareSprites.back().first.LoadSprite(child);
		}
		else if(key == "reverse flare sprite" && hasValue)
		{
			reverseFlareSprites.emplace_back(Body(), 1);
			reverseFlareSprites.back().first.LoadSprite(child);
		}
		else if(key == "steering flare sprite" && hasValue)
		{
			steeringFlareSprites.emplace_back(Body(), 1);
			steeringFlareSprites.back().first.LoadSprite(child);
		}
		else if(key == "flare sound" && hasValue)
			++flareSounds[Audio::Get(child.Token(1))];
		else if(key == "reverse flare sound" && hasValue)
			++reverseFlareSounds[Audio::Get(child.Token(1))];
		else if(key == "steering flare sound" && hasValue)
			++steeringFlareSounds[Audio::Get(child.Token(1))];
		else if(key == "afterburner effect" && hasValue)
			++afterburnerEffects[GameData::Effects().Get(child.Token(1))];
		else if(key == "jump effect" && hasValue)
			jumpEffects[GameData::Effects().Get(child.Token(1))] += child.Size() >= 3 ? child.Value(2) : 1.;
		else if(key == "hyperdrive sound" && hasValue)
			++hyperSounds[Audio::Get(child.Token(1))];
		else if(key == "hyperdrive in sound" && hasValue)
			++hyperInSounds[Audio::Get(child.Token(1))];
		else if(key == "hyperdrive out sound" && hasValue)
			++hyperOutSounds[Audio::Get(child.Token(1))];
		else if(key == "jump sound" && hasValue)
			++jumpSounds[Audio::Get(child.Token(1))];
		else if(key == "jump in sound" && hasValue)
			++jumpInSounds[Audio::Get(child.Token(1))];
		else if(key == "jump out sound" && hasValue)
			++jumpOutSounds[Audio::Get(child.Token(1))];
		else if(key == "cargo scan sound" && hasValue)
			++cargoScanSounds[Audio::Get(child.Token(1))];
		else if(key == "outfit scan sound" && hasValue)
			++outfitScanSounds[Audio::Get(child.Token(1))];
		else if(key == "flotsam sprite" && hasValue)
			flotsamSprite = SpriteSet::Get(child.Token(1));
		else if(key == "thumbnail" && hasValue)
			thumbnail.LoadSprite(child);
		else if(key == "weapon")
		{
			if(!weapon)
				weapon = make_shared<Weapon>();
			Weapon newWeapon = *weapon;
			newWeapon.Load(child);
			weapon = make_shared<Weapon>(std::move(newWeapon));
			if(weapon->Ammo())
				linkedOutfits.insert(weapon->Ammo());
		}
		else if(key == "ammo" && hasValue)
		{
			const Outfit *ammo = GameData::Outfits().Get(child.Token(1));
			ammoStored.insert(ammo);
			linkedOutfits.insert(ammo);
		}
		else if(key == "linked" && hasValue)
			linkedOutfits.insert(GameData::Outfits().Get(child.Token(1)));
		else if(key == "description" && hasValue)
			description.Load(child, playerConditions);
		else if(key == "cost" && hasValue)
			cost = child.Value(1);
		else if(key == "mass" && hasValue)
			mass = child.Value(1);
		else if(key == "licenses" && (child.HasChildren() || hasValue))
		{
			// Add any new licenses that were specified "inline".
			if(hasValue)
			{
				for(auto it = ++std::begin(child.Tokens()); it != std::end(child.Tokens()); ++it)
					AddLicense(*it);
			}
			// Add any new licenses that were specified as an indented list.
			for(const DataNode &grand : child)
				AddLicense(grand.Token(0));
		}
		else if(key == "jump range" && hasValue)
		{
			// Jump range must be positive.
			Set(key, max(0., child.Value(1)));
		}
		else if(hasValue)
			Set(key, child.Value(1));
		else
			child.PrintTrace("Skipping unrecognized attribute:");
	}

	if(displayName.empty())
		displayName = trueName;

	// If no plural name has been defined, append an 's' to the name and use that.
	// If the name ends in an 's', 'x', 'z', 'ch', or 'sh', and no plural name has been defined,
	// print a warning since an irregular plural is usually required in this case.
	// Unless this outfit definition isn't declared with a category,
	// because then this is probably being done in `add attributes` on a ship,
	// or it's a pseudo-outfit like submunitions, so the name doesn't matter.
	if(!displayName.empty() && pluralName.empty())
	{
		pluralName = displayName + 's';
		const char &last = displayName.back();
		if(!category.empty() && (last == 's' || last == 'x' || last == 'z'
				|| displayName.ends_with("ch") || displayName.ends_with("sh")))
			node.PrintTrace("Explicit plural name definition required, but none is provided. Defaulting to \""
					+ pluralName + "\".");
	}

	// Set the default jump fuel if not defined.
	bool isHyperdrive = attributes.Get("hyperdrive");
	bool isScramDrive = attributes.Get("scram drive");
	bool isJumpDrive = attributes.Get("jump drive");
	int64_t jumpFuel = attributes.Get("jump fuel");
	if((isHyperdrive || isScramDrive) && attributes.Get("hyperdrive fuel") <= 0)
	{
		if(jumpFuel > 0)
			attributes["hyperdrive fuel"] = jumpFuel;
		else
			Set("hyperdrive fuel", isScramDrive ? DEFAULT_SCRAM_DRIVE_COST : DEFAULT_HYPERDRIVE_COST);
	}
	if(isJumpDrive && attributes.Get("jump drive fuel") <= 0)
	{
		if(jumpFuel > 0)
			attributes["jump drive fuel"] = jumpFuel;
		else
			Set("jump drive fuel", DEFAULT_JUMP_DRIVE_COST);
	}
	if(jumpFuel)
		attributes.Erase("jump fuel");

	// Only outfits with the jump drive and jump range attributes can
	// use the jump range, so only keep track of the jump range on
	// viable outfits.
	if(isJumpDrive && attributes.Get("jump range"))
		GameData::AddJumpRange(Get("jump range"));

	// Legacy support for turrets that don't specify a turn rate:
	if(weapon && attributes.Get("turret mounts") && !weapon->TurretTurn()
		&& !weapon->AntiMissile() && !weapon->TractorBeam())
	{
		Weapon newWeapon = *weapon;
		newWeapon.turretTurn = 4.;
		weapon = make_shared<Weapon>(std::move(newWeapon));
		node.PrintTrace("Deprecated use of a turret without specified \"turret turn\":");
	}

	// Convert any legacy cargo / outfit scan definitions into power & speed,
	// so no runtime code has to check for both.
	auto convertScan = [&](string &&kind) -> void
	{
		string label = kind + " scan";
		int64_t initial = attributes.Get(label);
		if(initial)
		{
			attributes.Erase(label.c_str());
			node.PrintTrace("Deprecated use of \"" + label + "\" instead of \""
					+ label + " power\" and \"" + label + " speed\":");

			// Example: A scan value of 300 is equivalent to a scan power of 9.
			// 300 * 300 / 10000 = 9. (This 10000 is the scaling factor from the old attribute
			// to the new one and is not the same constant as ATTRIBUTE_PRECISION.)
			attributes[label + " power"] += initial * initial / 10000;
			// The default scan speed of 1 is unrelated to the magnitude of the scan value.
			// It may have been already specified, and if so, should not be increased.
			if(!attributes.Get(label + " efficiency"))
				Set(label + " efficiency", 15.);
		}

		// Similar check for scan speed which is replaced with scan efficiency.
		label += " speed";
		initial = attributes.Get(label);
		if(initial)
		{
			attributes.Erase(label.c_str());
			node.PrintTrace("Deprecated use of \"" + label + "\" instead of \""
					+ kind + " scan efficiency\":");
			// A reasonable update is 15x the previous value, as the base scan time
			// is 10x what it was before scan efficiency was introduced, along with
			// ships which are larger or further away also increasing the scan time.
			attributes[kind + " scan efficiency"] += initial * 15;
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



void Outfit::SetTrueName(const string &name)
{
	this->trueName = name;
}



const string &Outfit::DisplayName() const
{
	return displayName;
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



int Outfit::Index() const
{
	return index;
}



string Outfit::Description() const
{
	return description.ToString();
}



// Get the licenses needed to purchase this outfit.
const vector<string> &Outfit::Licenses() const
{
	return licenses;
}



// Get the image to display in the outfitter when buying this item.
const Drawable &Outfit::Thumbnail() const
{
	return thumbnail;
}



bool Outfit::Empty() const
{
	return attributes.empty();
}



double Outfit::Get(const char *attribute) const
{
	int64_t value = attributes.Get(attribute);
	if(!value)
		return 0.;
	return static_cast<double>(value) / ATTRIBUTE_PRECISION;
}



double Outfit::Get(const string &attribute) const
{
	return Get(attribute.c_str());
}



int64_t Outfit::GetPrecise(const char *attribute) const
{
	return attributes.Get(attribute);
}



int64_t Outfit::GetPrecise(const string &attribute) const
{
	return GetPrecise(attribute.c_str());
}



Outfit::AttributeIterator Outfit::begin() const
{
	return AttributeIterator(*this, attributes.begin());
}



Outfit::AttributeIterator Outfit::end() const
{
	return AttributeIterator(*this, attributes.end());
}



const Dictionary<int64_t> &Outfit::Precise() const
{
	return attributes;
}



// Determine whether the given number of instances of the given outfit can
// be added to a ship with the attributes represented by this instance. If
// not, return the maximum number that can be added.
int Outfit::CanAdd(const Outfit &other, int count) const
{
	for(const auto &[name, otherValue] : other.Precise())
	{
		// The minimum allowed value of most attributes is 0. Some attributes
		// have special functionality when negative, though, and are therefore
		// allowed to have values less than 0.
		optional<int64_t> minOpt = LowerLimitPrecise(name);
		if(!minOpt.has_value())
			continue;
		int64_t minimum = minOpt.value();

		// Only automatons may have a "required crew" of 0.
		if(!strcmp(name, "required crew"))
			minimum = !(GetPrecise("automaton") || other.GetPrecise("automaton"));

		int64_t value = GetPrecise(name);
		if(value + otherValue * count < minimum)
			count = (value - minimum) / -otherValue;
	}

	return count;
}



// For tracking a combination of outfits in a ship: add the given number of
// instances of the given outfit to this outfit.
void Outfit::Add(const Outfit &other, int count)
{
	cost += other.cost * count;
	mass += other.mass * count;
	for(const auto &[name, otherValue] : other.attributes)
		attributes[name] += otherValue * count;

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
	attributes[attribute] = value * ATTRIBUTE_PRECISION;
}



void Outfit::Set(const string &attribute, double value)
{
	Set(attribute.c_str(), value);
}



const set<const Outfit *> &Outfit::AmmoStored() const
{
	return ammoStored;
}



const set<const Outfit *> &Outfit::AmmoStoredOrUsed() const
{
	static set<const Outfit *> weaponAmmo;
	if(weapon && weapon->Ammo() && weaponAmmo.empty())
		weaponAmmo.insert(weapon->Ammo());
	return weapon ? weaponAmmo : ammoStored;
}



const set<const Outfit *> &Outfit::LinkedOutfits() const
{
	return linkedOutfits;
}



// Get this outfit's engine flare sprite, if any.
const vector<pair<Drawable, int>> &Outfit::FlareSprites() const
{
	return flareSprites;
}



const vector<pair<Drawable, int>> &Outfit::ReverseFlareSprites() const
{
	return reverseFlareSprites;
}



const vector<pair<Drawable, int>> &Outfit::SteeringFlareSprites() const
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
const map<const Effect *, double> &Outfit::JumpEffects() const
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
