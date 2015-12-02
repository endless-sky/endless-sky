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
#include "FontSet.h"
#include "Format.h"
#include "GameData.h"
#include "Outfit.h"
#include "Table.h"
#include "SpriteSet.h"
#include "Point.h"

#include <cmath>
#include <map>
#include <set>
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
	
	
	
	Point DrawWithColor(Point lpoint, Point rpoint, const vector<string> &llabels, const vector<string> &lvalues, const vector<string> &rlabels, const vector<string> &rvalues)
	{
		// Get standard colors to draw with.
		Color defLabelColor = *GameData::Colors().Get("medium");
		Color defValueColor = *GameData::Colors().Get("bright");
		
		//Colors for comparing need to be added
		Color negLabel;
		Color posLabel;
		Color negValue;
		Color posValue;
		
		Table ltable;
		ltable.AddColumn(10, Table::LEFT);
		ltable.AddColumn(WIDTH - 10, Table::RIGHT);
		ltable.DrawAt(lpoint);
		
		Table rtable;
		rtable.AddColumn(10, Table::LEFT);
		rtable.AddColumn(WIDTH - 10, Table::RIGHT);
		rtable.DrawAt(rpoint);
		
		
		for(unsigned i = 0; (i < llabels.size() && i < lvalues.size()) || (rlabels.size() && i < rvalues.size()); ++i)
		{
			Color llabelColor = defLabelColor;
			Color lvalueColor = defValueColor;
			Color rlabelColor = defLabelColor;
			Color rvalueColor = defValueColor;
			
			
			//Compare here  (cbb someone else can work on this...)
			//Change colors as needed
			//End
			
			//Draw gaps, etc. if length of vec are not equal
			if (i >= llabels.size() && i >= lvalues.size()) {//add lines onto right
				if (rlabels[i].empty())
				{
					rtable.DrawGap(10);
				}
				else
				{
					rtable.Draw(rlabels[i], rvalues[i].empty() ? rvalueColor : rlabelColor);
					rtable.Draw(rvalues[i], rvalueColor);
				}
				continue;
			}
			else if (i >= rlabels.size() && i >= rvalues.size()) {//add lines onto left
				if (llabels[i].empty())
				{
					ltable.DrawGap(10);
				}
				else
				{
					ltable.Draw(llabels[i], lvalues[i].empty() ? lvalueColor : llabelColor);
					ltable.Draw(lvalues[i], lvalueColor);
				}
				continue;
			}
			
			
			//Draw gap for when it's empty
			if(llabels[i].empty() && rlabels[i].empty())
			{
				ltable.DrawGap(10);
				rtable.DrawGap(10);
				continue;
			}
			else if(llabels[i].empty())
			{
				ltable.DrawGap(10);
				rtable.Draw(rlabels[i], rvalues[i].empty() ? rvalueColor : rlabelColor);
				rtable.Draw(rvalues[i], rvalueColor);
				continue;
			}
			else if (rlabels[i].empty())
			{
				rtable.DrawGap(10);
				ltable.Draw(llabels[i], lvalues[i].empty() ? lvalueColor : llabelColor);
				ltable.Draw(lvalues[i], lvalueColor);
				continue;
			}
			
			
			//Standard draw
			ltable.Draw(llabels[i], lvalues[i].empty() ? lvalueColor : llabelColor);
			ltable.Draw(lvalues[i], lvalueColor);
			rtable.Draw(rlabels[i], rvalues[i].empty() ? rvalueColor : rlabelColor);
			rtable.Draw(rvalues[i], rvalueColor);
			
		}
		
		
		return ltable.GetPoint();
	}
	
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
		"reverse thrusting energy",
		"reverse thrusting heat",
		"shield generation",
		"thrusting energy",
		"thrusting heat",
		"turn",
		"turning energy",
		"turning heat"
	};
}



OutfitInfoDisplay::OutfitInfoDisplay()
	: descriptionHeight(0), requirementsHeight(0), attributesHeight(0), maximumHeight(0)
{
}



OutfitInfoDisplay::OutfitInfoDisplay(const Outfit &outfit)
{
	Update(outfit);
}



