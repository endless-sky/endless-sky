/* ItemInfoDisplay.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "ItemInfoDisplay.h"

#include "text/alignment.hpp"
#include "Color.h"
#include "FillShader.h"
#include "text/FontSet.h"
#include "GameData.h"
#include "text/layout.hpp"
#include "Rectangle.h"
#include "Screen.h"
#include "text/Table.h"

#include <algorithm>
#include <cmath>

using namespace std;

namespace {
	const int HOVER_TIME = 60;
}

const Layout ItemInfoDisplay::commonLayout = Layout(WIDTH - 20, Alignment::JUSTIFIED);



// Get the panel width.
int ItemInfoDisplay::PanelWidth()
{
	return WIDTH;
}



// Get the height of each of the three panels.
int ItemInfoDisplay::MaximumHeight() const
{
	return maximumHeight;
}



int ItemInfoDisplay::DescriptionHeight() const
{
	return descriptionHeight;
}



int ItemInfoDisplay::AttributesHeight() const
{
	return attributesHeight;
}



// Draw each of the panels.
void ItemInfoDisplay::DrawDescription(const Point &topLeft) const
{
	Rectangle hoverTarget = Rectangle::FromCorner(topLeft, Point(PanelWidth(), DescriptionHeight()));
	Color color = hoverTarget.Contains(hoverPoint) ? *GameData::Colors().Get("bright") : *GameData::Colors().Get("medium");
	const Font &font = FontSet::Get(14);
	font.Draw(description, topLeft + Point(10., 12.), color);
}



void ItemInfoDisplay::DrawAttributes(const Point &topLeft) const
{
	Draw(topLeft, attributeLabels, attributeValues);
}



void ItemInfoDisplay::DrawTooltips() const
{
	if(!hoverCount || hoverCount-- < HOVER_TIME || hoverText.GetText().empty())
		return;
	
	const Font &font = FontSet::Get(14);
	int hoverParagraphBreak = font.ParagraphBreak(commonLayout);
	Point boxSize = font.FormattedBounds(hoverText) + Point(20., 20. - hoverParagraphBreak);
	
	Point topLeft = hoverPoint;
	if(topLeft.X() + boxSize.X() > Screen::Right())
		topLeft.X() -= boxSize.X();
	if(topLeft.Y() + boxSize.Y() > Screen::Bottom())
		topLeft.Y() -= boxSize.Y();
	
	FillShader::Fill(topLeft + .5 * boxSize, boxSize, *GameData::Colors().Get("tooltip background"));
	font.Draw(hoverText, topLeft + Point(10., 10.), *GameData::Colors().Get("medium"));
}



// Update the location where the mouse is hovering.
void ItemInfoDisplay::Hover(const Point &point)
{
	hoverPoint = point;
	hasHover = true;
}



void ItemInfoDisplay::ClearHover()
{
	hasHover = false;
}



void ItemInfoDisplay::UpdateDescription(const string &text, const vector<string> &licenses, bool isShip)
{
	if(licenses.empty())
		description.SetText(text);
	else
	{
		static const string NOUN[2] = {"outfit", "ship"};
		string fullText = text + "\tTo purchase this " + NOUN[isShip] + " you must have ";
		for(unsigned i = 0; i < licenses.size(); ++i)
		{
			bool isVoweled = false;
			for(const unsigned char &c : "aeiou")
			{
				const unsigned char beginning  = *licenses[i].begin();
				if(beginning == c || beginning == toupper(c))
					isVoweled = true;
			}
			if(i)
			{
				if(licenses.size() > 2)
					fullText += ", ";
				else
					fullText += " ";
			}
			if(i && i == licenses.size() - 1)
				fullText += "and ";
			fullText += (isVoweled ? "an " : "a ") + licenses[i] + " License";
		}
		fullText += ".\n";
		description.SetText(fullText);
	}
	
	// Pad by 10 pixels on the top and bottom.
	const Font &font = FontSet::Get(14);
	descriptionHeight = font.FormattedHeight(description) + 20;
}



Point ItemInfoDisplay::Draw(Point point, const vector<string> &labels, const vector<string> &values) const
{
	// Add ten pixels of padding at the top.
	point.Y() += 10.;
	
	// Get standard colors to draw with.
	const Color &labelColor = *GameData::Colors().Get("medium");
	const Color &valueColor = *GameData::Colors().Get("bright");
	
	Table table;
	// Use 10-pixel margins on both sides.
	table.AddColumn(10, {WIDTH - 20});
	table.AddColumn(WIDTH - 10, {WIDTH - 20, Alignment::RIGHT});
	table.SetHighlight(0, WIDTH);
	table.DrawAt(point);
	
	for(unsigned i = 0; i < labels.size() && i < values.size(); ++i)
	{
		if(labels[i].empty())
		{
			table.DrawGap(10);
			continue;
		}
		
		CheckHover(table, labels[i]);
		table.Draw(labels[i], values[i].empty() ? valueColor : labelColor);
		table.Draw(values[i], valueColor);
	}
	return table.GetPoint();
}



void ItemInfoDisplay::CheckHover(const Table &table, const string &label) const
{
	if(!hasHover)
		return;
	
	Point distance = hoverPoint - table.GetCenterPoint();
	Point radius = .5 * table.GetRowSize();
	if(abs(distance.X()) < radius.X() && abs(distance.Y()) < radius.Y())
	{
		hoverCount += 2 * (label == hover);
		hover = label;
		if(hoverCount >= HOVER_TIME)
		{
			hoverCount = HOVER_TIME;
			hoverText.SetText(GameData::Tooltip(label));
		}
	}
}
