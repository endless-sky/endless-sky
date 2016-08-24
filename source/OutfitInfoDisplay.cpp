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
#include "Format.h"
#include "Outfit.h"

#include <algorithm>
#include <map>
#include <set>

using namespace std;

namespace {
	static const set<string> ATTRIBUTES_TO_SCALE = {
		"afterburner energy",
		"afterburner fuel",
		"afterburner heat",
		"cloaking energy",
		"cloaking fuel",
		"cooling",
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
		"thrusting energy",
		"thrusting heat",
		"turn",
		"turning energy",
		"turning heat"
	};
	
	static const set<string> BOOLEAN_ATTRIBUTES = {
		"unplunderable"
	};
}



OutfitInfoDisplay::OutfitInfoDisplay(const Outfit &outfit)
{
	Update(outfit);
}



// Call this every time the ship changes.
void OutfitInfoDisplay::Update(const Outfit &outfit)
{
	UpdateDescription(outfit.Description());
	UpdateRequirements(outfit);
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



void OutfitInfoDisplay::UpdateRequirements(const Outfit &outfit)
{
	requirementLabels.clear();
	requirementValues.clear();
	requirementsHeight = 20;
	
	requirementLabels.push_back("cost:");
	requirementValues.push_back(Format::Number(outfit.Cost()));
	requirementsHeight += 20;
	
	static const string names[] = {
		"outfit space needed:", "outfit space",
		"weapon capacity needed:", "weapon capacity",
		"engine capacity needed:", "engine capacity",
		"gun ports needed:", "gun ports",
		"turret mounts needed:", "turret mounts"
	};
	static const int NAMES =  sizeof(names) / sizeof(names[0]);
	for(int i = 0; i + 1 < NAMES; i += 2)
		if(outfit.Get(names[i + 1]))
		{
			requirementLabels.push_back(string());
			requirementValues.push_back(string());
			requirementsHeight += 10;
		
			requirementLabels.push_back(names[i]);
			requirementValues.push_back(Format::Number(-outfit.Get(names[i + 1])));
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
		
		if(BOOLEAN_ATTRIBUTES.count(it.first)) 
		{
			attributeLabels.push_back("This outfit is " + it.first + ".");
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
	
	if(outfit.FuelDamage() && outfit.Reload())
	{
		attributeLabels.push_back("fuel damage / second:");
		attributeValues.push_back(Format::Number(60. * outfit.FuelDamage() / outfit.Reload()));
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
	static const string percentNames[] = {
		"tracking:",
		"optical tracking:",
		"infrared tracking:",
		"radar tracking:",
		"piercing:"
	};
	double percentValues[] = {
		outfit.Tracking(),
		outfit.OpticalTracking(),
		outfit.InfraredTracking(),
		outfit.RadarTracking(),
		outfit.Piercing()
	};
	for(unsigned i = 0; i < sizeof(percentValues) / sizeof(percentValues[0]); ++i)
		if(percentValues[i])
		{
			int percent = 100. * percentValues[i] + .5;
			attributeLabels.push_back(percentNames[i]);
			attributeValues.push_back(Format::Number(percent) + "%");
			attributesHeight += 20;
		}
	
	attributeLabels.push_back(string());
	attributeValues.push_back(string());
	attributesHeight += 10;
	
	static const string names[] = {
		"shield damage / shot:",
		"hull damage / shot:",
		"heat damage / shot:",
		"ion damage / shot:",
		"slowing damage / shot:",
		"fuel damage / shot:",
		"disruption damage / shot:",
		"firing energy / shot:",
		"firing heat / shot:",
		"firing fuel / shot:",
		"inaccuracy:",
		"blast radius:",
		"missile strength:",
		"anti-missile:",
	};
	double values[] = {
		outfit.ShieldDamage(),
		outfit.HullDamage(),
		outfit.HeatDamage(),
		outfit.IonDamage() * 100.,
		outfit.SlowingDamage() * 100.,
		outfit.FuelDamage(),
		outfit.DisruptionDamage() * 100.,
		outfit.FiringEnergy(),
		outfit.FiringHeat(),
		outfit.FiringFuel(),
		outfit.Inaccuracy(),
		outfit.BlastRadius(),
		static_cast<double>(outfit.MissileStrength()),
		static_cast<double>(outfit.AntiMissile())
	};
	static const int NAMES = sizeof(names) / sizeof(names[0]);
	for(int i = (isContinuous ? 9 : 0); i < NAMES; ++i)
		if(values[i])
		{
			attributeLabels.push_back(names[i]);
			attributeValues.push_back(Format::Number(values[i]));
			attributesHeight += 20;
		}
}
