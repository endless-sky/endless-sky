/* OutfitInfoDisplay.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "OutfitInfoDisplay.h"

#include "Depreciation.h"
#include "text/Format.h"
#include "Outfit.h"
#include "PlayerInfo.h"

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
		make_pair(1. / 60., "")
	};
	
	const map<string, int> SCALE = {
		{"active cooling", 0},
		{"afterburner energy", 0},
		{"afterburner fuel", 0},
		{"afterburner heat", 0},
		{"cloak", 0},
		{"cloaking energy", 0},
		{"cloaking fuel", 0},
		{"cloaking heat", 0},
		{"cooling", 0},
		{"cooling energy", 0},
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
		{"ion resistance energy", 0},
		{"ion resistance fuel", 0},
		{"ion resistance heat", 0},
		{"jump speed", 0},
		{"reverse thrusting energy", 0},
		{"reverse thrusting heat", 0},
		{"scram drive", 0},
		{"shield generation", 0},
		{"shield energy", 0},
		{"shield fuel", 0},
		{"shield heat", 0},
		{"slowing resistance energy", 0},
		{"slowing resistance fuel", 0},
		{"slowing resistance heat", 0},
		{"solar collection", 0},
		{"solar heat", 0},
		{"thrusting energy", 0},
		{"thrusting heat", 0},
		{"turn", 0},
		{"turning energy", 0},
		{"turning heat", 0},
		
		{"thrust", 1},
		{"reverse thrust", 1},
		{"afterburner thrust", 1},
		
		{"ion resistance", 2},
		{"disruption resistance", 2},
		{"slowing resistance", 2},
		
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
		
		{"disruption protection", 4},
		{"energy protection", 4},
		{"force protection", 4},
		{"fuel protection", 4},
		{"heat protection", 4},
		{"hull protection", 4},
		{"ion protection", 4},
		{"piercing protection", 4},
		{"shield protection", 4},
		{"slowing protection", 4},
		
		{"repair delay", 5},
		{"disabled repair delay", 5},
		{"shield delay", 5},
		{"depleted shield delay", 5}
	};
	
	const map<string, string> BOOLEAN_ATTRIBUTES = {
		{"unplunderable", "This outfit cannot be plundered."},
		{"installable", "This is not an installable item."},
		{"hyperdrive", "Allows you to make hyperjumps."},
		{"jump drive", "Lets you jump to any nearby system."}
	};
}



OutfitInfoDisplay::OutfitInfoDisplay(const Outfit &outfit, const PlayerInfo &player, bool canSell)
{
	Update(outfit, player, canSell);
}



// Call this every time the ship changes.
void OutfitInfoDisplay::Update(const Outfit &outfit, const PlayerInfo &player, bool canSell)
{
	UpdateDescription(outfit.Description(), outfit.Licenses(), false);
	UpdateRequirements(outfit, player, canSell);
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



void OutfitInfoDisplay::UpdateRequirements(const Outfit &outfit, const PlayerInfo &player, bool canSell)
{
	requirementLabels.clear();
	requirementValues.clear();
	requirementsHeight = 20;
	
	int day = player.GetDate().DaysSinceEpoch();
	int64_t cost = outfit.Cost();
	int64_t buyValue = player.StockDepreciation().Value(&outfit, day);
	int64_t sellValue = player.FleetDepreciation().Value(&outfit, day);
	
	if(buyValue == cost)
		requirementLabels.push_back("cost:");
	else
	{
		ostringstream out;
		out << "cost (" << (100 * buyValue) / cost << "%):";
		requirementLabels.push_back(out.str());
	}
	requirementValues.push_back(Format::Credits(buyValue));
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
	
	bool hasContent = true;
	static const vector<string> NAMES = {
		"", "",
		"outfit space needed:", "outfit space",
		"weapon capacity needed:", "weapon capacity",
		"engine capacity needed:", "engine capacity",
		"", "",
		"gun ports needed:", "gun ports",
		"turret mounts needed:", "turret mounts"
	};
	for(unsigned i = 0; i + 1 < NAMES.size(); i += 2)
	{
		if(NAMES[i].empty() && hasContent)
		{
			requirementLabels.emplace_back();
			requirementValues.emplace_back();
			requirementsHeight += 10;
			hasContent = false;
		}
		else if(outfit.Get(NAMES[i + 1]))
		{
			requirementLabels.push_back(NAMES[i]);
			requirementValues.push_back(Format::Number(-outfit.Get(NAMES[i + 1])));
			requirementsHeight += 20;
			hasContent = true;
		}
	}
}



void OutfitInfoDisplay::UpdateAttributes(const Outfit &outfit)
{
	attributeLabels.clear();
	attributeValues.clear();
	attributesHeight = 20;
	
	bool hasNormalAttributes = false;
	for(const pair<const char *, double> &it : outfit.Attributes())
	{
		static const set<string> SKIP = {
			"outfit space", "weapon capacity", "engine capacity", "gun ports", "turret mounts"
		};
		if(SKIP.count(it.first))
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
	
	if(!outfit.IsWeapon())
		return;
	
	// Insert padding if any normal attributes were listed above.
	if(hasNormalAttributes)
	{
		attributeLabels.emplace_back();
		attributeValues.emplace_back();
		attributesHeight += 10;
	}
	
	if(outfit.Ammo())
	{
		attributeLabels.emplace_back("ammo:");
		attributeValues.emplace_back(outfit.Ammo()->Name());
		attributesHeight += 20;
		if(outfit.AmmoUsage() != 1)
		{
			attributeLabels.emplace_back("ammo usage:");
			attributeValues.emplace_back(Format::Number(outfit.AmmoUsage()));
			attributesHeight += 20;
		}
	}
	
	attributeLabels.emplace_back("range:");
	attributeValues.emplace_back(Format::Number(outfit.Range()));
	attributesHeight += 20;
	
	static const vector<pair<string, string>> VALUE_NAMES = {
		{"shield damage", ""},
		{"hull damage", ""},
		{"fuel damage", ""},
		{"heat damage", ""},
		{"energy damage", ""},
		{"ion damage", ""},
		{"slowing damage", ""},
		{"disruption damage", ""},
		{"% shield damage", "%"},
		{"% hull damage", "%"},
		{"% fuel damage", "%"},
		{"% heat damage", "%"},
		{"% energy damage", "%"},
		{"firing energy", ""},
		{"firing heat", ""},
		{"firing fuel", ""},
		{"firing hull", ""},
		{"firing shields", ""},
		{"firing ion", ""},
		{"firing slowing", ""},
		{"firing disruption", ""},
		{"% firing energy", "%"},
		{"% firing heat", "%"},
		{"% firing fuel", "%"},
		{"% firing hull", "%"},
		{"% firing shields", "%"}
	};
	
	vector<double> values = {
		outfit.ShieldDamage(),
		outfit.HullDamage(),
		outfit.FuelDamage(),
		outfit.HeatDamage(),
		outfit.EnergyDamage(),
		outfit.IonDamage() * 100.,
		outfit.SlowingDamage() * 100.,
		outfit.DisruptionDamage() * 100.,
		outfit.RelativeShieldDamage() * 100.,
		outfit.RelativeHullDamage() * 100.,
		outfit.RelativeFuelDamage() * 100.,
		outfit.RelativeHeatDamage() * 100.,
		outfit.RelativeEnergyDamage() * 100.,
		outfit.FiringEnergy(),
		outfit.FiringHeat(),
		outfit.FiringFuel(),
		outfit.FiringHull(),
		outfit.FiringShields(),
		outfit.FiringIon() * 100.,
		outfit.FiringSlowing() * 100.,
		outfit.FiringDisruption() * 100.,
		outfit.RelativeFiringEnergy() * 100.,
		outfit.RelativeFiringHeat() * 100.,
		outfit.RelativeFiringFuel() * 100.,
		outfit.RelativeFiringHull() * 100.,
		outfit.RelativeFiringShields() * 100.
	};
	
	// Add any per-second values to the table.
	double reload = outfit.Reload();
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
	
	bool isContinuous = (reload <= 1);
	attributeLabels.emplace_back("shots / second:");
	if(isContinuous)
		attributeValues.emplace_back("continuous");
	else
		attributeValues.emplace_back(Format::Number(60. / reload));
	attributesHeight += 20;
	
	double turretTurn = outfit.TurretTurn() * 60.;
	if(turretTurn)
	{
		attributeLabels.emplace_back("turret turn rate:");
		attributeValues.emplace_back(Format::Number(turretTurn));
		attributesHeight += 20;
	}
	int homing = outfit.Homing();
	if(homing)
	{
		static const string skill[] = {
			"none",
			"poor",
			"fair",
			"good",
			"excellent"
		};
		attributeLabels.emplace_back("homing:");
		attributeValues.push_back(skill[max(0, min(4, homing))]);
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
		outfit.Tracking(),
		outfit.OpticalTracking(),
		outfit.InfraredTracking(),
		outfit.RadarTracking(),
		outfit.Piercing()
	};
	for(unsigned i = 0; i < PERCENT_NAMES.size(); ++i)
		if(percentValues[i])
		{
			int percent = lround(100. * percentValues[i]);
			attributeLabels.push_back(PERCENT_NAMES[i]);
			attributeValues.push_back(Format::Number(percent) + "%");
			attributesHeight += 20;
		}
	
	// Pad the table.
	attributeLabels.emplace_back();
	attributeValues.emplace_back();
	attributesHeight += 10;
	
	// Add per-shot values to the table. If the weapon fires continuously,
	// the values have already been added.
	if(!isContinuous)
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
		"anti-missile:"
	};
	vector<double> otherValues = {
		outfit.Inaccuracy(),
		outfit.BlastRadius(),
		static_cast<double>(outfit.MissileStrength()),
		static_cast<double>(outfit.AntiMissile())
	};
	
	for(unsigned i = 0; i < OTHER_NAMES.size(); ++i)
		if(otherValues[i])
		{
			attributeLabels.emplace_back(OTHER_NAMES[i]);
			attributeValues.emplace_back(Format::Number(otherValues[i]));
			attributesHeight += 20;
		}
}
