/* ShipInfoDisplay.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "ShipInfoDisplay.h"

#include "text/alignment.hpp"
#include "CategoryList.h"
#include "CategoryTypes.h"
#include "Color.h"
#include "Depreciation.h"
#include "FillShader.h"
#include "text/Format.h"
#include "GameData.h"
#include "text/layout.hpp"
#include "Outfit.h"
#include "PlayerInfo.h"
#include "Ship.h"
#include "text/Table.h"

#include <algorithm>
#include <map>
#include <sstream>

using namespace std;



ShipInfoDisplay::ShipInfoDisplay(const Ship &ship, const PlayerInfo &player, bool descriptionCollapsed)
{
	Update(ship, player, descriptionCollapsed);
}



// Call this every time the ship changes.
void ShipInfoDisplay::Update(const Ship &ship, const PlayerInfo &player, bool descriptionCollapsed)
{
	UpdateDescription(ship.Description(), ship.Attributes().Licenses(), true);
	UpdateAttributes(ship, player, descriptionCollapsed);
	const Depreciation &depreciation = ship.IsYours() ? player.FleetDepreciation() : player.StockDepreciation();
	UpdateOutfits(ship, player, depreciation);

	maximumHeight = max(descriptionHeight, max(attributesHeight, outfitsHeight));
}



int ShipInfoDisplay::GetAttributesHeight(bool sale) const
{
	return attributesHeight + (sale ? saleHeight : 0);
}



int ShipInfoDisplay::OutfitsHeight() const
{
	return outfitsHeight;
}



void ShipInfoDisplay::DrawAttributes(const Point &topLeft) const
{
	DrawAttributes(topLeft, false);
}



// Draw each of the panels.
void ShipInfoDisplay::DrawAttributes(const Point &topLeft, const bool sale) const
{
	// Header.
	Point point = Draw(topLeft, attributeHeaderLabels, attributeHeaderValues);

	// Sale info.
	if(sale)
	{
		point = Draw(point, saleLabels, saleValues);

		const Color &color = *GameData::Colors().Get("medium");
		FillShader::Fill(point + Point(.5 * WIDTH, 5.), Point(WIDTH - 20., 1.), color);
	}
	else
		point -= Point(0, 10.);

	// Body.
	point = Draw(point, attributeLabels, attributeValues);

	// Get standard colors to draw with.
	const Color &labelColor = *GameData::Colors().Get("medium");
	const Color &valueColor = *GameData::Colors().Get("bright");

	Table table;
	table.AddColumn(10, {WIDTH - 10, Alignment::LEFT});
	table.AddColumn(WIDTH - 90, {WIDTH - 80, Alignment::RIGHT});
	table.AddColumn(WIDTH - 10, {WIDTH - 20, Alignment::RIGHT});
	table.SetHighlight(0, WIDTH);
	table.DrawAt(point);
	table.DrawGap(10.);

	table.Advance();
	table.Draw("energy", labelColor);
	table.Draw("heat", labelColor);

	for(unsigned i = 0; i < tableLabels.size(); ++i)
	{
		CheckHover(table, tableLabels[i]);
		table.Draw(tableLabels[i], labelColor);
		table.Draw(energyTable[i], valueColor);
		table.Draw(heatTable[i], valueColor);
	}
}



void ShipInfoDisplay::DrawOutfits(const Point &topLeft) const
{
	Draw(topLeft, outfitLabels, outfitValues);
}



void ShipInfoDisplay::UpdateAttributes(const Ship &ship, const PlayerInfo &player, bool descriptionCollapsed)
{
	bool isGeneric = ship.Name().empty() || ship.GetPlanet();

	attributeHeaderLabels.clear();
	attributeHeaderValues.clear();

	attributeHeaderLabels.push_back("model:");
	attributeHeaderValues.push_back(ship.ModelName());
	attributesHeight = 20;

	attributeLabels.clear();
	attributeValues.clear();
	attributesHeight += 20;

	const Outfit &attributes = ship.Attributes();

	if(!ship.IsYours())
		for(const string &license : attributes.Licenses())
		{
			if(player.HasLicense(license))
				continue;

			const auto &licenseOutfit = GameData::Outfits().Find(license + " License");
			if(descriptionCollapsed || (licenseOutfit && licenseOutfit->Cost()))
			{
				attributeLabels.push_back("license:");
				attributeValues.push_back(license);
				attributesHeight += 20;
			}
		}

	int64_t fullCost = ship.Cost();
	const Depreciation &depreciation = ship.IsYours() ? player.FleetDepreciation() : player.StockDepreciation();
	int64_t depreciated = depreciation.Value(ship, player.GetDate().DaysSinceEpoch());
	if(depreciated == fullCost)
		attributeLabels.push_back("cost:");
	else
	{
		ostringstream out;
		out << "cost (" << (100 * depreciated) / fullCost << "%):";
		attributeLabels.push_back(out.str());
	}
	attributeValues.push_back(Format::Credits(depreciated));
	attributesHeight += 20;

	attributeLabels.push_back(string());
	attributeValues.push_back(string());
	attributesHeight += 10;
	double shieldRegen = attributes.Get("shield generation")
		* (1. + attributes.Get("shield generation multiplier"));
	bool hasShieldRegen = shieldRegen > 0.;
	if(hasShieldRegen)
	{
		attributeLabels.push_back("shields (charge):");
		attributeValues.push_back(Format::Number(attributes.Get("shields"))
			+ " (" + Format::Number(60. * shieldRegen) + "/s)");
	}
	else
	{
		attributeLabels.push_back("shields:");
		attributeValues.push_back(Format::Number(attributes.Get("shields")));
	}
	attributesHeight += 20;
	double hullRepair = attributes.Get("hull repair rate")
		* (1. + attributes.Get("hull repair multiplier"));
	bool hasHullRepair = hullRepair > 0.;
	if(hasHullRepair)
	{
		attributeLabels.push_back("hull (repair):");
		attributeValues.push_back(Format::Number(attributes.Get("hull"))
			+ " (" + Format::Number(60. * hullRepair) + "/s)");
	}
	else
	{
		attributeLabels.push_back("hull:");
		attributeValues.push_back(Format::Number(attributes.Get("hull")));
	}
	attributesHeight += 20;
	double emptyMass = attributes.Mass();
	double currentMass = ship.Mass();
	attributeLabels.push_back(isGeneric ? "mass with no cargo:" : "mass:");
	attributeValues.push_back(Format::Number(isGeneric ? emptyMass : currentMass) + " tons");
	attributesHeight += 20;
	attributeLabels.push_back(isGeneric ? "cargo space:" : "cargo:");
	if(isGeneric)
		attributeValues.push_back(Format::Number(attributes.Get("cargo space")) + " tons");
	else
		attributeValues.push_back(Format::Number(ship.Cargo().Used())
			+ " / " + Format::Number(attributes.Get("cargo space")) + " tons");
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

	double fullMass = emptyMass + attributes.Get("cargo space");
	isGeneric &= (fullMass != emptyMass);
	double forwardThrust = attributes.Get("thrust") ? attributes.Get("thrust") : attributes.Get("afterburner thrust");
	attributeLabels.push_back(string());
	attributeValues.push_back(string());
	attributesHeight += 10;
	attributeLabels.push_back(isGeneric ? "movement (full - no cargo):" : "movement:");
	attributeValues.push_back(string());
	attributesHeight += 20;
	attributeLabels.push_back("max speed:");
	attributeValues.push_back(Format::Number(60. * forwardThrust / ship.Drag()));
	attributesHeight += 20;

	// Movement stats are influenced by inertia redeuction.
	double reduction = 1. + attributes.Get("inertia reduction");
	emptyMass /= reduction;
	currentMass /= reduction;
	fullMass /= reduction;
	attributeLabels.push_back("acceleration:");
	if(!isGeneric)
		attributeValues.push_back(Format::Number(3600. * forwardThrust / currentMass));
	else
		attributeValues.push_back(Format::Number(3600. * forwardThrust / fullMass)
			+ " - " + Format::Number(3600. * forwardThrust / emptyMass));
	attributesHeight += 20;

	attributeLabels.push_back("turning:");
	if(!isGeneric)
		attributeValues.push_back(Format::Number(60. * attributes.Get("turn") / currentMass));
	else
		attributeValues.push_back(Format::Number(60. * attributes.Get("turn") / fullMass)
			+ " - " + Format::Number(60. * attributes.Get("turn") / emptyMass));
	attributesHeight += 20;

	// Find out how much outfit, engine, and weapon space the chassis has.
	map<string, double> chassis;
	static const vector<string> NAMES = {
		"outfit space free:", "outfit space",
		"    weapon capacity:", "weapon capacity",
		"    engine capacity:", "engine capacity",
		"gun ports free:", "gun ports",
		"turret mounts free:", "turret mounts"
	};
	for(unsigned i = 1; i < NAMES.size(); i += 2)
		chassis[NAMES[i]] = attributes.Get(NAMES[i]);
	for(const auto &it : ship.Outfits())
		for(auto &cit : chassis)
			cit.second -= min(0., it.second * it.first->Get(cit.first));

	attributeLabels.push_back(string());
	attributeValues.push_back(string());
	attributesHeight += 10;
	for(unsigned i = 0; i < NAMES.size(); i += 2)
	{
		attributeLabels.push_back(NAMES[i]);
		attributeValues.push_back(Format::Number(attributes.Get(NAMES[i + 1]))
			+ " / " + Format::Number(chassis[NAMES[i + 1]]));
		attributesHeight += 20;
	}

	// Print the number of bays for each bay-type we have
	for(const auto &category : GameData::GetCategory(CategoryType::BAY))
	{
		const string &bayType = category.Name();
		int totalBays = ship.BaysTotal(bayType);
		if(totalBays)
		{
			// make sure the label is printed in lower case
			string bayLabel = bayType;
			transform(bayLabel.begin(), bayLabel.end(), bayLabel.begin(), ::tolower);

			attributeLabels.emplace_back(bayLabel + " bays:");
			attributeValues.emplace_back(to_string(totalBays));
			attributesHeight += 20;
		}
	}

	tableLabels.clear();
	energyTable.clear();
	heatTable.clear();
	// Skip a spacer and the table header.
	attributesHeight += 30;

	const double idleEnergyPerFrame = attributes.Get("energy generation")
		+ attributes.Get("solar collection")
		+ attributes.Get("fuel energy")
		- attributes.Get("energy consumption")
		- attributes.Get("cooling energy");
	const double idleHeatPerFrame = attributes.Get("heat generation")
		+ attributes.Get("solar heat")
		+ attributes.Get("fuel heat")
		- ship.CoolingEfficiency() * (attributes.Get("cooling") + attributes.Get("active cooling"));
	tableLabels.push_back("idle:");
	energyTable.push_back(Format::Number(60. * idleEnergyPerFrame));
	heatTable.push_back(Format::Number(60. * idleHeatPerFrame));

	attributesHeight += 20;
	const double movingEnergyPerFrame =
		max(attributes.Get("thrusting energy"), attributes.Get("reverse thrusting energy"))
		+ attributes.Get("turning energy")
		+ attributes.Get("afterburner energy");
	const double movingHeatPerFrame = max(attributes.Get("thrusting heat"), attributes.Get("reverse thrusting heat"))
		+ attributes.Get("turning heat")
		+ attributes.Get("afterburner heat");
	tableLabels.push_back("moving:");
	energyTable.push_back(Format::Number(-60. * movingEnergyPerFrame));
	heatTable.push_back(Format::Number(60. * movingHeatPerFrame));

	attributesHeight += 20;
	double firingEnergy = 0.;
	double firingHeat = 0.;
	for(const auto &it : ship.Outfits())
		if(it.first->IsWeapon() && it.first->Reload())
		{
			firingEnergy += it.second * it.first->FiringEnergy() / it.first->Reload();
			firingHeat += it.second * it.first->FiringHeat() / it.first->Reload();
		}
	tableLabels.push_back("firing:");
	energyTable.push_back(Format::Number(-60. * firingEnergy));
	heatTable.push_back(Format::Number(60. * firingHeat));

	attributesHeight += 20;
	double shieldEnergy = (hasShieldRegen) ? attributes.Get("shield energy")
		* (1. + attributes.Get("shield energy multiplier")) : 0.;
	double hullEnergy = (hasHullRepair) ? attributes.Get("hull energy")
		* (1. + attributes.Get("hull energy multiplier")) : 0.;
	tableLabels.push_back((shieldEnergy && hullEnergy) ? "shields / hull:" :
		hullEnergy ? "repairing hull:" : "charging shields:");
	energyTable.push_back(Format::Number(-60. * (shieldEnergy + hullEnergy)));
	double shieldHeat = (hasShieldRegen) ? attributes.Get("shield heat")
		* (1. + attributes.Get("shield heat multiplier")) : 0.;
	double hullHeat = (hasHullRepair) ? attributes.Get("hull heat")
		* (1. + attributes.Get("hull heat multiplier")) : 0.;
	heatTable.push_back(Format::Number(60. * (shieldHeat + hullHeat)));

	attributesHeight += 20;
	const double maxEnergy = attributes.Get("energy capacity");
	const double maxHeat = 60. * ship.HeatDissipation() * ship.MaximumHeat();
	tableLabels.push_back("max:");
	energyTable.push_back(Format::Number(maxEnergy));
	heatTable.push_back(Format::Number(maxHeat));
	// Pad by 10 pixels on the top and bottom.
	attributesHeight += 30;
}



void ShipInfoDisplay::UpdateOutfits(const Ship &ship, const PlayerInfo &player, const Depreciation &depreciation)
{
	outfitLabels.clear();
	outfitValues.clear();
	outfitsHeight = 20;

	map<string, map<string, int>> listing;
	for(const auto &it : ship.Outfits())
		listing[it.first->Category()][it.first->DisplayName()] += it.second;

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


	int64_t totalCost = depreciation.Value(ship, player.GetDate().DaysSinceEpoch());
	int64_t chassisCost = depreciation.Value(
		GameData::Ships().Get(ship.ModelName()),
		player.GetDate().DaysSinceEpoch());
	saleLabels.clear();
	saleValues.clear();
	saleHeight = 20;

	saleLabels.push_back("This ship will sell for:");
	saleValues.push_back(string());
	saleHeight += 20;
	saleLabels.push_back("empty hull:");
	saleValues.push_back(Format::Credits(chassisCost));
	saleHeight += 20;
	saleLabels.push_back("  + outfits:");
	saleValues.push_back(Format::Credits(totalCost - chassisCost));
	saleHeight += 5;
}
