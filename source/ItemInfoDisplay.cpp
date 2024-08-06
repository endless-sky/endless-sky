/* ItemInfoDisplay.cpp
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

#include "ItemInfoDisplay.h"

#include "text/alignment.hpp"
#include "Color.h"
#include "FillShader.h"
#include "text/FontSet.h"
#include "GameData.h"
#include "Rectangle.h"
#include "Screen.h"
#include "text/Table.h"

using namespace std;

namespace {
	const int HOVER_TIME = 60;
}



ItemInfoDisplay::ItemInfoDisplay()
{
	description.SetAlignment(Alignment::JUSTIFIED);
	description.SetWrapWidth(WIDTH - 20);
	description.SetFont(FontSet::Get(14));

	hoverText.SetAlignment(Alignment::JUSTIFIED);
	hoverText.SetWrapWidth(WIDTH - 20);
	hoverText.SetFont(FontSet::Get(14));
}



// Get the panel width.
int ItemInfoDisplay::PanelWidth()
{
	return WIDTH;
}



int ItemInfoDisplay::AttributesHeight() const
{
	return attributes.Height();
}



bool ItemInfoDisplay::HasDescription() const
{
	return description.LongestLineWidth();
}



// Draw each of the panels.
Point ItemInfoDisplay::DrawDescription(const Point &topLeft) const
{
	Rectangle hoverTarget = Rectangle::FromCorner(topLeft, Point(PanelWidth(), description.Height() + 20.));
	Color color = hoverTarget.Contains(hoverPoint) ? *GameData::Colors().Get("bright") : *GameData::Colors().Get("medium");
	description.Draw(topLeft + Point(10., 12.), color);

	// If there is a description, pad under it by 20 pixels.
	int descriptionHeight = description.Height();
	if(descriptionHeight)
		descriptionHeight += 20;
	return topLeft + Point(0., descriptionHeight);
}



Point ItemInfoDisplay::DrawAttributes(const Point &topLeft) const
{
	return Draw(attributes, topLeft);
}



void ItemInfoDisplay::DrawTooltips() const
{
	if(!hoverCount || hoverCount-- < HOVER_TIME || !hoverText.Height())
		return;

	Point textSize(hoverText.WrapWidth(), hoverText.Height() - hoverText.ParagraphBreak());
	Point boxSize = textSize + Point(20., 20.);

	Point topLeft = hoverPoint;
	if(topLeft.X() + boxSize.X() > Screen::Right())
		topLeft.X() -= boxSize.X();
	if(topLeft.Y() + boxSize.Y() > Screen::Bottom())
		topLeft.Y() -= boxSize.Y();

	FillShader::Fill(topLeft + .5 * boxSize, boxSize, *GameData::Colors().Get("tooltip background"));
	hoverText.Draw(topLeft + Point(10., 10.), *GameData::Colors().Get("medium"));
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



FlexTable ItemInfoDisplay::CreateTable(const vector<string> &labels, const vector<string> &values)
{
	// Get standard colors to draw with.
	const Color &labelColor = *GameData::Colors().Get("medium");
	const Color &valueColor = *GameData::Colors().Get("bright");

	// Use 10-pixel margins on both sides.
	FlexTable table(WIDTH - 20, 2);
	table.SetFlexStrategy(FlexTable::FlexStrategy::INDIVIDUAL);
	table.GetColumn(1).SetAlignment(Alignment::RIGHT);

	for(unsigned i = 0; i < labels.size() && i < values.size(); ++i)
	{
		if(labels[i].empty() && i)
		{
			table.GetCell(-1, 0).SetBottomGap(table.GetCell(-1, 0).GetBottomGap() + 10);
			continue;
		}

		if(values[i].empty())
			table.FillUnifiedRow(labels[i], valueColor);
		else
			table.FillRow({{labels[i], labelColor}, {values[i], valueColor}});
	}
	return table;
}



void ItemInfoDisplay::UpdateDescription(const string &text, const vector<string> &licenses, bool isShip)
{
	if(licenses.empty())
		description.Wrap(text);
	else
	{
		static const string NOUN[2] = {"outfit", "ship"};
		string fullText = text + "\tTo purchase this " + NOUN[isShip] + " you must have ";
		for(unsigned i = 0; i < licenses.size(); ++i)
		{
			bool isVoweled = false;
			for(const char &c : "aeiou")
				if(*licenses[i].begin() == c || *licenses[i].begin() == toupper(c))
					isVoweled = true;
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
		description.Wrap(fullText);
	}
}



Point ItemInfoDisplay::Draw(FlexTable &table, Point drawPoint, int labelIndex) const
{
	Point end = table.Draw(drawPoint);
	CheckHover(table, drawPoint, labelIndex);
	return end;
}



void ItemInfoDisplay::CheckHover(FlexTable &table, Point drawPoint, int labelIndex) const
{
	if(!hasHover)
		return;

	for(int row = 0; row < table.Rows(); ++row)
	{
		if(table.GetRowHitbox(row, drawPoint).Contains(hoverPoint))
		{
			const string &label = table.GetCell(row, labelIndex).Text();
			hoverCount += 2 * (label == hover);
			hover = label;
			if(hoverCount >= HOVER_TIME)
			{
				hoverCount = HOVER_TIME;
				hoverText.Wrap(GameData::Tooltip(label));
			}
		}
	}
}
