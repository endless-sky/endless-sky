/* ShipWarnings.cpp
Copyright (c) 2017 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "ShipWarnings.h"

#include "Color.h"
#include "FillShader.h"
#include "Font.h"
#include "FontSet.h"
#include "GameData.h"
#include "Outfit.h"
#include "Point.h"
#include "Ship.h"

#include <map>

using namespace std;

namespace {
	static map<int,pair<string,string>> WARNINGS = {
		// bit, <icon, label>
		{ 0, make_pair("E",  "flight check: no energy"           ) },
		{ 1, make_pair("T",  "flight check: no thrusters"        ) },
		{ 2, make_pair("TE", "flight check: no thruster energy"  ) },
		{ 3, make_pair("S",  "flight check: no steering"         ) },
		{ 4, make_pair("SE", "flight check: no steering energy"  ) },
		{ 5, make_pair("SP", "flight check: solar power warning" ) },
		{ 6, make_pair("O",  "flight check: overheating"         ) }
	};
}



ShipWarnings::ShipWarnings()
{
}



ShipWarnings::ShipWarnings(const Ship &ship, int updateBits)
{
	Update(ship, updateBits);
}



// Check for problems that prevent the ship from flying.
void ShipWarnings::Update(const Ship &ship, int updateBits)
{
	const Outfit &attributes = ship.Attributes();
	int newWarnings = 0;
	
	double energyBalance = attributes.Get("energy generation") - attributes.Get("energy consumption");
	energyBalance += attributes.Get("solar collection");
	double energy = energyBalance + attributes.Get("energy capacity");
	if(energyBalance <= 0. || energy < 0.)
		newWarnings |= NO_ENERGY;
	
	if(!attributes.Get("thrust") && !attributes.Get("reverse thrust")
			&& !attributes.Get("afterburner thrust"))
		newWarnings |= NO_THRUSTERS;
	
	if(attributes.Get("thrusting energy") > energy)
		newWarnings |= NO_THRUSTER_ENERGY;
	
	if(!attributes.Get("turn"))
		newWarnings |= NO_STEERING;
	
	if(attributes.Get("turning energy") > energy)
		newWarnings |= NO_STEERING_ENERGY;
	
	// Check energy balance with solar collection at infinite distance from a star (only 20% of
	// maximum solar collection).
	double energyLoss = .8 * attributes.Get("solar collection");
	energy -= energyLoss;
	if(energyLoss > 0. && (energyBalance < energyLoss || attributes.Get("thrusting energy") > energy || attributes.Get("turning energy") > energy))
		newWarnings |= SOLAR_POWER_WARNING;
	
	if(ship.IdleHeat() >= 100. * ship.Mass())
		newWarnings |= OVERHEATING;
	
	// Update requested bits.
	warnings = (warnings & ~updateBits) | (newWarnings & updateBits);
}



void ShipWarnings::Draw(const Point &center, bool blink)
{
	if(warnings == 0)
		return;
	
	const Font &font = FontSet::Get(14);
	const Color &bright = *GameData::Colors().Get("bright");
	const vector<string> icons = WarningIcons();
	
	Point offset(10. -10. * icons.size(), -.5 * font.Height());
	for(const string& icon : icons)
	{
		const Rectangle zone(center + offset, Point(font.Width(icon), font.Height()));
		if(blink)
			FillShader::Fill(zone.Center(), zone.Dimensions(), bright.Additive(.25));
		font.Draw(icon, zone.TopLeft(), bright);
		offset.X() += 20.;
	}
}



int ShipWarnings::Warnings() const
{
	return warnings;
}



vector<string> ShipWarnings::WarningIcons() const
{
	vector<string> icons;
	for(int bit = 0; (warnings >> bit) != 0; ++bit)
	{
		bool hasBit = (warnings >> bit) & 1;
		if(hasBit)
		{
			auto it = WARNINGS.find(bit);
			icons.push_back(it != WARNINGS.end() ? it->second.first : "?");
		}
	}
	return icons;
}



vector<string> ShipWarnings::WarningLabels() const
{
	vector<string> labels;
	for(int bit = 0; (warnings >> bit) != 0; ++bit)
	{
		bool hasBit = (warnings >> bit) & 1;
		if(hasBit)
		{
			auto it = WARNINGS.find(bit);
			labels.push_back(it != WARNINGS.end() ? it->second.second : "unknown ship warning bit " + to_string(bit));
		}
	}
	return labels;
}



// Click zones matching what is drawn and tooltip names as values.
vector<ClickZone<string>> ShipWarnings::TooltipZones(const Point &center) const
{
	vector<ClickZone<string>> zones;
	
	const Font &font = FontSet::Get(14);
	const vector<string> icons = WarningIcons();
	const vector<string> labels = WarningLabels();
	
	Point offset(10. -10. * icons.size(), -.5 * font.Height());
	for(size_t i = 0; i < icons.size(); ++i)
	{
		const Point dimensions(font.Width(icons[i]), font.Height());
		zones.emplace_back(center + offset, dimensions, labels[i]);
		offset.X() += 20.;
	}
	return zones;
}
