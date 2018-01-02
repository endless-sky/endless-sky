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

#include "Color.h"
#include "Depreciation.h"
#include "Format.h"
#include "Outfit.h"
#include "PlayerInfo.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <sstream>

using namespace std;

namespace {
	const map<string, double> SCALE = {
		{"active cooling", 60.},
		{"afterburner energy", 60.},
		{"afterburner fuel", 60.},
		{"afterburner heat", 60.},
		{"cloaking energy", 60.},
		{"cloaking fuel", 60.},
		{"cloaking heat", 60.},
		{"cooling", 60.},
		{"cooling energy", 60.},
		{"energy consumption", 60.},
		{"energy generation", 60.},
		{"heat generation", 60.},
		{"hull repair rate", 60.},
		{"hull energy", 60.},
		{"hull heat", 60.},
		{"reverse thrusting energy", 60.},
		{"reverse thrusting heat", 60.},
		{"shield generation", 60.},
		{"shield energy", 60.},
		{"shield heat", 60.},
		{"solar collection", 60.},
		{"thrusting energy", 60.},
		{"thrusting heat", 60.},
		{"turn", 60.},
		{"turning energy", 60.},
		{"turning heat", 60.},
		
		{"thrust", 60. * 60.},
		{"reverse thrust", 60. * 60.},
		{"afterburner thrust", 60. * 60.},
		
		{"ion resistance", 60. * 100.},
		{"disruption resistance", 60. * 100.},
		{"slowing resistance", 60. * 100.}
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
	requirementValues.push_back(Format::Number(buyValue));
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
		requirementValues.push_back(Format::Number(sellValue));
		requirementsHeight += 20;
	}
	
	static const vector<string> NAMES = {
		"outfit space needed:", "outfit space",
		"weapon capacity needed:", "weapon capacity",
		"engine capacity needed:", "engine capacity",
		"gun ports needed:", "gun ports",
		"turret mounts needed:", "turret mounts"
	};
	for(unsigned i = 0; i + 1 < NAMES.size(); i += 2)
		if(outfit.Get(NAMES[i + 1]))
		{
			requirementLabels.emplace_back();
			requirementValues.emplace_back();
			requirementsHeight += 10;
		
			requirementLabels.push_back(NAMES[i]);
			requirementValues.push_back(Format::Number(-outfit.Get(NAMES[i + 1])));
			requirementsHeight += 20;
		}
}



void OutfitInfoDisplay::UpdateAttributes(const Outfit &outfit)
{
	attributeLabels.clear();
	attributeValues.clear();
	attributesHeight = 20;
	
	for(const pair<const char *, double> &it : outfit.Attributes())
	{
		static const set<string> SKIP = {
			"outfit space", "weapon capacity", "engine capacity", "gun ports", "turret mounts"
		};
		if(SKIP.count(it.first))
			continue;
		
		auto sit = SCALE.find(it.first);
		double scale = (sit == SCALE.end() ? 1. : sit->second);
		
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
			attributeValues.emplace_back(Format::Number(it.second * scale));
			attributesHeight += 20;
		}
	}
	
	if(!outfit.IsWeapon())
		return;
	
	// Pad the table.
	attributeLabels.emplace_back();
	attributeValues.emplace_back();
	attributesHeight += 10;
	
	if(outfit.Ammo())
	{
		attributeLabels.emplace_back("ammo:");
		attributeValues.emplace_back(outfit.Ammo()->Name());
		attributesHeight += 20;
	}
	
	attributeLabels.emplace_back("range:");
	attributeValues.emplace_back(Format::Number(outfit.Range()));
	attributesHeight += 20;
	
	static const vector<string> VALUE_NAMES = {
		"shield damage",
		"hull damage",
		"fuel damage",
		"heat damage",
		"ion damage",
		"slowing damage",
		"disruption damage",
		"firing energy",
		"firing heat",
		"firing fuel"
	};
	
	vector<double> values = {
		outfit.ShieldDamage(),
		outfit.HullDamage(),
		outfit.FuelDamage(),
		outfit.HeatDamage(),
		outfit.IonDamage() * 100.,
		outfit.SlowingDamage() * 100.,
		outfit.DisruptionDamage() * 100.,
		outfit.FiringEnergy(),
		outfit.FiringHeat(),
		outfit.FiringFuel()
	};
	
	// Add any per-second values to the table.
	double reload = outfit.Reload();
	if(reload)
	{
		static const string PER_SECOND = " / second:";
		for(unsigned i = 0; i < values.size(); ++i)
			if(values[i])
			{
				attributeLabels.emplace_back(VALUE_NAMES[i] + PER_SECOND);
				attributeValues.emplace_back(Format::Number(60. * values[i] / reload));
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
				attributeLabels.emplace_back(VALUE_NAMES[i] + PER_SHOT);
				attributeValues.emplace_back(Format::Number(values[i]));
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
