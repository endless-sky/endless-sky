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

// TODO: move this to a common header file
// C++11 replacement for sizeof(a)/sizeof(a[0]), from https://www.g-truc.net/post-0708.html
template <typename T, std::size_t N>
constexpr std::size_t countof(T const (&)[N]) noexcept
{
    return N;
}

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
	for(unsigned i = 0; i + 1 < countof(names); i += 2)
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
	
	if(outfit.Reload())
	{
		static const string perSecondLabels[] = {
			"shield damage / second:",
			"hull damage / second:",
			"heat damage / second:",
			"ion damage / second:",
			"slowing damage / second:",
			"disruption damage / second:",
			"firing energy / second:",
			"firing heat / second:",
			"firing fuel / second:"
		};
		const double values[] = {                 // TODO: eliminate redundancy with per-shot values
		    outfit.ShieldDamage(),
		    outfit.HullDamage(),
		    outfit.HeatDamage(),
		    100. * outfit.IonDamage(),
		    100. * outfit.SlowingDamage(),
		    100. * outfit.DisruptionDamage(),
		    outfit.FiringEnergy(),
		    outfit.FiringHeat(),
		    outfit.FiringFuel()
		};
		static_assert(countof(values) == countof(perSecondLabels), "labels / values size mismatch");

		const double shotsPerSec = 60. / outfit.Reload();
		for(unsigned i = 0; i < countof(perSecondLabels); ++i)
			if(values[i])
			{
				attributeLabels.push_back(perSecondLabels[i]);
				attributeValues.push_back(Format::Number(shotsPerSec * values[i]));
				attributesHeight += 20;
			}
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
	for(unsigned i = 0; i < countof(percentValues); ++i)
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

// per-shot info
	static const string names[] = {
		"shield damage / shot:",
		"hull damage / shot:",
		"heat damage / shot:",
		"ion damage / shot:",
		"slowing damage / shot:",
		"disruption damage / shot:",
		"firing energy / shot:",
		"firing heat / shot:",
		"firing fuel / shot:",
		"inaccuracy:",                      // loop starts here for continuous-fire weapons
		"blast radius:",
		"missile strength:",
		"anti-missile damage / shot:",      // having this next to shots/second is important
	};
	const double values[] = {
		outfit.ShieldDamage(),
		outfit.HullDamage(),
		outfit.HeatDamage(),
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
	bool isContinuous = (outfit.Reload() <= 1);
	for(unsigned i = (isContinuous ? 9 : 0); i < countof(names); ++i)
		if(values[i])
		{
			attributeLabels.push_back(names[i]);
			attributeValues.push_back(Format::Number(values[i]));
			attributesHeight += 20;
		}

	attributeLabels.push_back("shots / second:");
	if(isContinuous)
		attributeValues.push_back("continuous");
	else
		attributeValues.push_back(Format::Number(60. / outfit.Reload()));
	attributesHeight += 20;
}
