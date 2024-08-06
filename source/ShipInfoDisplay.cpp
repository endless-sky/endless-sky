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
// Panels that have scrolling abilities are not limited by space, allowing more detailed attributes.
void ShipInfoDisplay::Update(const Ship &ship, const PlayerInfo &player, bool descriptionCollapsed, bool scrollingPanel)
{
	UpdateDescription(ship.Description(), ship.Attributes().Licenses(), true);
	UpdateAttributes(ship, player, descriptionCollapsed, scrollingPanel);
	const Depreciation &depreciation = ship.IsYours() ? player.FleetDepreciation() : player.StockDepreciation();
	UpdateOutfits(ship, player, depreciation);
}



int ShipInfoDisplay::AttributesHeight() const
{
	return AttributesHeight(false);
}



int ShipInfoDisplay::AttributesHeight(bool sale) const
{
	// There are two places with 10-pixel paddings.
	return 20 + attributeHeader.Height() + (sale * sales.Height()) + attributes.Height() + energyHeatTable.Height();
}



Point ShipInfoDisplay::DrawAttributes(const Point &topLeft) const
{
	return DrawAttributes(topLeft, false);
}



// Draw each of the panels.
Point ShipInfoDisplay::DrawAttributes(const Point &topLeft, const bool sale) const
{
	// Header.
	const Point attributePosition = topLeft + Point{10., 10.};
	Point drawPoint = Draw(attributeHeader, attributePosition);

	// Sale info.
	if(sale)
		drawPoint = Draw(sales, drawPoint);

	// Body.
	drawPoint = Draw(attributes, drawPoint);
	drawPoint.Y() += 10;

	drawPoint = Draw(energyHeatTable, drawPoint);

	return drawPoint;
}



Point ShipInfoDisplay::DrawOutfits(const Point &topLeft) const
{
	return Draw(outfits, topLeft);
}



