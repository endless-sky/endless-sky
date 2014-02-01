/* ShipInfoDisplay.h
Michael Zahniser, 24 Jan 2014

Class representing three panels of information about a given ship. One shows the
ship's description, the second summarizes its attributes, and the third lists
all outfits currently installed in the ship. This is used for the shipyard, for
showing changes to your ship as you add upgrades, for scanning other ships, etc.
*/

#ifndef SHIP_INFO_DISPLAY_H_INCLUDED
#define SHIP_INFO_DISPLAY_H_INCLUDED

#include "WrappedText.h"

class Point;
class Ship;

#include <vector>
#include <string>



class ShipInfoDisplay {
public:
	ShipInfoDisplay();
	ShipInfoDisplay(const Ship &ship);
	
	// Call this every time the ship changes.
	void Update(const Ship &ship);
	
	// Get the panel width.
	int PanelWidth() const;
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
	void UpdateDescription(const Ship &ship);
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
