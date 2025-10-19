/* ItemInfoDisplay.h
Copyright (c) 2016 by Michael Zahniser

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

#include "Point.h"
#include "Tooltip.h"
#include "text/WrappedText.h"

#include <string>
#include <vector>

class Table;



// Class representing three panels of information about a given item. One shows
// a text description, one shows the item's attributes, and a third may be
// different depending on what kind of item it is (a ship or an outfit).
class ItemInfoDisplay {
public:
	ItemInfoDisplay();
	virtual ~ItemInfoDisplay() = default;

	// Get the panel width.
	static int PanelWidth();
	// Get the height of each of the panels.
	int MaximumHeight() const;
	int DescriptionHeight() const;
	int AttributesHeight() const;

	// Draw each of the panels.
	void DrawDescription(const Point &topLeft) const;
	virtual void DrawAttributes(const Point &topLeft) const;
	void DrawTooltips() const;

	// Update the location where the mouse is hovering.
	void Hover(const Point &point);
	void ClearHover();


protected:
	void UpdateDescription(const std::string &text, const std::vector<std::string> &licenses, bool isShip);
	Point Draw(Point point, const std::vector<std::string> &labels, const std::vector<std::string> &values) const;
	void CheckHover(const Table &table, const std::string &label) const;


protected:
	static const int WIDTH = 250;

	WrappedText description;
	int descriptionHeight = 0;

	std::vector<std::string> attributeLabels;
	std::vector<std::string> attributeValues;
	int attributesHeight = 0;

	int maximumHeight = 0;

	// For tooltips:
	Point hoverPoint;
	mutable std::string hover;
	mutable Tooltip tooltip;
	bool hasHover = false;
};