void ShipInfoDisplay::UpdateAttributes(const Ship &ship, const PlayerInfo &player, bool descriptionCollapsed,
		bool scrollingPanel)
{
	bool isGeneric = ship.Name().empty() || ship.GetPlanet();

	vector<string> attributeHeaderLabels, attributeHeaderValues;

	attributeHeaderLabels.push_back("model:");
	attributeHeaderValues.push_back(ship.DisplayModelName());

	// Only show the ship category on scrolling panels with no risk of overflow.
	if(scrollingPanel)
	{
		attributeHeaderLabels.push_back("category:");
		const string &category = ship.BaseAttributes().Category();
		attributeHeaderValues.push_back(category.empty() ? "???" : category);
	}
	attributeHeader = CreateTable(attributeHeaderLabels, attributeHeaderValues);

	vector<string> attributeLabels, attributeValues;

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

	attributeLabels.push_back(string());
	attributeValues.push_back(string());
	double shieldRegen = (attributes.Get("shield generation")
		+ attributes.Get("delayed shield generation"))
		* (1. + attributes.Get("shield generation multiplier"));
	bool hasShieldRegen = shieldRegen > 0.;
	if(hasShieldRegen)
	{
		attributeLabels.push_back("shields (charge):");
		attributeValues.push_back(Format::Number(ship.MaxShields())
			+ " (" + Format::Number(60. * shieldRegen) + "/s)");
	}
	else
	{
		attributeLabels.push_back("shields:");
		attributeValues.push_back(Format::Number(ship.MaxShields()));
	}
	double hullRepair = (attributes.Get("hull repair rate")
		+ attributes.Get("delayed hull repair rate"))
		* (1. + attributes.Get("hull repair multiplier"));
	bool hasHullRepair = hullRepair > 0.;
	if(hasHullRepair)
	{
		attributeLabels.push_back("hull (repair):");
		attributeValues.push_back(Format::Number(ship.MaxHull())
			+ " (" + Format::Number(60. * hullRepair) + "/s)");
	}
	else
	{
		attributeLabels.push_back("hull:");
		attributeValues.push_back(Format::Number(ship.MaxHull()));
	}
	double emptyMass = attributes.Mass();
	double currentMass = ship.Mass();
	attributeLabels.push_back(isGeneric ? "mass with no cargo:" : "mass:");
	attributeValues.push_back(Format::Number(isGeneric ? emptyMass : currentMass) + " tons");
	attributeLabels.push_back(isGeneric ? "cargo space:" : "cargo:");
	if(isGeneric)
		attributeValues.push_back(Format::Number(attributes.Get("cargo space")) + " tons");
	else
		attributeValues.push_back(Format::Number(ship.Cargo().Used())
			+ " / " + Format::Number(attributes.Get("cargo space")) + " tons");
	attributeLabels.push_back("required crew / bunks:");
	attributeValues.push_back(Format::Number(ship.RequiredCrew())
		+ " / " + Format::Number(attributes.Get("bunks")));
	attributeLabels.push_back(isGeneric ? "fuel capacity:" : "fuel:");
	double fuelCapacity = attributes.Get("fuel capacity");
	if(isGeneric)
		attributeValues.push_back(Format::Number(fuelCapacity));
	else
		attributeValues.push_back(Format::Number(ship.Fuel() * fuelCapacity)
			+ " / " + Format::Number(fuelCapacity));

	double fullMass = emptyMass + attributes.Get("cargo space");
	isGeneric &= (fullMass != emptyMass);
	double forwardThrust = attributes.Get("thrust") ? attributes.Get("thrust") : attributes.Get("afterburner thrust");
	attributeLabels.push_back(string());
	attributeValues.push_back(string());
	attributeLabels.push_back(isGeneric ? "movement (full - no cargo):" : "movement:");
	attributeValues.push_back(string());
	attributeLabels.push_back("max speed:");
	attributeValues.push_back(Format::Number(60. * forwardThrust / ship.Drag()));

	// Movement stats are influenced by inertia reduction.
	double reduction = 1. + attributes.Get("inertia reduction");
	emptyMass /= reduction;
	currentMass /= reduction;
	fullMass /= reduction;
	attributeLabels.push_back("acceleration:");
	double baseAccel = 3600. * forwardThrust * (1. + attributes.Get("acceleration multiplier"));
	if(!isGeneric)
		attributeValues.push_back(Format::Number(baseAccel / currentMass));
	else
		attributeValues.push_back(Format::Number(baseAccel / fullMass)
			+ " - " + Format::Number(baseAccel / emptyMass));

	attributeLabels.push_back("turning:");
	double baseTurn = 60. * attributes.Get("turn") * (1. + attributes.Get("turn multiplier"));
	if(!isGeneric)
		attributeValues.push_back(Format::Number(baseTurn / currentMass));
	else
		attributeValues.push_back(Format::Number(baseTurn / fullMass)
			+ " - " + Format::Number(baseTurn / emptyMass));

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
	for(unsigned i = 0; i < NAMES.size(); i += 2)
	{
		attributeLabels.push_back(NAMES[i]);
		attributeValues.push_back(Format::Number(attributes.Get(NAMES[i + 1]))
			+ " / " + Format::Number(chassis[NAMES[i + 1]]));
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
		}
	}
	ItemInfoDisplay::attributes = CreateTable(attributeLabels, attributeValues);

	ResetEnergyHeatTable();
	const Color &dim = *GameData::Colors().Get("medium");
	const Color &bright = *GameData::Colors().Get("bright");

	const double idleEnergyPerFrame = attributes.Get("energy generation")
		+ attributes.Get("solar collection")
		+ attributes.Get("fuel energy")
		- attributes.Get("energy consumption")
		- attributes.Get("cooling energy");
	const double idleHeatPerFrame = attributes.Get("heat generation")
		+ attributes.Get("solar heat")
		+ attributes.Get("fuel heat")
		- ship.CoolingEfficiency() * (attributes.Get("cooling") + attributes.Get("active cooling"));
	energyHeatTable.FillRow({{"idle:", dim}, {Format::Number(60. * idleEnergyPerFrame), bright},
			{Format::Number(60. * idleHeatPerFrame), bright}});

	// Add energy and heat while moving to the table.
	const double movingEnergyPerFrame =
		max(attributes.Get("thrusting energy"), attributes.Get("reverse thrusting energy"))
		+ attributes.Get("turning energy")
		+ attributes.Get("afterburner energy");
	const double movingHeatPerFrame = max(attributes.Get("thrusting heat"), attributes.Get("reverse thrusting heat"))
		+ attributes.Get("turning heat")
		+ attributes.Get("afterburner heat");
	energyHeatTable.FillRow({{"moving:", dim}, {Format::Number(-60. * movingEnergyPerFrame), bright},
			{Format::Number(60. * movingHeatPerFrame), bright}});

	// Add energy and heat while firing to the table.
	double firingEnergy = 0.;
	double firingHeat = 0.;
	for(const auto &it : ship.Outfits())
		if(it.first->IsWeapon() && it.first->Reload())
		{
			firingEnergy += it.second * it.first->FiringEnergy() / it.first->Reload();
			firingHeat += it.second * it.first->FiringHeat() / it.first->Reload();
		}
	energyHeatTable.FillRow({{"firing:", dim}, {Format::Number(-60. * firingEnergy), bright},
			{Format::Number(60. * firingHeat), bright}});

	// Add energy and heat when doing shield and hull repair to the table.
	double shieldEnergy = (hasShieldRegen) ? (attributes.Get("shield energy")
		+ attributes.Get("delayed shield energy"))
		* (1. + attributes.Get("shield energy multiplier")) : 0.;
	double hullEnergy = (hasHullRepair) ? (attributes.Get("hull energy")
		+ attributes.Get("delayed hull energy"))
		* (1. + attributes.Get("hull energy multiplier")) : 0.;
	double shieldHeat = (hasShieldRegen) ? (attributes.Get("shield heat")
		+ attributes.Get("delayed shield heat"))
		* (1. + attributes.Get("shield heat multiplier")) : 0.;
	double hullHeat = (hasHullRepair) ? (attributes.Get("hull heat")
		+ attributes.Get("delayed hull heat"))
		* (1. + attributes.Get("hull heat multiplier")) : 0.;
	auto shieldText = (shieldEnergy && hullEnergy) ? "shields / hull:" : hullEnergy ? "repairing hull:" : "charging shields:";
	energyHeatTable.FillRow({{shieldText, dim}, {Format::Number(-60. * (shieldEnergy + hullEnergy)), bright},
			{Format::Number(60. * (shieldHeat + hullHeat)), bright}});

	if(scrollingPanel)
	{
		// Add up the maximum possible changes and add the total to the table.
		const double overallEnergy = idleEnergyPerFrame
			- movingEnergyPerFrame
			- firingEnergy
			- shieldEnergy
			- hullEnergy;
		const double overallHeat = idleHeatPerFrame
			+ movingHeatPerFrame
			+ firingHeat
			+ shieldHeat
			+ hullHeat;
		energyHeatTable.FillRow({{"net change:", dim}, {Format::Number(60. * overallEnergy), bright},
				{Format::Number(60. * overallHeat), bright}});
	}

	// Add maximum values of energy and heat to the table.
	const double maxEnergy = attributes.Get("energy capacity");
	const double maxHeat = 60. * ship.HeatDissipation() * ship.MaximumHeat();
	energyHeatTable.FillRow({{"max:", dim}, {Format::Number(maxEnergy), bright}, {Format::Number(maxHeat), bright}});
}



