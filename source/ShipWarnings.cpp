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
#include "GameData.h"
#include "Outfit.h"
#include "Point.h"
#include "Ship.h"
#include "Sprite.h"
#include "SpriteSet.h"
#include "SpriteShader.h"

#include <map>

using namespace std;

namespace {
	struct ShipWarning {
		string icon;
		string label;
	};

	// Labels that end with '!' are serious warnings.
	static const string ICON_SERIOUS = "ui/icon error.png";
	static const string ICON_OTHER = "ui/icon warning.png";
	// bit, warning
	static const map<int,ShipWarning> WARNINGS = {
		// serious warnings
		{ 0, { "ui/icon error", "ship warning: no energy!"   } },
		{ 1, { "ui/icon error", "ship warning: no steering!" } },
		{ 2, { "ui/icon error", "ship warning: no thruster!" } },
		{ 3, { "ui/icon error", "ship warning: overheating!" } },
		// other warnings
		{ 4, { "ui/icon warning", "ship warning: afterburner only" } },
		{ 5, { "ui/icon warning", "ship warning: battery only"     } },
		{ 6, { "ui/icon warning", "ship warning: limited movement" } },
		{ 7, { "ui/icon warning", "ship warning: solar power"      } }
	};
}



ShipWarnings::ShipWarnings()
{
}



ShipWarnings::ShipWarnings(const Ship &ship, int updateBits)
{
	Update(ship, updateBits);
}



// Check for problems in the ship.
void ShipWarnings::Update(const Ship &ship, int updateBits)
{
	const Outfit &attributes = ship.Attributes();
	int newWarnings = 0;
	
	double energyBalance = attributes.Get("energy generation") - attributes.Get("energy consumption");
	energyBalance += attributes.Get("solar collection");
	double energy = energyBalance + attributes.Get("energy capacity");
	if(energy <= 0.)
		newWarnings |= NO_ENERGY;
	
	if(!attributes.Get("turn"))
		newWarnings |= NO_STEERING;
	
	if(!attributes.Get("thrust") && !attributes.Get("reverse thrust") && !attributes.Get("afterburner thrust"))
		newWarnings |= NO_THRUSTER;
	
	if(ship.IdleHeat() >= 100. * ship.Mass())
		newWarnings |= OVERHEATING;
	
	if(attributes.Get("afterburner thrust") && !attributes.Get("thrust") && !attributes.Get("reverse thrust"))
		newWarnings |= AFTERBURNER_ONLY;
	
	if(energyBalance <= 0.)
		newWarnings |= BATTERY_ONLY;
	
	if(attributes.Get("thrusting energy") > energy || attributes.Get("turning energy") > energy)
		newWarnings |= LIMITED_MOVEMENT;
	
	// At infinite distance from a star only 20% of the maximum solar power is collected.
	double energyLoss = .8 * attributes.Get("solar collection");
	if(energyBalance > 0. && energyLoss > 0. && energyBalance < energyLoss)
		newWarnings |= SOLAR_POWER;
	
	// Update requested bits.
	warnings = (warnings & ~updateBits) | (newWarnings & updateBits);
}



void ShipWarnings::Draw(const Point &center)
{
	const vector<string> icons = WarningIcons();
	if(icons.empty())
		return;
	
	Point offset(-.5 * iconSize * (icons.size() - 1), 0.);
	for(const string& icon : icons)
	{
		const Sprite *sprite = SpriteSet::Get(icon);
		const double zoom = max(iconSize / sprite->Width(), iconSize / sprite->Height());
		SpriteShader::Draw(sprite, center + offset, zoom);
		offset.X() += iconSize;
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
			const string icon = it != WARNINGS.end() ? it->second.icon : "ui/icon warning";
			if(packWarnings > 0 && icons.size() == packWarnings)
				break;
			icons.push_back(icon);
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
			const string label = it != WARNINGS.end() ? it->second.label : "ship warning: unknown bit " + to_string(bit);
			if(packWarnings > 0 && labels.size() == packWarnings)
				labels[labels.size() - 1] += "\n" + label;
			else
				labels.push_back(label);
		}
	}
	return labels;
}



// Click zones matching what is drawn and tooltip names as values.
vector<ClickZone<string>> ShipWarnings::TooltipZones(const Point &center) const
{
	vector<ClickZone<string>> zones;
	
	const vector<string> labels = WarningLabels();
	if(labels.empty())
		return zones;
	
	Point offset(-.5 * iconSize * (labels.size() - 1), 0.);
	const Point dimensions(iconSize, iconSize);
	for(const string& label : labels)
	{
		zones.emplace_back(center + offset, dimensions, label);
		offset.X() += dimensions.X();
	}
	return zones;
}



// Maximum number of warnings that will be drawn. A value of 0 means no limit.
// Extra warnings are not displayed and instead have their labels concateneted with
// '\n' to the label of the last warning that is displayed.
int ShipWarnings::PackWarnings() const
{
	return packWarnings;
}



void ShipWarnings::SetPackWarnings(size_t num)
{
	packWarnings = num;
}



// Icons are square. The icon image is resized to match the target size.
double ShipWarnings::IconSize() const
{
	return iconSize;
}



void ShipWarnings::SetIconSize(double size)
{
	iconSize = size;
}



Point ShipWarnings::Dimensions() const
{
	if(iconSize <= 0. || warnings == 0)
		return Point(0., 0.);
	
	size_t numIcons = 0;
	for(int bit = 0; (warnings >> bit) != 0; ++bit)
	{
		bool hasBit = (warnings >> bit) & 1;
		if(hasBit)
		{
			++numIcons;
			if(packWarnings > 0 && numIcons == packWarnings)
				break;
		}
	}
	return Point(iconSize * numIcons, iconSize);
}
