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

#include "WrappedText.h"

#include <vector>
#include <string>

class Government;
class Point;
class Ship;



// Class representing three panels of information about a given ship. One shows the
// ship's description, the second summarizes its attributes, and the third lists
// all outfits currently installed in the ship. This is used for the shipyard, for
// showing changes to your ship as you add upgrades, for scanning other ships, etc.
class ShipInfoDisplay {
public:
	ShipInfoDisplay();
	ShipInfoDisplay(const Ship &ship, const Government *systemGovernment = nullptr);
	
	// Call this every time the ship changes.
	void Update(const Ship &ship, const Government *systemGovernment = nullptr);
	
	// Get the panel width.
	static int PanelWidth();
	// Get the height of each of the three panels.
	int MaximumHeight() const;
	int DescriptionHeight() const;
	int AttributesHeight() const;
	int OutfitsHeight() const;
	int SaleHeight() const;
	
	// Draw each of the panels.
	void DrawDescription(const Point &topLeft) const;
	void DrawAttributes(const Point &topLeft) const;
	void DrawOutfits(const Point &topLeft) const;
	void DrawSale(const Point &topLeft) const;
	
	
private:
	void UpdateDescription(const Ship &ship, const Government *systemGovernment);
	void UpdateAttributes(const Ship &ship);
	void UpdateOutfits(const Ship &ship);
	
	
private:
	WrappedText description;
	int descriptionHeight;
	
	std::vector<std::string> attributeLabels;
	std::vector<std::string> attributeValues;
	int attributesHeight;
	
	std::vector<std::string> tableLabels;
	std::vector<std::string> energyTable;
	std::vector<std::string> heatTable;
	
	std::vector<std::string> outfitLabels;
	std::vector<std::string> outfitValues;
	int outfitsHeight;
	
	std::vector<std::string> saleLabels;
	std::vector<std::string> saleValues;
	int saleHeight;
	
	int maximumHeight;
};



#endif
