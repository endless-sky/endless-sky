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
#include "Font.h"
#include "FontSet.h"
#include "Format.h"
#include "GameData.h"
#include "Outfit.h"
#include "Table.h"

#include <cmath>
#include <map>
#include <sstream>

using namespace std;

namespace {
	static const int WIDTH = 250;
	
	Point Draw(Point point, const vector<string> &labels, const vector<string> &values)
	{
		// Get standard colors to draw with.
		Color labelColor = *GameData::Colors().Get("medium");
		Color valueColor = *GameData::Colors().Get("bright");
		
		Table table;
		// Use 10-pixel margins on both sides.
		table.AddColumn(10, Table::LEFT);
		table.AddColumn(WIDTH - 10, Table::RIGHT);
		table.DrawAt(point);
		
		for(unsigned i = 0; i < labels.size() && i < values.size(); ++i)
		{
			if(labels[i].empty())
			{
				table.DrawGap(10);
				continue;
			}
			
			table.Draw(labels[i], values[i].empty() ? valueColor : labelColor);
			table.Draw(values[i], valueColor);
		}
		return table.GetPoint();
	}
}



OutfitInfoDisplay::OutfitInfoDisplay()
	: descriptionHeight(0), requirementsHeight(0), attributesHeight(0), maximumHeight(0)
{
}



OutfitInfoDisplay::OutfitInfoDisplay(const Outfit &outfit)
{
	Update(outfit);
}



// Call this every time the ship changes.
void OutfitInfoDisplay::Update(const Outfit &outfit)
{
	UpdateDescription(outfit);
	UpdateRequirements(outfit);
	UpdateAttributes(outfit);
	
	maximumHeight = max(descriptionHeight, max(requirementsHeight, attributesHeight));
}



// Get the panel width.
int OutfitInfoDisplay::PanelWidth()
{
	return WIDTH;
}



// Get the height of each of the three panels.
int OutfitInfoDisplay::MaximumHeight() const
{
	return maximumHeight;
}



int OutfitInfoDisplay::DescriptionHeight() const
{
	return descriptionHeight;
}



int OutfitInfoDisplay::RequirementsHeight() const
{
	return requirementsHeight;
}



int OutfitInfoDisplay::AttributesHeight() const
{
	return attributesHeight;
}



// Draw each of the panels.
void OutfitInfoDisplay::DrawDescription(const Point &topLeft) const
{
	description.Draw(topLeft + Point(10., 3.), *GameData::Colors().Get("medium"));
}



void OutfitInfoDisplay::DrawRequirements(const Point &topLeft) const
{
	Draw(topLeft, requirementLabels, requirementValues);
}



void OutfitInfoDisplay::DrawAttributes(const Point &topLeft) const
{
	Draw(topLeft, attributeLabels, attributeValues);
}



void OutfitInfoDisplay::UpdateDescription(const Outfit &outfit)
{
	description.SetAlignment(WrappedText::JUSTIFIED);
	description.SetWrapWidth(WIDTH - 20);
	description.SetFont(FontSet::Get(14));
	
	description.Wrap(outfit.Description());
	
	// Pad by 10 pixels on the top and bottom.
	descriptionHeight = description.Height() + 20;
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
		"guns ports needed:", "gun ports",
		"turret mounts needed:", "turrent mounts"
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
		if(it.first == "cost" || it.first == "outfit space"
				|| it.first == "weapon capacity" || it.first == "engine capacity"
				|| it.first == "gun ports" || it.first == "turret mounts")
			continue;
		
		attributeLabels.push_back(it.first + ':');
		attributeValues.push_back(Format::Number(it.second));
		attributesHeight += 20;
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
	
	if(outfit.ShieldDamage())
	{
		attributeLabels.push_back("shield damage / second:");
		attributeValues.push_back(Format::Number(
			60. * outfit.ShieldDamage() / outfit.WeaponGet("reload")));
		attributesHeight += 20;
	}
	
	if(outfit.HullDamage())
	{
		attributeLabels.push_back("hull damage / second:");
		attributeValues.push_back(Format::Number(
			60. * outfit.HullDamage() / outfit.WeaponGet("reload")));
		attributesHeight += 20;
	}
	
	int homing = outfit.WeaponGet("homing");
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
		attributeValues.push_back(skill[homing]);
		attributesHeight += 20;
	}
	
	attributeLabels.push_back(string());
	attributeValues.push_back(string());
	attributesHeight += 10;
	
	static const string names[] = {
		"inaccuracy",
		"firing energy",
		"firing heat",
		"blast radius",
		"missile strength",
		"anti-missile",
		"capture attack",
		"capture defense"
	};
	static const int NAMES =  sizeof(names) / sizeof(names[0]);
	for(int i = 0; i < NAMES; ++i)
		if(outfit.WeaponGet(names[i]))
		{
			attributeLabels.push_back(names[i]);
			attributeValues.push_back(Format::Number(outfit.WeaponGet(names[i])));
			attributesHeight += 20;
		}
}
