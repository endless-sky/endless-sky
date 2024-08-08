/* OutfitInfoDisplay.h
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

#pragma once

#include "ItemInfoDisplay.h"

#include <string>
#include <vector>

class Outfit;
class PlayerInfo;
class Point;



// Class representing three panels of information about a given outfit. One
// shows the outfit's description, one shows the required space and cost to
// install it, and one shows other attributes of the outfit.
class OutfitInfoDisplay : public ItemInfoDisplay {
public:
	OutfitInfoDisplay() = default;
	OutfitInfoDisplay(const Outfit &outfit, const PlayerInfo &player,
			bool canSell = false, bool descriptionCollapsed = true);

	// Call this every time the ship changes.
	void Update(const Outfit &outfit, const PlayerInfo &player, bool canSell = false, bool descriptionCollapsed = true);

	// Provided by ItemInfoDisplay:
	// int PanelWidth();
	virtual int AttributesHeight() const override;

	// Provided by ItemInfoDisplay:
	// Point DrawDescription(const Point &topLeft) const;
	virtual Point DrawAttributes(const Point &topLeft) const override;
	Point DrawRequirements(const Point &topLeft) const;


private:
	void UpdateRequirements(const Outfit &outfit, const PlayerInfo &player, bool canSell, bool descriptionCollapsed);
	void UpdateAttributes(const Outfit &outfit);


private:
	static void AddRequirementAttribute(const std::string &label, double value, std::vector<std::string> &labels,
			std::vector<std::string> &values);


private:
	mutable FlexTable requirements{WIDTH, 2};
};
