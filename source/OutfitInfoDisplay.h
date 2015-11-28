/* OutfitInfoDisplay.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef OUTFIT_INFO_DISPLAY_H_
#define OUTFIT_INFO_DISPLAY_H_

#include "WrappedText.h"

#include <vector>
#include <string>

class Point;
class Outfit;



// Class representing three panels of information about a given outfit. One
// shows the outfit's description, one shows the required space and cost to
// install it, and one shows other attributes of the outfit.
class OutfitInfoDisplay {
public:
	OutfitInfoDisplay();
	OutfitInfoDisplay(const Outfit &outfit);
	OutfitInfoDisplay(const Outfit &outfit, const Outfit &stcOutfit);
	
	// Call this every time the ship changes.
	void Update(const Outfit &outfit);
	
	// Get the panel width.
	static int PanelWidth();
	// Get the height of each of the three panels.
	int MaximumHeight() const;
	int DescriptionHeight() const;
	int RequirementsHeight() const;
	int AttributesHeight() const;
	
	// Draw each of the panels.
	void DrawDescription(const Point &topLeft) const;
	void DrawRequirements(const Point &topLeft) const;
	void DrawAttributes(const Point &topLeft, bool comparative) const;
	
	
private:
	void UpdateDescription(const Outfit &outfit);
	void UpdateRequirements(const Outfit &outfit);
	void UpdateAttributes(const Outfit &outfit);
	void UpdateBothAtt(const Outfit &lftOft, const Outfit &rgtOft);
	
	
private:
	WrappedText description;
	int descriptionHeight;
	
	std::vector<std::string> requirementLabels;
	std::vector<std::string> requirementValues;
	int requirementsHeight;
	
	//The new description that 'sticks'
	std::vector<std::string> stcAttributeLabels;
	std::vector<std::string> stcAttributeValues;
	
	std::vector<std::string> attributeLabels;
	std::vector<std::string> attributeValues;
	int attributesHeight;
	
	int maximumHeight;
	
};



#endif
