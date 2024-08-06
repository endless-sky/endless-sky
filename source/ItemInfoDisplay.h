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
#include "text/FlexTable.h"
#include "text/WrappedText.h"

#include <string>
#include <vector>



// Class representing three panels of information about a given item. One shows
// a text description, one shows the item's attributes, and a third may be
// different depending on what kind of item it is (a ship or an outfit).
class ItemInfoDisplay {
public:
	ItemInfoDisplay();
	virtual ~ItemInfoDisplay() = default;

	// Get the panel width.
	static int PanelWidth();
	virtual int AttributesHeight() const;

	bool HasDescription() const;

	// Draw each of the panels.
	Point DrawDescription(const Point &topLeft) const;
	virtual Point DrawAttributes(const Point &topLeft) const;
	void DrawTooltips() const;

	// Update the location where the mouse is hovering.
	void Hover(const Point &point);
	void ClearHover();

	static FlexTable CreateTable(const std::vector<std::string> &labels, const std::vector<std::string> &values);


protected:
	void UpdateDescription(const std::string &text, const std::vector<std::string> &licenses, bool isShip);
	Point Draw(FlexTable &table, Point drawPoint, int labelIndex = 0) const;
	void CheckHover(FlexTable &table, Point drawPoint, int labelIndex = 0) const;


protected:
	static const int WIDTH = 250;

	WrappedText description;

	mutable FlexTable attributes{WIDTH - 20, 2};

	// For tooltips:
	Point hoverPoint;
	mutable std::string hover;
	mutable int hoverCount = 0;
	bool hasHover = false;
	mutable WrappedText hoverText;
};