void ShipInfoDisplay::UpdateOutfits(const Ship &ship, const PlayerInfo &player, const Depreciation &depreciation)
{
	vector<string> outfitLabels, outfitValues;

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
		}

		outfitLabels.push_back(cit.first + ':');
		outfitValues.push_back(string());

		for(const auto &it : cit.second)
		{
			outfitLabels.push_back(it.first);
			outfitValues.push_back(to_string(it.second));
		}
	}


	int64_t totalCost = depreciation.Value(ship, player.GetDate().DaysSinceEpoch());
	int64_t chassisCost = depreciation.Value(
		GameData::Ships().Get(ship.TrueModelName()),
		player.GetDate().DaysSinceEpoch());

	outfits = CreateTable(outfitLabels, outfitValues);

	vector<string> saleLabels, saleValues;

	saleLabels.push_back("This ship will sell for:");
	saleValues.push_back(string());
	saleLabels.push_back("empty hull:");
	saleValues.push_back(Format::Credits(chassisCost));
	saleLabels.push_back("  + outfits:");
	saleValues.push_back(Format::Credits(totalCost - chassisCost));

	sales = CreateTable(saleLabels, saleValues);
	const Color &color = *GameData::Colors().Get("medium");
	sales.GetCell(-1, 0).SetUnderlineColor(color);
	sales.GetCell(-1, 1).SetUnderlineColor(color);
}



void ShipInfoDisplay::ResetEnergyHeatTable()
{
	energyHeatTable = FlexTable{WIDTH - 20, 3};
	energyHeatTable.Clear();
	energyHeatTable.SetFlexStrategy(FlexTable::FlexStrategy::EVEN);
	energyHeatTable.GetColumn(1).SetAlignment(Alignment::RIGHT);
	energyHeatTable.GetColumn(2).SetAlignment(Alignment::RIGHT);

	energyHeatTable.FillRow({"", "energy", "heat"});
}
