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
#include <map>
#include <set>
#include <sstream>

using namespace std;

namespace {
	static const set<string> ATTRIBUTES_TO_SCALE = {
		"active cooling",
		"afterburner energy",
		"afterburner fuel",
		"afterburner heat",
		"cloaking energy",
		"cloaking fuel",
		"cooling",
		"cooling energy",
		"energy consumption",
		"energy generation",
		"heat generation",
		"hull repair rate",
		"hull energy",
		"hull heat",
		"reverse thrusting energy",
		"reverse thrusting heat",
		"shield generation",
		"shield energy",
		"shield heat",
		"solar collection",
		"thrusting energy",
		"thrusting heat",
		"turn",
		"turning energy",
		"turning heat"
	};
	
	static const map<string, string> BOOLEAN_ATTRIBUTES = {
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
			requirementLabels.push_back(string());
			requirementValues.push_back(string());
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
	
	map<string, map<string, int>> listing;
	for(const auto &it : outfit.Attributes())
	{
		if(it.first == "outfit space"
				|| it.first == "weapon capacity" || it.first == "engine capacity"
				|| it.first == "gun ports" || it.first == "turret mounts")
			continue;
		
		string value;
		double scale = 1.;
		if(it.first == "thrust" || it.first == "reverse thrust" || it.first == "afterburner thrust")
			scale = 60. * 60.;
		else if(ATTRIBUTES_TO_SCALE.count(it.first))
			scale = 60.;
		
		auto bit = BOOLEAN_ATTRIBUTES.find(it.first);
		if(bit != BOOLEAN_ATTRIBUTES.end()) 
		{
			attributeLabels.push_back(bit->second);
			attributeValues.push_back(" ");
			attributesHeight += 20;
		}
		else
		{
			attributeLabels.push_back(it.first + ':');
			attributeValues.push_back(Format::Number(it.second * scale));
			attributesHeight += 20;
		}
	}
	
	if(!outfit.IsWeapon())
		return;
	
	attributeLabels.push_back(string());
	attributeValues.push_back(string());
	attributesHeight += 10;
	
	if(outfit.Ammo())
	{
		attributeLabels.push_back("ammo:");
		attributeValues.push_back(outfit.Ammo()->Name());
		attributesHeight += 20;
	}
	
	attributeLabels.push_back("range:");
	attributeValues.push_back(Format::Number(outfit.Range()));
	attributesHeight += 20;
	
	if(outfit.ShieldDamage() && outfit.Reload())
	{
		attributeLabels.push_back("shield damage / second:");
		attributeValues.push_back(Format::Number(60. * outfit.ShieldDamage() / outfit.Reload()));
		attributesHeight += 20;
	}
	
	if(outfit.HullDamage() && outfit.Reload())
	{
		attributeLabels.push_back("hull damage / second:");
		attributeValues.push_back(Format::Number(60. * outfit.HullDamage() / outfit.Reload()));
		attributesHeight += 20;
	}
	
	if(outfit.HeatDamage() && outfit.Reload())
	{
		attributeLabels.push_back("heat damage / second:");
		attributeValues.push_back(Format::Number(60. * outfit.HeatDamage() / outfit.Reload()));
		attributesHeight += 20;
	}

	if(outfit.FuelDamage() && outfit.Reload())
        {
                attributeLabels.push_back("fuel damage / second:");
                attributeValues.push_back(Format::Number(60. * outfit.FuelDamage() / outfit.Reload()));
                attributesHeight += 20;
        }
	
	if(outfit.IonDamage() && outfit.Reload())
	{
		attributeLabels.push_back("ion damage / second:");
		attributeValues.push_back(Format::Number(6000. * outfit.IonDamage() / outfit.Reload()));
		attributesHeight += 20;
	}
	
	if(outfit.SlowingDamage() && outfit.Reload())
	{
		attributeLabels.push_back("slowing damage / second:");
		attributeValues.push_back(Format::Number(6000. * outfit.SlowingDamage() / outfit.Reload()));
		attributesHeight += 20;
	}
	
	if(outfit.DisruptionDamage() && outfit.Reload())
	{
		attributeLabels.push_back("disruption damage / second:");
		attributeValues.push_back(Format::Number(6000. * outfit.DisruptionDamage() / outfit.Reload()));
		attributesHeight += 20;
	}
	
	if(outfit.FiringEnergy() && outfit.Reload())
	{
		attributeLabels.push_back("firing energy / second:");
		attributeValues.push_back(Format::Number(60. * outfit.FiringEnergy() / outfit.Reload()));
		attributesHeight += 20;
	}
	
	if(outfit.FiringHeat() && outfit.Reload())
	{
		attributeLabels.push_back("firing heat / second:");
		attributeValues.push_back(Format::Number(60. * outfit.FiringHeat() / outfit.Reload()));
		attributesHeight += 20;
	}
	
	if(outfit.FiringFuel() && outfit.Reload())
	{
		attributeLabels.push_back("firing fuel / second:");
		attributeValues.push_back(Format::Number(60. * outfit.FiringFuel() / outfit.Reload()));
		attributesHeight += 20;
	}
	
	bool isContinuous = (outfit.Reload() <= 1);
	attributeLabels.push_back("shots / second:");
	if(isContinuous)
		attributeValues.push_back("continuous");
	else
		attributeValues.push_back(Format::Number(60. / outfit.Reload()));
	attributesHeight += 20;
	
	double turretTurn = outfit.TurretTurn() * 60.;
	if(turretTurn)
	{
		attributeLabels.push_back("turret turn rate:");
		attributeValues.push_back(Format::Number(turretTurn));
		attributesHeight += 20;
	}
	int homing = outfit.Homing();
	if(homing)
	{
		static const string skill[] = {
			"no",
			"poor",
			"fair",
			"good",
			"excellent"
		};
		attributeLabels.push_back("homing:");
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
			int percent = 100. * percentValues[i] + .5;
			attributeLabels.push_back(PERCENT_NAMES[i]);
			attributeValues.push_back(Format::Number(percent) + "%");
			attributesHeight += 20;
		}
	
	attributeLabels.push_back(string());
	attributeValues.push_back(string());
	attributesHeight += 10;
	
	static const vector<string> OTHER_NAMES = {
		"shield damage / shot:",
		"hull damage / shot:",
		"heat damage / shot:",
		"fuel damage / shot:",
		"ion damage / shot:",
		"slowing damage / shot:",
		"disruption damage / shot:",
		"firing energy / shot:",
		"firing heat / shot:",
		"firing fuel / shot:",
		"inaccuracy:",
		"blast radius:",
		"missile strength:",
		"anti-missile:",
	};
	vector<double> values = {
		outfit.ShieldDamage(),
		outfit.HullDamage(),
		outfit.HeatDamage(),
		outfit.FuelDamage(),
		outfit.IonDamage() * 100.,
		outfit.SlowingDamage() * 100.,
		outfit.DisruptionDamage() * 100.,
		outfit.FiringEnergy(),
		outfit.FiringHeat(),
		outfit.FiringFuel(),
		outfit.Inaccuracy(),
		outfit.BlastRadius(),
		static_cast<double>(outfit.MissileStrength()),
		static_cast<double>(outfit.AntiMissile())
	};
	for(unsigned i = (isContinuous ? 9 : 0); i < OTHER_NAMES.size(); ++i)
		if(values[i])
		{
			attributeLabels.push_back(OTHER_NAMES[i]);
			attributeValues.push_back(Format::Number(values[i]));
			attributesHeight += 20;
		}
}
