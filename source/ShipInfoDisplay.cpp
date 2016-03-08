/* ShipInfoDisplay.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "ShipInfoDisplay.h"

#include "Color.h"
#include "Format.h"
#include "GameData.h"
#include "Outfit.h"
#include "Ship.h"
#include "Table.h"

#include <algorithm>
#include <map>

using namespace std;



ShipInfoDisplay::ShipInfoDisplay(const Ship &ship)
{
	Update(ship);
}



// Call this every time the ship changes.
void ShipInfoDisplay::Update(const Ship &ship)
{
	UpdateDescription(ship);
	UpdateAttributes(ship);
	UpdateOutfits(ship);
	
	maximumHeight = max(descriptionHeight, max(attributesHeight, outfitsHeight));
}



int ShipInfoDisplay::OutfitsHeight() const
{
	return outfitsHeight;
}



int ShipInfoDisplay::SaleHeight() const
{
	return saleHeight;
}



// Draw each of the panels.
void ShipInfoDisplay::DrawAttributes(const Point &topLeft) const
{
	Point point = Draw(topLeft, attributeLabels, attributeValues);
	
	// Get standard colors to draw with.
	Color labelColor = *GameData::Colors().Get("medium");
	Color valueColor = *GameData::Colors().Get("bright");
	
	Table table;
	table.AddColumn(10, Table::LEFT);
	table.AddColumn(WIDTH - 90, Table::RIGHT);
	table.AddColumn(WIDTH - 10, Table::RIGHT);
	table.DrawAt(point);
	table.DrawGap(10.);
	
	table.Advance();
	table.Draw("energy", labelColor);
	table.Draw("heat", labelColor);
	
	for(unsigned i = 0; i < tableLabels.size(); ++i)
	{
		table.Draw(tableLabels[i], labelColor);
		table.Draw(energyTable[i], valueColor);
		table.Draw(heatTable[i], valueColor);
	}
}



void ShipInfoDisplay::DrawOutfits(const Point &topLeft) const
{
	Draw(topLeft, outfitLabels, outfitValues);
}



void ShipInfoDisplay::DrawSale(const Point &topLeft) const
{
	Draw(topLeft, saleLabels, saleValues);
}



void ShipInfoDisplay::UpdateDescription(const Ship &ship)
{
	const vector<string> &licenses = ship.Licenses();
	if(licenses.empty())
		ItemInfoDisplay::UpdateDescription(ship.Description());
	else
	{
		string text = ship.Description() + "\tTo purchase this ship you must have ";
		for(unsigned i = 0; i < licenses.size(); ++i)
		{
			if(i)
			{
				if(licenses.size() > 2)
					text += ", ";
				else
					text += " ";
			}
			if(i && i == licenses.size() - 1)
				text += "and ";
			text += "a " + licenses[i] + " License";
		}
		text += ".";
		ItemInfoDisplay::UpdateDescription(text);
	}
}



void ShipInfoDisplay::UpdateAttributes(const Ship &ship)
{
	bool isGeneric = ship.Name().empty() || ship.GetPlanet();
	
	attributeLabels.clear();
	attributeValues.clear();
	attributesHeight = 20;
	
	const Outfit &attributes = ship.Attributes();
	
	attributeLabels.push_back("cost:");
	attributeValues.push_back(Format::Number(ship.Cost()));
	attributesHeight += 20;
	
	attributeLabels.push_back(string());
	attributeValues.push_back(string());
	attributesHeight += 10;
	if(attributes.Get("shield generation"))
	{
		attributeLabels.push_back("shields charge / max:");
		attributeValues.push_back(Format::Number(60. * attributes.Get("shield generation"))
			+ " / " + Format::Number(attributes.Get("shields")));
	}
	else
	{
		attributeLabels.push_back("shields:");
		attributeValues.push_back(Format::Number(attributes.Get("shields")));
	}
	attributesHeight += 20;
	if(attributes.Get("hull repair rate"))
	{
		attributeLabels.push_back("hull repair / max:");
		attributeValues.push_back(Format::Number(60. * attributes.Get("hull repair rate"))
			+ " / " + Format::Number(attributes.Get("hull")));
	}
	else
	{
		attributeLabels.push_back("hull:");
		attributeValues.push_back(Format::Number(attributes.Get("hull")));
	}
	attributesHeight += 20;
	double emptyMass = ship.Mass();
	attributeLabels.push_back(isGeneric ? "mass with no cargo:" : "mass:");
	attributeValues.push_back(Format::Number(emptyMass));
	attributesHeight += 20;
	attributeLabels.push_back(isGeneric ? "cargo space:" : "cargo:");
	if(isGeneric)
		attributeValues.push_back(Format::Number(attributes.Get("cargo space")));
	else
		attributeValues.push_back(Format::Number(ship.Cargo().Used())
			+ " / " + Format::Number(attributes.Get("cargo space")));
	attributesHeight += 20;
	attributeLabels.push_back("required crew / bunks:");
	attributeValues.push_back(Format::Number(ship.RequiredCrew())
		+ " / " + Format::Number(attributes.Get("bunks")));
	attributesHeight += 20;
	attributeLabels.push_back(isGeneric ? "fuel capacity:" : "fuel:");
	double fuelCapacity = attributes.Get("fuel capacity");
	if(isGeneric)
		attributeValues.push_back(Format::Number(fuelCapacity));
	else
		attributeValues.push_back(Format::Number(ship.Fuel() * fuelCapacity)
			+ " / " + Format::Number(fuelCapacity));
	attributesHeight += 20;
	
	double fullMass = emptyMass + (isGeneric ? attributes.Get("cargo space") : ship.Cargo().Used());
	isGeneric &= (fullMass != emptyMass);
	attributeLabels.push_back(string());
	attributeValues.push_back(string());
	attributesHeight += 10;
	attributeLabels.push_back(isGeneric ? "movement, full / no cargo:" : "movement:");
	attributeValues.push_back(string());
	attributesHeight += 20;
	attributeLabels.push_back("max speed:");
	attributeValues.push_back(Format::Number(60. * attributes.Get("thrust") / attributes.Get("drag")));
	attributesHeight += 20;
	
	attributeLabels.push_back("acceleration:");
	if(!isGeneric)
		attributeValues.push_back(Format::Number(3600. * attributes.Get("thrust") / fullMass));
	else
		attributeValues.push_back(Format::Number(3600. * attributes.Get("thrust") / fullMass)
			+ " / " + Format::Number(3600. * attributes.Get("thrust") / emptyMass));
	attributesHeight += 20;
	
	attributeLabels.push_back("turning:");
	if(!isGeneric)
		attributeValues.push_back(Format::Number(60. * attributes.Get("turn") / fullMass));
	else
		attributeValues.push_back(Format::Number(60. * attributes.Get("turn") / fullMass)
			+ " / " + Format::Number(60. * attributes.Get("turn") / emptyMass));
	attributesHeight += 20;
	
	// Find out how much outfit, engine, and weapon space the chassis has.
	map<string, double> chassis;
	static const string names[] = {
		"outfit space free:", "outfit space",
		"    weapon capacity:", "weapon capacity",
		"    engine capacity:", "engine capacity",
		"guns ports free:", "gun ports",
		"turret mounts free:", "turret mounts"
	};
	static const int NAMES =  sizeof(names) / sizeof(names[0]);
	for(int i = 1; i < NAMES; i += 2)
		chassis[names[i]] = attributes.Get(names[i]);
	for(const auto &it : ship.Outfits())
		for(auto &cit : chassis)
			cit.second -= it.GetQuantity() * it.GetOutfit()->Get(cit.first);
	
	attributeLabels.push_back(string());
	attributeValues.push_back(string());
	attributesHeight += 10;
	for(int i = 0; i < NAMES; i += 2)
	{
		attributeLabels.push_back(names[i]);
		attributeValues.push_back(Format::Number(attributes.Get(names[i + 1]))
			+ " / " + Format::Number(chassis[names[i + 1]]));
		attributesHeight += 20;
	}
	
	if(ship.DroneBaysFree())
	{
		attributeLabels.push_back("drone bays:");
		attributeValues.push_back(to_string(ship.DroneBaysFree()));
		attributesHeight += 20;
	}
	if(ship.FighterBaysFree())
	{
		attributeLabels.push_back("fighter bays:");
		attributeValues.push_back(to_string(ship.FighterBaysFree()));
		attributesHeight += 20;
	}
	
	tableLabels.clear();
	energyTable.clear();
	heatTable.clear();
	// Skip a spacer and the table header.
	attributesHeight += 30;
	
	tableLabels.push_back("idle:");
	energyTable.push_back(Format::Number(60. * attributes.Get("energy generation")));
	heatTable.push_back(Format::Number(
		60. * (attributes.Get("heat generation") - attributes.Get("cooling"))));
	attributesHeight += 20;
	tableLabels.push_back("moving:");
	energyTable.push_back(Format::Number(
		-60. * (attributes.Get("thrusting energy")
			+ attributes.Get("reverse thrusting energy")
			+ attributes.Get("turning energy"))));
	heatTable.push_back(Format::Number(
		60. * (attributes.Get("thrusting heat")
			+ attributes.Get("reverse thrusting heat")
			+ attributes.Get("turning heat"))));
	attributesHeight += 20;
	double firingEnergy = 0.;
	double firingHeat = 0.;
	for(const auto &it : ship.Outfits())
		if(it.GetOutfit()->IsWeapon() && it.GetOutfit()->Reload())
		{
			firingEnergy += it.GetQuantity() * it.GetOutfit()->FiringEnergy() / it.GetOutfit()->Reload();
			firingHeat += it.GetQuantity() * it.GetOutfit()->FiringHeat() / it.GetOutfit()->Reload();
		}
	tableLabels.push_back("firing:");
	energyTable.push_back(Format::Number(-60. * firingEnergy));
	heatTable.push_back(Format::Number(60. * firingHeat));
	attributesHeight += 20;
	tableLabels.push_back("max:");
	energyTable.push_back(Format::Number(attributes.Get("energy capacity")));
	heatTable.push_back(Format::Number(60. * emptyMass * .1 * attributes.Get("heat dissipation")));
	// Pad by 10 pixels on the top and bottom.
	attributesHeight += 30;
}



void ShipInfoDisplay::UpdateOutfits(const Ship &ship)
{
	outfitLabels.clear();
	outfitValues.clear();
	outfitsHeight = 20;
	int outfitsValue = ship.Outfits().GetTotalCost();
	
	map<string, map<string, int>> listing;
	for(const auto &it : ship.Outfits())
	{
		listing[it.GetOutfit()->Category()][it.GetOutfit()->Name()] += it.GetQuantity();
	}
	
	for(const auto &cit : listing)
	{
		// Pad by 10 pixels before each category.
		if(&cit != &*listing.begin())
		{
			outfitLabels.push_back(string());
			outfitValues.push_back(string());
			outfitsHeight += 10;
		}
				
		outfitLabels.push_back(cit.first + ':');
		outfitValues.push_back(string());
		outfitsHeight += 20;
		
		for(const auto &it : cit.second)
		{
			outfitLabels.push_back(it.first);
			outfitValues.push_back(to_string(it.second));
			outfitsHeight += 20;
		}
	}
	
	
	saleLabels.clear();
	saleValues.clear();
	saleHeight = 20;
	int totalValue = ship.Attributes().Cost();
	
	saleLabels.push_back("This ship will sell for:");
	saleValues.push_back(string());
	saleHeight += 20;
	saleLabels.push_back("empty hull:");
	saleValues.push_back(Format::Number(totalValue - outfitsValue) + " credits");
	saleHeight += 20;
	saleLabels.push_back("  + outfits:");
	saleValues.push_back(Format::Number(outfitsValue) + " credits");
	saleHeight += 20;
	saleLabels.push_back("= total:");
	saleValues.push_back(Format::Number(totalValue) + " credits");
	saleHeight += 20;
}
