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

#include "text/Alignment.h"
#include "CategoryList.h"
#include "CategoryType.h"
#include "Depreciation.h"
#include "shader/FillShader.h"
#include "text/Format.h"
#include "GameData.h"
#include "Outfit.h"
#include "PlayerInfo.h"
#include "Ship.h"
#include "text/Table.h"
#include "Weapon.h"

#include <algorithm>
#include <map>
#include <sstream>

using namespace std;



// Call this every time the ship changes.
// Panels that have scrolling abilities are not limited by space, allowing more detailed attributes.
void ShipInfoDisplay::Update(const Ship &ship, const PlayerInfo &player, bool hasFleetCapacity,
	bool descriptionCollapsed, bool scrollingPanel)
{
	UpdateDescription(ship.Description(), ship.Attributes().Licenses(), true);
	UpdateAttributes(ship, player, hasFleetCapacity, descriptionCollapsed, scrollingPanel);
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
void ShipInfoDisplay::DrawAttributes(const Point &topLeft, bool sale) const
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



void ShipInfoDisplay::UpdateAttributes(const Ship &ship, const PlayerInfo &player, bool hasFleetCapacity,
	bool descriptionCollapsed, bool scrollingPanel)
{
	bool isGeneric = ship.GivenName().empty() || ship.GetPlanet();

	attributeHeaderLabels.clear();
	attributeHeaderValues.clear();

	attributeHeaderLabels.emplace_back("model:");
	attributeHeaderValues.emplace_back(ship.DisplayModelName());

	attributesHeight = 20;

	// Only show the ship category on scrolling panels with no risk of overflow.
	if(scrollingPanel)
	{
		attributeHeaderLabels.emplace_back("category:");
		const string &category = ship.BaseAttributes().Category();
		attributeHeaderValues.emplace_back(category.empty() ? "???" : category);
		attributesHeight += 20;
	}

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
				attributeLabels.emplace_back("license:");
				attributeValues.emplace_back(license);
				attributesHeight += 20;
			}
		}

	int64_t fullCost = ship.Cost();
	const Depreciation &depreciation = ship.IsYours() ? player.FleetDepreciation() : player.StockDepreciation();
	int64_t depreciated = depreciation.Value(ship, player.GetDate().DaysSinceEpoch());
	if(depreciated == fullCost)
		attributeLabels.emplace_back("cost:");
	else
	{
		ostringstream out;
		out << "cost (" << (100 * depreciated) / fullCost << "%):";
		attributeLabels.emplace_back(out.str());
	}
	attributeValues.emplace_back(Format::AbbreviatedNumber(depreciated));
	attributesHeight += 20;

	if(hasFleetCapacity)
	{
		attributeLabels.emplace_back("fleet cost:");
		attributeValues.emplace_back(Format::Number(ship.FleetCost()));
		attributesHeight += 20;
	}

	attributeLabels.emplace_back();
	attributeValues.emplace_back();
	attributesHeight += 10;
	double shieldRegen = (attributes.Get("shield generation")
		+ attributes.Get("delayed shield generation"))
		* (1. + attributes.Get("shield generation multiplier"));
	bool hasShieldRegen = shieldRegen > 0.;
	if(hasShieldRegen)
	{
		attributeLabels.emplace_back("shields (charge):");
		attributeValues.emplace_back(Format::Number(ship.MaxShields())
			+ " (" + Format::Number(60. * shieldRegen) + "/s)");
	}
	else
	{
		attributeLabels.emplace_back("shields:");
		attributeValues.emplace_back(Format::Number(ship.MaxShields()));
	}
	attributesHeight += 20;
	double hullRepair = (attributes.Get("hull repair rate")
		+ attributes.Get("delayed hull repair rate"))
		* (1. + attributes.Get("hull repair multiplier"));
	bool hasHullRepair = hullRepair > 0.;
	if(hasHullRepair)
	{
		attributeLabels.emplace_back("hull (repair):");
		attributeValues.emplace_back(Format::Number(ship.MaxHull())
			+ " (" + Format::Number(60. * hullRepair) + "/s)");
	}
	else
	{
		attributeLabels.emplace_back("hull:");
		attributeValues.emplace_back(Format::Number(ship.MaxHull()));
	}
	attributesHeight += 20;
	if(scrollingPanel)
	{
		attributeLabels.emplace_back("disabled at:");
		attributeValues.emplace_back(Format::Number(ship.MinHull()));
		attributesHeight += 20;
	}
	double emptyMass = attributes.Mass();
	double currentMass = ship.Mass();
	attributeLabels.emplace_back(isGeneric ? "mass with no cargo:" : "mass:");
	attributeValues.emplace_back(Format::Number(isGeneric ? emptyMass : currentMass) + " tons");
	attributesHeight += 20;
	attributeLabels.emplace_back(isGeneric ? "cargo space:" : "cargo:");
	if(isGeneric)
		attributeValues.emplace_back(Format::Number(attributes.Get("cargo space")) + " tons");
	else
		attributeValues.emplace_back(Format::Number(ship.Cargo().Used())
			+ " / " + Format::Number(attributes.Get("cargo space")) + " tons");
	attributesHeight += 20;
	attributeLabels.emplace_back("required crew / bunks:");
	attributeValues.emplace_back(Format::Number(ship.RequiredCrew())
		+ " / " + Format::Number(attributes.Get("bunks")));
	attributesHeight += 20;
	attributeLabels.emplace_back(isGeneric ? "fuel capacity:" : "fuel:");
	double fuelCapacity = ship.MaxFuel();
	if(isGeneric)
		attributeValues.emplace_back(Format::Number(fuelCapacity));
	else
		attributeValues.emplace_back(Format::Number(ship.FuelLevel()) + " / " + Format::Number(fuelCapacity));
	attributesHeight += 20;

	double fullMass = emptyMass + attributes.Get("cargo space");
	isGeneric &= (fullMass != emptyMass);
	double thrust = attributes.Get("thrust");
	double afterburnerThrust = attributes.Get("afterburner thrust");
	double mainThrust = thrust ? thrust : afterburnerThrust;
	double allThrust = thrust + afterburnerThrust;
	double reverseThrust = attributes.Get("reverse thrust");
	double drag = ship.Drag();
	attributeLabels.emplace_back();
	attributeValues.emplace_back();
	attributesHeight += 10;
	attributeLabels.emplace_back(isGeneric ? "movement (full - no cargo):" : "movement:");
	attributeValues.emplace_back();
	attributesHeight += 20;
	attributeLabels.emplace_back("max speed:");
	attributeValues.emplace_back(Format::Number(60. * mainThrust / drag));
	attributesHeight += 20;
	if(scrollingPanel)
	{
		if(thrust && afterburnerThrust)
		{
			attributeLabels.emplace_back("    w/ afterburner:");
			attributeValues.emplace_back(Format::Number(60. * allThrust / drag));
			attributesHeight += 20;
		}
		if(reverseThrust)
		{
			attributeLabels.emplace_back("    reversed:");
			attributeValues.emplace_back(Format::Number(60. * reverseThrust / drag));
			attributesHeight += 20;
		}
	}

	// Movement stats are influenced by inertia reduction.
	double reduction = 1. + attributes.Get("inertia reduction");
	emptyMass /= reduction;
	currentMass /= reduction;
	fullMass /= reduction;
	double accelMult = 1. + attributes.Get("acceleration multiplier");

	auto MassInfluencedMovementDisplay = [&](double value) -> void {
		if(!isGeneric)
			attributeValues.emplace_back(Format::Number(value / currentMass));
		else
			attributeValues.emplace_back(Format::Number(value / fullMass) + " - " + Format::Number(value / emptyMass));
		attributesHeight += 20;
	};
	auto AccelerationDisplay = [&](double thrustUsed) -> void {
		MassInfluencedMovementDisplay(3600. * thrustUsed * accelMult);
	};

	attributeLabels.emplace_back("acceleration:");
	AccelerationDisplay(mainThrust);
	if(scrollingPanel)
	{
		if(thrust && afterburnerThrust)
		{
			attributeLabels.emplace_back("    w/ afterburner:");
			AccelerationDisplay(allThrust);
		}
		if(reverseThrust)
		{
			attributeLabels.emplace_back("    reversed:");
			AccelerationDisplay(reverseThrust);
		}
	}

	attributeLabels.emplace_back("turning:");
	double baseTurn = 60. * attributes.Get("turn") * (1. + attributes.Get("turn multiplier"));
	MassInfluencedMovementDisplay(baseTurn);

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

	attributeLabels.emplace_back();
	attributeValues.emplace_back();
	attributesHeight += 10;
	for(unsigned i = 0; i < NAMES.size(); i += 2)
	{
		attributeLabels.emplace_back(NAMES[i]);
		attributeValues.emplace_back(Format::Number(attributes.Get(NAMES[i + 1]))
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
			ranges::transform(bayLabel, bayLabel.begin(), ::tolower);

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
	tableLabels.emplace_back("idle:");
	energyTable.emplace_back(Format::Number(60. * idleEnergyPerFrame));
	heatTable.emplace_back(Format::Number(60. * idleHeatPerFrame));

	// Add energy and heat while moving to the table.
	attributesHeight += 20;
	double movingEnergyPerFrame = attributes.Get("turning energy");
	double movingHeatPerFrame = attributes.Get("turning heat");
	if(mainThrust && reverseThrust)
	{
		movingEnergyPerFrame += max(attributes.Get("thrusting energy") + attributes.Get("afterburner energy"),
			attributes.Get("reverse thrusting energy"));
		movingHeatPerFrame += max(attributes.Get("thrusting heat") + attributes.Get("afterburner heat"),
			attributes.Get("reverse thrusting heat"));
	}
	else if(mainThrust)
	{
		movingEnergyPerFrame += attributes.Get("thrusting energy") + attributes.Get("afterburner energy");
		movingHeatPerFrame += attributes.Get("thrusting heat") + attributes.Get("afterburner heat");
	}
	else if(reverseThrust)
	{
		movingEnergyPerFrame += attributes.Get("reverse thrusting energy");
		movingHeatPerFrame += attributes.Get("reverse thrusting heat");
	}
	// If this is a scrolling panel, break up movement into each individual movement capability.
	if(scrollingPanel)
	{
		tableLabels.emplace_back("thrusting:");
		energyTable.emplace_back(Format::Number(-60. * attributes.Get("thrusting energy")));
		heatTable.emplace_back(Format::Number(60. * attributes.Get("thrusting heat")));
		if(reverseThrust)
		{
			attributesHeight += 20;
			tableLabels.emplace_back("reversing:");
			energyTable.emplace_back(Format::Number(-60. * attributes.Get("reverse thrusting energy")));
			heatTable.emplace_back(Format::Number(60. * attributes.Get("reverse thrusting heat")));
		}
		if(afterburnerThrust)
		{
			attributesHeight += 20;
			tableLabels.emplace_back("afterburner:");
			energyTable.emplace_back(Format::Number(-60. * attributes.Get("afterburner energy")));
			heatTable.emplace_back(Format::Number(60. * attributes.Get("afterburner heat")));
		}
		attributesHeight += 20;
		tableLabels.emplace_back("turning:");
		energyTable.emplace_back(Format::Number(-60. * attributes.Get("turning energy")));
		heatTable.emplace_back(Format::Number(60. * attributes.Get("turning heat")));
	}
	else
	{
		// Otherwise, just display a singular worst-case value.
		tableLabels.emplace_back("moving:");
		energyTable.emplace_back(Format::Number(-60. * movingEnergyPerFrame));
		heatTable.emplace_back(Format::Number(60. * movingHeatPerFrame));
	}

	// Add energy and heat while firing to the table.
	attributesHeight += 20;
	double firingEnergy = 0.;
	double firingHeat = 0.;
	for(const auto &it : ship.Outfits())
	{
		const Weapon *weapon = it.first->GetWeapon().get();
		if(weapon && weapon->Reload())
		{
			firingEnergy += it.second * weapon->FiringEnergy() / weapon->Reload();
			firingHeat += it.second * weapon->FiringHeat() / weapon->Reload();
		}
	}
	tableLabels.emplace_back("firing:");
	energyTable.emplace_back(Format::Number(-60. * firingEnergy));
	heatTable.emplace_back(Format::Number(60. * firingHeat));

	// Add energy and heat when doing shield and hull repair to the table.
	attributesHeight += 20;
	double shieldEnergy = (hasShieldRegen) ? (attributes.Get("shield energy")
		+ attributes.Get("delayed shield energy"))
		* (1. + attributes.Get("shield energy multiplier")) : 0.;
	double hullEnergy = (hasHullRepair) ? (attributes.Get("hull energy")
		+ attributes.Get("delayed hull energy"))
		* (1. + attributes.Get("hull energy multiplier")) : 0.;
	tableLabels.emplace_back((shieldEnergy && hullEnergy) ? "shields / hull:" :
		hullEnergy ? "repairing hull:" : "charging shields:");
	energyTable.emplace_back(Format::Number(-60. * (shieldEnergy + hullEnergy)));
	double shieldHeat = (hasShieldRegen) ? (attributes.Get("shield heat")
		+ attributes.Get("delayed shield heat"))
		* (1. + attributes.Get("shield heat multiplier")) : 0.;
	double hullHeat = (hasHullRepair) ? (attributes.Get("hull heat")
		+ attributes.Get("delayed hull heat"))
		* (1. + attributes.Get("hull heat multiplier")) : 0.;
	heatTable.emplace_back(Format::Number(60. * (shieldHeat + hullHeat)));

	if(scrollingPanel)
	{
		// Add up the maximum possible changes and add the total to the table.
		attributesHeight += 20;
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
		tableLabels.emplace_back("net change:");
		energyTable.emplace_back(Format::Number(60. * overallEnergy));
		heatTable.emplace_back(Format::Number(60. * overallHeat));
	}

	// Add maximum values of energy and heat to the table.
	attributesHeight += 20;
	const double maxEnergy = attributes.Get("energy capacity");
	const double maxHeat = 60. * ship.HeatDissipation() * ship.MaxHeat();
	tableLabels.emplace_back("capacity:");
	energyTable.emplace_back(Format::Number(maxEnergy));
	heatTable.emplace_back(Format::Number(maxHeat));

	if(scrollingPanel && !maxEnergy)
	{
		attributesHeight += 20;
		tableLabels.emplace_back("surge:");
		energyTable.emplace_back(Format::Number(max(0., idleEnergyPerFrame)));
		heatTable.emplace_back("N/A");
	}
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
		if(it.first->IsDefined() && !it.first->Category().empty() && !it.first->DisplayName().empty())
			listing[it.first->Category()][it.first->DisplayName()] += it.second;

	for(const auto &cit : listing)
	{
		// Pad by 10 pixels before each category.
		if(&cit != &*listing.begin())
		{
			outfitLabels.emplace_back();
			outfitValues.emplace_back();
			outfitsHeight += 10;
		}

		outfitLabels.emplace_back(cit.first + ':');
		outfitValues.emplace_back();
		outfitsHeight += 20;

		for(const auto &it : cit.second)
		{
			outfitLabels.emplace_back(it.first);
			outfitValues.emplace_back(to_string(it.second));
			outfitsHeight += 20;
		}
	}


	int64_t totalCost = depreciation.Value(ship, player.GetDate().DaysSinceEpoch());
	int64_t chassisCost = depreciation.Value(
		GameData::Ships().Get(ship.TrueModelName()),
		player.GetDate().DaysSinceEpoch());
	saleLabels.clear();
	saleValues.clear();
	saleHeight = 20;

	saleLabels.emplace_back("This ship will sell for:");
	saleValues.emplace_back();
	saleHeight += 20;
	saleLabels.emplace_back("empty hull:");
	saleValues.emplace_back(Format::AbbreviatedNumber(chassisCost));
	saleHeight += 20;
	saleLabels.emplace_back("  + outfits:");
	saleValues.emplace_back(Format::AbbreviatedNumber(totalCost - chassisCost));
	saleHeight += 20;
}
