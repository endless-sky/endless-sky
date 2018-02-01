/* ShipInfoDisplay.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef SHIP_INFO_DISPLAY_H_
#define SHIP_INFO_DISPLAY_H_

#include "ItemInfoDisplay.h"

#include <string>
#include <vector>

class Depreciation;
class Point;
class Ship;



// Class representing three panels of information about a given ship. One shows the
// ship's description, the second summarizes its attributes, and the third lists
// all outfits currently installed in the ship. This is used for the shipyard, for
// showing changes to your ship as you add upgrades, for scanning other ships, etc.
class ShipInfoDisplay : public ItemInfoDisplay {
public:
	ShipInfoDisplay() = default;
	ShipInfoDisplay(const Ship &ship, const Depreciation &depreciation, int day);
	
	// Call this every time the ship changes.
	void Update(const Ship &ship, const Depreciation &depreciation, int day);
	
	// Provided by ItemInfoDisplay:
	// int PanelWidth();
	// int MaximumHeight() const;
	// int DescriptionHeight() const;
	// int AttributesHeight() const;
	int OutfitsHeight() const;
	int SaleHeight() const;
	
	// Provided by ItemInfoDisplay:
	// void DrawDescription(const Point &topLeft) const;
	virtual void DrawAttributes(const Point &topLeft) const override;
	void DrawOutfits(const Point &topLeft) const;
	void DrawSale(const Point &topLeft) const;
	
	
private:
	void UpdateAttributes(const Ship &ship, const Depreciation &depreciation, int day);
	void UpdateOutfits(const Ship &ship, const Depreciation &depreciation, int day);
	
	
private:
	std::vector<std::string> tableLabels;
	std::vector<std::string> energyTable;
	std::vector<std::string> heatTable;
	
	std::vector<std::string> outfitLabels;
	std::vector<std::string> outfitValues;
	int outfitsHeight = 0;
	
	std::vector<std::string> saleLabels;
	std::vector<std::string> saleValues;
	int saleHeight = 0;
};



#endif