OutfitInfoDisplay::OutfitInfoDisplay(const Outfit &outfit, const Outfit &stcOutfit)
{
	//Assume that this is only used for the comparing panels in map
	UpdateBothAtt(stcOutfit,outfit);
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
	description.Draw(topLeft + Point(10., 2.), *GameData::Colors().Get("medium"));
}



void OutfitInfoDisplay::DrawRequirements(const Point &topLeft) const
{
	Draw(topLeft, requirementLabels, requirementValues);
}



void OutfitInfoDisplay::DrawAttributes(const Point &topLeft, bool comparative) const
{
	if(comparative)
	{
		Point size(PanelWidth(), AttributesHeight());
		Point sec = topLeft + Point(-size.X()-0.5,0);
		DrawWithColor(sec, topLeft, stcAttributeLabels, stcAttributeValues, attributeLabels, attributeValues);
	}
	else
	{
		//standard
		Draw(topLeft, attributeLabels, attributeValues);
	}
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

void OutfitInfoDisplay::UpdateBothAtt(const Outfit &lftOft, const Outfit &rgtOft)
{
	//Update normally
	UpdateAttributes(rgtOft);
	
	
	stcAttributeLabels.clear();
	stcAttributeValues.clear();
	
	//temp to ensure height is the maximum
	int tempAttributesHeight = 20;
	
	map<string, map<string, int>> listing;
	for(const auto &it : lftOft.Attributes())
	{
		if(it.first == "cost" || it.first == "outfit space"
		   || it.first == "weapon capacity" || it.first == "engine capacity"
		   || it.first == "gun ports" || it.first == "turret mounts")
			continue;
		
		string value;
		double scale = 1.;
		if(it.first == "thrust" || it.first == "reverse thrust" || it.first == "afterburner thrust")
			scale = 60. * 60.;
		else if(ATTRIBUTES_TO_SCALE.find(it.first) != ATTRIBUTES_TO_SCALE.end())
			scale = 60.;
		
		stcAttributeLabels.push_back(it.first + ':');
		stcAttributeValues.push_back(Format::Number(it.second * scale));
		tempAttributesHeight += 20;
	}
	
	if(!lftOft.IsWeapon())
		return;
	
	stcAttributeLabels.push_back(string());
	stcAttributeValues.push_back(string());
	tempAttributesHeight += 10;
	
	if(lftOft.Ammo())
	{
		stcAttributeLabels.push_back("ammo:");
		stcAttributeValues.push_back(lftOft.Ammo()->Name());
		tempAttributesHeight += 20;
	}
	
	stcAttributeLabels.push_back("range:");
	stcAttributeValues.push_back(Format::Number(lftOft.Range()));
	tempAttributesHeight += 20;
	
	if(lftOft.ShieldDamage() && lftOft.Reload())
	{
		stcAttributeLabels.push_back("shield damage / second:");
		stcAttributeValues.push_back(Format::Number(60. * lftOft.ShieldDamage() / lftOft.Reload()));
		tempAttributesHeight += 20;
	}
	
	if(lftOft.HullDamage() && lftOft.Reload())
	{
		stcAttributeLabels.push_back("hull damage / second:");
		stcAttributeValues.push_back(Format::Number(60. * lftOft.HullDamage() / lftOft.Reload()));
		tempAttributesHeight += 20;
	}
	
	if(lftOft.HeatDamage() && lftOft.Reload())
	{
		stcAttributeLabels.push_back("heat damage / second:");
		stcAttributeValues.push_back(Format::Number(60. * lftOft.HeatDamage() / lftOft.Reload()));
		tempAttributesHeight += 20;
	}
	
	if(lftOft.IonDamage() && lftOft.Reload())
	{
		stcAttributeLabels.push_back("ion damage / second:");
		stcAttributeValues.push_back(Format::Number(6000. * lftOft.IonDamage() / lftOft.Reload()));
		tempAttributesHeight += 20;
	}
	
	if(lftOft.FiringEnergy() && lftOft.Reload())
	{
		stcAttributeLabels.push_back("firing energy / second:");
		stcAttributeValues.push_back(Format::Number(60. * lftOft.FiringEnergy() / lftOft.Reload()));
		tempAttributesHeight += 20;
	}
	
	if(lftOft.FiringHeat() && lftOft.Reload())
	{
		stcAttributeLabels.push_back("firing heat / second:");
		stcAttributeValues.push_back(Format::Number(60. * lftOft.FiringHeat() / lftOft.Reload()));
		tempAttributesHeight += 20;
	}
	
	if(lftOft.FiringFuel() && lftOft.Reload())
	{
		stcAttributeLabels.push_back("firing fuel / second:");
		stcAttributeValues.push_back(Format::Number(60. * lftOft.FiringFuel() / lftOft.Reload()));
		tempAttributesHeight += 20;
	}
	
	bool isContinuous = (lftOft.Reload() <= 1);
	stcAttributeLabels.push_back("shots / second:");
	if(isContinuous)
		stcAttributeValues.push_back("continuous");
	else
		stcAttributeValues.push_back(Format::Number(60. / lftOft.Reload()));
	
	tempAttributesHeight += 20;
	
	int homing = lftOft.Homing();
	if(homing)
	{
		static const string skill[] = {
			"no",
			"poor",
			"fair",
			"good",
			"excellent"
		};
		stcAttributeLabels.push_back("homing:");
		stcAttributeValues.push_back(skill[max(0, min(4, homing))]);
		tempAttributesHeight += 20;
	}
	
	stcAttributeLabels.push_back(string());
	stcAttributeValues.push_back(string());
	tempAttributesHeight += 10;
	
	static const string names[] = {
		"shield damage / shot: ",
		"hull damage / shot: ",
		"heat damage / shot: ",
		"ion damage / shot: ",
		"firing energy / shot:",
		"firing heat / shot:",
		"firing fuel / shot:",
		"inaccuracy:",
		"blast radius:",
		"missile strength:",
		"anti-missile:",
	};
	double values[] = {
		lftOft.ShieldDamage(),
		lftOft.HullDamage(),
		lftOft.HeatDamage(),
		lftOft.IonDamage() * 100.,
		lftOft.FiringEnergy(),
		lftOft.FiringHeat(),
		lftOft.FiringFuel(),
		lftOft.Inaccuracy(),
		lftOft.BlastRadius(),
		static_cast<double>(lftOft.MissileStrength()),
		static_cast<double>(lftOft.AntiMissile())
	};
	static const int NAMES = sizeof(names) / sizeof(names[0]);
	for(int i = (isContinuous ? 7 : 0); i < NAMES; ++i)
		if(values[i])
		{
			stcAttributeLabels.push_back(names[i]);
			stcAttributeValues.push_back(Format::Number(values[i]));
			tempAttributesHeight += 20;
		}
	
	if(tempAttributesHeight>attributesHeight)
		while (tempAttributesHeight>attributesHeight) {
			
			attributeLabels.push_back(string());
			attributeValues.push_back(string());
			attributesHeight+=10;
		}
	
	if(tempAttributesHeight<attributesHeight)
	{
		while (tempAttributesHeight<attributesHeight) {
			
			stcAttributeLabels.push_back(string());
			stcAttributeValues.push_back(string());
			tempAttributesHeight+=10;
		}
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
		
		string value;
		double scale = 1.;
		if(it.first == "thrust" || it.first == "reverse thrust" || it.first == "afterburner thrust")
			scale = 60. * 60.;
		else if(ATTRIBUTES_TO_SCALE.find(it.first) != ATTRIBUTES_TO_SCALE.end())
			scale = 60.;
		
		attributeLabels.push_back(it.first + ':');
		attributeValues.push_back(Format::Number(it.second * scale));
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
	
	attributeLabels.push_back(string());
	attributeValues.push_back(string());
	attributesHeight += 10;
	
	static const string names[] = {
		"shield damage / shot: ",
		"hull damage / shot: ",
		"heat damage / shot: ",
		"ion damage / shot: ",
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
		outfit.FiringEnergy(),
		outfit.FiringHeat(),
		outfit.FiringFuel(),
		outfit.Inaccuracy(),
		outfit.BlastRadius(),
		static_cast<double>(outfit.MissileStrength()),
		static_cast<double>(outfit.AntiMissile())
	};
	static const int NAMES = sizeof(names) / sizeof(names[0]);
	for(int i = (isContinuous ? 7 : 0); i < NAMES; ++i)
		if(values[i])
		{
			attributeLabels.push_back(names[i]);
			attributeValues.push_back(Format::Number(values[i]));
			attributesHeight += 20;
		}
}
