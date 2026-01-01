/* OutfitInfoDisplay.cpp
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

#include "OutfitInfoDisplay.h"

#include "Depreciation.h"
#include "text/Format.h"
#include "GameData.h"
#include "Outfit.h"
#include "PlayerInfo.h"
#include "Weapon.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <sstream>

using namespace std;

namespace {
	const vector<pair<double, string>> SCALE_LABELS = {
		make_pair(60., ""),
		make_pair(60. * 60., ""),
		make_pair(60. * 100., ""),
		make_pair(100., "%"),
		make_pair(100., ""),
		make_pair(1. / 60., "s")
	};

	const map<string, int> SCALE = {
		{"active cooling", 0},
		{"afterburner shields", 0},
		{"afterburner hull", 0},
		{"afterburner energy", 0},
		{"afterburner fuel", 0},
		{"afterburner heat", 0},
		{"burn resistance energy", 0},
		{"burn resistance fuel", 0},
		{"burn resistance heat", 0},
		{"cloak", 0},
		{"cloaking energy", 0},
		{"cloaking fuel", 0},
		{"cloaking heat", 0},
		{"cloaking shields", 0},
		{"cloaked firing", 0},
		{"cooling", 0},
		{"cooling energy", 0},
		{"corrosion resistance energy", 0},
		{"corrosion resistance fuel", 0},
		{"corrosion resistance heat", 0},
		{"discharge resistance energy", 0},
		{"discharge resistance fuel", 0},
		{"discharge resistance heat", 0},
		{"disruption resistance energy", 0},
		{"disruption resistance fuel", 0},
		{"disruption resistance heat", 0},
		{"energy consumption", 0},
		{"energy generation", 0},
		{"fuel consumption", 0},
		{"fuel energy", 0},
		{"fuel generation", 0},
		{"fuel heat", 0},
		{"heat generation", 0},
		{"heat dissipation", 0},
		{"hull repair rate", 0},
		{"hull energy", 0},
		{"hull fuel", 0},
		{"hull heat", 0},
		{"delayed hull repair rate", 0},
		{"delayed hull energy", 0},
		{"delayed hull fuel", 0},
		{"delayed hull heat", 0},
		{"ion resistance energy", 0},
		{"ion resistance fuel", 0},
		{"ion resistance heat", 0},
		{"scramble resistance energy", 0},
		{"scramble resistance fuel", 0},
		{"scramble resistance heat", 0},
		{"jump speed", 0},
		{"leak resistance energy", 0},
		{"leak resistance fuel", 0},
		{"leak resistance heat", 0},
		{"overheat damage rate", 0},
		{"reverse thrusting shields", 0},
		{"reverse thrusting hull", 0},
		{"reverse thrusting energy", 0},
		{"reverse thrusting fuel", 0},
		{"reverse thrusting heat", 0},
		{"scram drive", 0},
		{"shield generation", 0},
		{"shield energy", 0},
		{"shield fuel", 0},
		{"shield heat", 0},
		{"delayed shield generation", 0},
		{"delayed shield energy", 0},
		{"delayed shield fuel", 0},
		{"delayed shield heat", 0},
		{"slowing resistance energy", 0},
		{"slowing resistance fuel", 0},
		{"slowing resistance heat", 0},
		{"solar collection", 0},
		{"solar heat", 0},
		{"thrusting shields", 0},
		{"thrusting hull", 0},
		{"thrusting energy", 0},
		{"thrusting fuel", 0},
		{"thrusting heat", 0},
		{"turn", 0},
		{"turning shields", 0},
		{"turning hull", 0},
		{"turning energy", 0},
		{"turning fuel", 0},
		{"turning heat", 0},

		{"thrust", 1},
		{"reverse thrust", 1},
		{"afterburner thrust", 1},

		{"afterburner discharge", 2},
		{"afterburner corrosion", 2},
		{"afterburner ion", 2},
		{"afterburner scramble", 2},
		{"afterburner leakage", 2},
		{"afterburner burn", 2},
		{"afterburner slowing", 2},
		{"afterburner disruption", 2},

		{"reverse thrusting discharge", 2},
		{"reverse thrusting corrosion", 2},
		{"reverse thrusting ion", 2},
		{"reverse thrusting scramble", 2},
		{"reverse thrusting leakage", 2},
		{"reverse thrusting burn", 2},
		{"reverse thrusting slowing", 2},
		{"reverse thrusting disruption", 2},

		{"thrusting discharge", 2},
		{"thrusting corrosion", 2},
		{"thrusting ion", 2},
		{"thrusting scramble", 2},
		{"thrusting leakage", 2},
		{"thrusting burn", 2},
		{"thrusting slowing", 2},
		{"thrusting disruption", 2},

		{"turning discharge", 2},
		{"turning corrosion", 2},
		{"turning ion", 2},
		{"turning scramble", 2},
		{"turning leakage", 2},
		{"turning burn", 2},
		{"turning slowing", 2},
		{"turning disruption", 2},

		{"ion resistance", 2},
		{"scramble resistance", 2},
		{"disruption resistance", 2},
		{"slowing resistance", 2},
		{"discharge resistance", 2},
		{"corrosion resistance", 2},
		{"leak resistance", 2},
		{"burn resistance", 2},

		{"cloak by mass", 3},
		{"shield multiplier", 3},
		{"hull multiplier", 3},
		{"hull repair multiplier", 3},
		{"hull energy multiplier", 3},
		{"hull fuel multiplier", 3},
		{"hull heat multiplier", 3},
		{"piercing resistance", 3},
		{"shield generation multiplier", 3},
		{"shield energy multiplier", 3},
		{"shield fuel multiplier", 3},
		{"shield heat multiplier", 3},
		{"threshold percentage", 3},
		{"overheat damage threshold", 3},
		{"high shield permeability", 3},
		{"low shield permeability", 3},
		{"cloaked shield permeability", 3},
		{"cloaked regen multiplier", 3},
		{"cloaked repair multiplier", 3},
		{"acceleration multiplier", 3},
		{"turn multiplier", 3},
		{"turret turn multiplier", 3},

		{"burn protection", 3},
		{"corrosion protection", 3},
		{"discharge protection", 3},
		{"disruption protection", 3},
		{"drag reduction", 3},
		{"energy protection", 3},
		{"force protection", 3},
		{"fuel protection", 3},
		{"heat protection", 3},
		{"hull protection", 3},
		{"inertia reduction", 3},
		{"ion protection", 3},
		{"scramble protection", 3},
		{"leak protection", 3},
		{"piercing protection", 3},
		{"shield protection", 3},
		{"slowing protection", 3},
		{"cloak hull protection", 3},
		{"cloak shield protection", 3},

		{"repair delay", 5},
		{"cloaking repair delay", 5},
		{"disabled repair delay", 5},
		{"shield delay", 5},
		{"cloaking shield delay", 5},
		{"depleted shield delay", 5},
		{"disabled recovery time", 5}
	};

	const map<string, string> BOOLEAN_ATTRIBUTES = {
		{"unplunderable", "This outfit cannot be plundered."},
		{"installable", "This is not an installable item."},
		{"hyperdrive", "Allows you to make hyperjumps."},
		{"jump drive", "Lets you jump to any nearby system."},
		{"minable", "This item is mined from asteroids."},
		{"map minables", "This map reveals minables."},
		{"atrocity", "This outfit is considered an atrocity."},
		{"unique", "This item is unique."},
		{"cloaked afterburner", "You may use afterburners while cloaked."},
		{"cloaked boarding", "You may board while cloaked."},
		{"cloaked communication", "You may make hails while cloaked."},
		{"cloaked deployment", "You may deploy from bays while cloaked."},
		{"cloaked pickup", "You may pick up flotsam while cloaked."},
		{"cloaked scanning", "You may scan other ships while cloaked."}
	};

	bool IsNotRequirement(const string &label)
	{
		return label == "automaton" ||
			SCALE.find(label) != SCALE.end() ||
			BOOLEAN_ATTRIBUTES.find(label) != BOOLEAN_ATTRIBUTES.end();
	}
}



OutfitInfoDisplay::OutfitInfoDisplay(const Outfit &outfit, const PlayerInfo &player,
		bool canSell, bool descriptionCollapsed)
{
	Update(outfit, player, canSell, descriptionCollapsed);
}



// Call this every time the ship changes.
void OutfitInfoDisplay::Update(const Outfit &outfit, const PlayerInfo &player, bool canSell, bool descriptionCollapsed)
{
	UpdateDescription(outfit.Description(), outfit.Licenses(), false);
	UpdateRequirements(outfit, player, canSell, descriptionCollapsed);
	UpdateAttributes(outfit);

	maximumHeight = max(descriptionHeight, max(requirementsHeight, attributesHeight));
}



int OutfitInfoDisplay::RequirementsHeight() const
{
	return requirementsHeight;
}



void OutfitInfoDisplay::DrawRequirements(const Point &topLeft) const
{
	Draw(topLeft, requirementLabels, requirementValues);
}



void OutfitInfoDisplay::UpdateRequirements(const Outfit &outfit, const PlayerInfo &player,
		bool canSell, bool descriptionCollapsed)
{
	requirementLabels.clear();
	requirementValues.clear();
	requirementsHeight = 20;

	int day = player.GetDate().DaysSinceEpoch();
	int64_t cost = outfit.Cost();
	int64_t buyValue = player.StockDepreciation().Value(&outfit, day);
	int64_t sellValue = player.FleetDepreciation().Value(&outfit, day);

	for(const string &license : outfit.Licenses())
	{
		if(player.HasLicense(license))
			continue;

		const auto &licenseOutfit = GameData::Outfits().Find(license + " License");
		if(descriptionCollapsed || (licenseOutfit && licenseOutfit->Cost()))
		{
			requirementLabels.push_back("license:");
			requirementValues.push_back(license);
			requirementsHeight += 20;
		}
	}

	if(buyValue == cost)
		requirementLabels.push_back("cost:");
	else
	{
		ostringstream out;
		out << "cost (" << (100 * buyValue) / cost << "%):";
		requirementLabels.push_back(out.str());
	}
	requirementValues.push_back(buyValue ? Format::Credits(buyValue) : "free");
	requirementsHeight += 20;

	if(canSell && sellValue != buyValue)
	{
		if(sellValue == cost)
			requirementLabels.push_back("sells for:");
		else
		{
			ostringstream out;
			out << "sells for (" << (100 * sellValue) / cost << "%):";
			requirementLabels.push_back(out.str());
		}
		requirementValues.push_back(Format::Credits(sellValue));
		requirementsHeight += 20;
	}

	if(outfit.Mass())
	{
		requirementLabels.emplace_back("mass:");
		requirementValues.emplace_back(Format::Number(outfit.Mass()));
		requirementsHeight += 20;
	}

	requirementLabels.emplace_back();
	requirementValues.emplace_back();
	requirementsHeight += 10;

	bool hasContent = false;
	static const vector<string> BEFORE = {"outfit space", "weapon capacity", "engine capacity"};
	for(const auto &attr : BEFORE)
	{
		if(outfit.Get(attr) < 0)
		{
			AddRequirementAttribute(attr, outfit.Get(attr));
			hasContent = true;
		}
	}

	if(hasContent)
	{
		requirementLabels.emplace_back();
		requirementValues.emplace_back();
		requirementsHeight += 10;
	}

	for(const pair<const char *, double> &it : outfit.Attributes())
		if(!count(BEFORE.begin(), BEFORE.end(), it.first))
			AddRequirementAttribute(it.first, it.second);
}



// Any attribute with a negative value is considered a requirement.
// Any exceptions to that rule would require in-game code to handle
// their unique properties, so when code is added to handle a new
// attribute, this code also should also be updated.
void OutfitInfoDisplay::AddRequirementAttribute(string label, double value)
{
	// These attributes have negative values but are not requirements
	if(IsNotRequirement(label))
		return;

	// Special case for 'required crew' - use positive values as a requirement.
	if(label == "required crew")
	{
		if(value > 0)
		{
			requirementLabels.push_back(label + ":");
			requirementValues.push_back(Format::Number(value));
			requirementsHeight += 20;
			return;
		}
		else
			value *= -1;
	}

	if(value >= 0)
		return;

	requirementLabels.push_back(label + " needed:");
	requirementValues.push_back(Format::Number(-value));
	requirementsHeight += 20;
}



void OutfitInfoDisplay::UpdateAttributes(const Outfit &outfit)
{
	attributeLabels.clear();
	attributeValues.clear();
	attributesHeight = 20;

	bool hasNormalAttributes = false;

	// These attributes are regularly negative on outfits, so when positive,
	// tag them with "added" and show them first. They conveniently
	// don't use SCALE or BOOLEAN_ATTRIBUTES.
	static const vector<string> EXPECTED_NEGATIVE = {
		"outfit space", "weapon capacity", "engine capacity", "gun ports", "turret mounts"
	};

	for(const string &attr : EXPECTED_NEGATIVE)
	{
		double value = outfit.Get(attr);
		if(value <= 0)
			continue;

		attributeLabels.emplace_back(attr + " added:");
		attributeValues.emplace_back(Format::Number(value));
		attributesHeight += 20;
		hasNormalAttributes = true;
	}

	for(const pair<const char *, double> &it : outfit.Attributes())
	{
		if(count(EXPECTED_NEGATIVE.begin(), EXPECTED_NEGATIVE.end(), it.first))
			continue;

		// Only show positive values here, with some exceptions.
		// Negative values are usually handled as a "requirement"
		if(static_cast<string>(it.first) == "required crew")
		{
			// 'required crew' is inverted - positive values are requirements.
			if(it.second > 0)
				continue;

			// A negative 'required crew' would be a benefit, so it is listed here.
		}
		// If this attribute is not a requirement, it is always listed here, though it may be negative.
		else if(it.second < 0 && !IsNotRequirement(it.first))
			continue;

		auto sit = SCALE.find(it.first);
		double scale = (sit == SCALE.end() ? 1. : SCALE_LABELS[sit->second].first);
		string units = (sit == SCALE.end() ? "" : SCALE_LABELS[sit->second].second);

		auto bit = BOOLEAN_ATTRIBUTES.find(it.first);
		if(bit != BOOLEAN_ATTRIBUTES.end())
		{
			attributeLabels.emplace_back(bit->second);
			attributeValues.emplace_back(" ");
			attributesHeight += 20;
		}
		else
		{
			attributeLabels.emplace_back(static_cast<string>(it.first) + ":");
			attributeValues.emplace_back(Format::Number(it.second * scale) + units);
			attributesHeight += 20;
		}
		hasNormalAttributes = true;
	}

	const Weapon *weapon = outfit.GetWeapon().get();
	if(!weapon)
		return;

	// Insert padding if any normal attributes were listed above.
	if(hasNormalAttributes)
	{
		attributeLabels.emplace_back();
		attributeValues.emplace_back();
		attributesHeight += 10;
	}

	if(weapon->Ammo())
	{
		attributeLabels.emplace_back("ammo:");
		attributeValues.emplace_back(weapon->Ammo()->DisplayName());
		attributesHeight += 20;
		if(weapon->AmmoUsage() != 1)
		{
			attributeLabels.emplace_back("ammo usage:");
			attributeValues.emplace_back(Format::Number(weapon->AmmoUsage()));
			attributesHeight += 20;
		}
	}

	double range = weapon->Range();
	attributeLabels.emplace_back("range:");
	attributeValues.emplace_back(Format::Number(range));
	attributesHeight += 20;

	attributeLabels.emplace_back("velocity:");
	double velocity = weapon->WeightedVelocity();
	if(velocity == range)
		attributeValues.emplace_back("instantaneous");
	else
		attributeValues.emplace_back(Format::Number(velocity * 60.));
	attributesHeight += 20;

	// Identify the dropoff at range and inform the player.
	double fullDropoff = weapon->MaxDropoff();
	if(fullDropoff != 1.)
	{
		attributeLabels.emplace_back("dropoff modifier:");
		attributeValues.emplace_back(Format::Number(100. * fullDropoff) + "%");
		attributesHeight += 20;
		// Identify the ranges between which the dropoff takes place.
		attributeLabels.emplace_back("dropoff range:");
		const pair<double, double> &ranges = weapon->DropoffRanges();
		attributeValues.emplace_back(Format::Number(ranges.first)
			+ " - " + Format::Number(ranges.second));
		attributesHeight += 20;
	}

	static const vector<pair<string, string>> VALUE_NAMES = {
		{"shield damage", ""},
		{"hull damage", ""},
		{"minable damage", ""},
		{"fuel damage", ""},
		{"heat damage", ""},
		{"energy damage", ""},
		{"ion damage", ""},
		{"scrambling damage", ""},
		{"slowing damage", ""},
		{"disruption damage", ""},
		{"discharge damage", ""},
		{"corrosion damage", ""},
		{"leak damage", ""},
		{"burn damage", ""},
		{"% shield damage", "%"},
		{"% hull damage", "%"},
		{"% minable damage", "%"},
		{"% fuel damage", "%"},
		{"% heat damage", "%"},
		{"% energy damage", "%"},
		{"firing energy", ""},
		{"firing heat", ""},
		{"firing fuel", ""},
		{"firing hull", ""},
		{"firing shields", ""},
		{"firing ion", ""},
		{"firing scramble", ""},
		{"firing slowing", ""},
		{"firing disruption", ""},
		{"firing discharge", ""},
		{"firing corrosion", ""},
		{"firing leak", ""},
		{"firing burn", ""},
		{"% firing energy", "%"},
		{"% firing heat", "%"},
		{"% firing fuel", "%"},
		{"% firing hull", "%"},
		{"% firing shields", "%"}
	};

	vector<double> values = {
		weapon->ShieldDamage(),
		weapon->HullDamage(),
		weapon->MinableDamage() != weapon->HullDamage() ? weapon->MinableDamage() : 0.,
		weapon->FuelDamage(),
		weapon->HeatDamage(),
		weapon->EnergyDamage(),
		weapon->IonDamage() * 100.,
		weapon->ScramblingDamage() * 100.,
		weapon->SlowingDamage() * 100.,
		weapon->DisruptionDamage() * 100.,
		weapon->DischargeDamage() * 100.,
		weapon->CorrosionDamage() * 100.,
		weapon->LeakDamage() * 100.,
		weapon->BurnDamage() * 100.,
		weapon->RelativeShieldDamage() * 100.,
		weapon->RelativeHullDamage() * 100.,
		weapon->RelativeMinableDamage() != weapon->RelativeHullDamage() ? weapon->RelativeMinableDamage() * 100. : 0.,
		weapon->RelativeFuelDamage() * 100.,
		weapon->RelativeHeatDamage() * 100.,
		weapon->RelativeEnergyDamage() * 100.,
		weapon->FiringEnergy(),
		weapon->FiringHeat(),
		weapon->FiringFuel(),
		weapon->FiringHull(),
		weapon->FiringShields(),
		weapon->FiringIon() * 100.,
		weapon->FiringScramble() * 100.,
		weapon->FiringSlowing() * 100.,
		weapon->FiringDisruption() * 100.,
		weapon->FiringDischarge() * 100.,
		weapon->FiringCorrosion() * 100.,
		weapon->FiringLeak() * 100.,
		weapon->FiringBurn() * 100.,
		weapon->RelativeFiringEnergy() * 100.,
		weapon->RelativeFiringHeat() * 100.,
		weapon->RelativeFiringFuel() * 100.,
		weapon->RelativeFiringHull() * 100.,
		weapon->RelativeFiringShields() * 100.
	};

	// Add any per-second values to the table.
	double reload = weapon->Reload();
	if(reload)
	{
		static const string PER_SECOND = " / second:";
		for(unsigned i = 0; i < values.size(); ++i)
			if(values[i])
			{
				attributeLabels.emplace_back(VALUE_NAMES[i].first + PER_SECOND);
				attributeValues.emplace_back(Format::Number(60. * values[i] / reload) + VALUE_NAMES[i].second);
				attributesHeight += 20;
			}
	}

	bool oneFrame = (weapon->TotalLifetime() == 1.);
	bool isContinuous = (reload <= 1. && oneFrame);
	bool isContinuousBurst = (weapon->BurstCount() > 1 && weapon->BurstReload() <= 1. && oneFrame);
	attributeLabels.emplace_back("shots / second:");
	if(isContinuous)
		attributeValues.emplace_back("continuous");
	else if(isContinuousBurst)
	{
		int value = static_cast<int>(lround(weapon->BurstReload() * 100. / reload));
		attributeValues.emplace_back("continuous (" + Format::Number(value) + "%)");
	}
	else
		attributeValues.emplace_back(Format::Number(60. / reload));
	attributesHeight += 20;

	double turretTurn = weapon->TurretTurn() * 60.;
	if(turretTurn)
	{
		attributeLabels.emplace_back("turret turn rate:");
		attributeValues.emplace_back(Format::Number(turretTurn));
		attributesHeight += 20;
	}
	double arc = weapon->Arc();
	if(arc < 360.)
	{
		attributeLabels.emplace_back("arc:");
		attributeValues.emplace_back(Format::Number(arc));
		attributesHeight += 20;
	}
	if(weapon->Homing())
	{
		attributeLabels.emplace_back("homing type:");
		attributeValues.emplace_back(weapon->Leading() ? "leading" : "direct");
		attributesHeight += 20;
	}
	static const vector<string> PERCENT_NAMES = {
		"tracking:",
		"optical tracking:",
		"infrared tracking:",
		"radar tracking:",
		"piercing:"
	};
	vector<double> percentValues = {
		weapon->Tracking(),
		weapon->OpticalTracking(),
		weapon->InfraredTracking(),
		weapon->RadarTracking(),
		weapon->Piercing()
	};
	for(unsigned i = 0; i < PERCENT_NAMES.size(); ++i)
		if(percentValues[i])
		{
			int percent = lround(100. * percentValues[i]);
			attributeLabels.push_back(PERCENT_NAMES[i]);
			attributeValues.push_back(Format::Number(percent) + "%");
			attributesHeight += 20;
		}
	if(weapon->ThrottleControl())
	{
		attributeLabels.emplace_back("Projectiles can control thrust.");
		attributeValues.emplace_back(" ");
		attributesHeight += 20;
	}
	if(weapon->HasBlindspot())
	{
		attributeLabels.emplace_back("Cannot track targets behind it.");
		attributeValues.emplace_back(" ");
		attributesHeight += 20;
	}

	// Pad the table.
	attributeLabels.emplace_back();
	attributeValues.emplace_back();
	attributesHeight += 10;

	// Add per-shot values to the table. If the weapon fires continuously,
	// the values have already been added.
	if(!isContinuous && !isContinuousBurst)
	{
		static const string PER_SHOT = " / shot:";
		for(unsigned i = 0; i < VALUE_NAMES.size(); ++i)
			if(values[i])
			{
				attributeLabels.emplace_back(VALUE_NAMES[i].first + PER_SHOT);
				attributeValues.emplace_back(Format::Number(values[i]) + VALUE_NAMES[i].second);
				attributesHeight += 20;
			}
	}

	static const vector<string> OTHER_NAMES = {
		"inaccuracy:",
		"blast radius:",
		"missile strength:",
		"anti-missile:",
		"tractor beam:",
		"mining precision:",
	};
	vector<double> otherValues = {
		weapon->Inaccuracy(),
		weapon->BlastRadius(),
		static_cast<double>(weapon->MissileStrength()),
		static_cast<double>(weapon->AntiMissile()),
		weapon->TractorBeam() * 60.,
		weapon->Prospecting() && weapon->MinableDamage() ? weapon->Prospecting() / weapon->MinableDamage() : 0.,
	};

	for(unsigned i = 0; i < OTHER_NAMES.size(); ++i)
		if(otherValues[i])
		{
			attributeLabels.emplace_back(OTHER_NAMES[i]);
			attributeValues.emplace_back(Format::Number(otherValues[i]));
			attributesHeight += 20;
		}
}
