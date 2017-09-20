/* TextTooltip.cpp
Copyright (c) 2017 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "TextTooltip.h"

#include "Color.h"
#include "FillShader.h"
#include "FontSet.h"
#include "GameData.h"
#include "Point.h"
#include "Screen.h"
#include "WrappedText.h"

using namespace std;

namespace {
	const int HOVER_TIME = 60;
}



TextTooltip::TextTooltip()
{
	tooltipText.SetAlignment(WrappedText::JUSTIFIED);
	tooltipText.SetWrapWidth(250 - 20);
	tooltipText.SetFont(FontSet::Get(14));
}



void TextTooltip::Draw()
{
	if(hoverLabel.empty())
	{
		hoverCount = max(0, hoverCount - 1);
		return;
	}
	
	hoverCount = min(HOVER_TIME, hoverCount + 1);
	if(hoverCount < HOVER_TIME)
		return;
	
	Point textSize(tooltipText.WrapWidth(), tooltipText.Height() - tooltipText.ParagraphBreak());
	Point boxSize = textSize + Point(20., 20.);
	
	Point topLeft = hoverPoint;
	if(topLeft.X() + boxSize.X() > Screen::Right())
		topLeft.X() -= boxSize.X();
	if(topLeft.Y() + boxSize.Y() > Screen::Bottom())
		topLeft.Y() -= boxSize.Y();
	
	FillShader::Fill(topLeft + .5 * boxSize, boxSize, Color(.2, 1.));
	tooltipText.Draw(topLeft + Point(10., 10.), Color(.5, 0.));
}



void TextTooltip::Clear()
{
	hoverCount = 0;
	hoverLabel.clear();
	tooltipText.Wrap("");
}



const Point &TextTooltip::HoverPoint() const
{
	return hoverPoint;
}



void TextTooltip::SetHoverPoint(const Point &point)
{
	hoverPoint = point;
}



void TextTooltip::SetLabel(const std::string &label)
{
	if(hoverLabel == label)
		return;
	
	hoverLabel = label;
	if(label.empty())
		return;
	
	string text = GameData::Tooltip(hoverLabel);
	if(text.empty())
		text = "Missing tooltip: \"" + label + "\"";
	
	tooltipText.Wrap(text);
}



const string &TextTooltip::Label() const
{
	return hoverLabel;
}



vector<ClickZone<string>> &TextTooltip::Zones()
{
	return hoverZones;
}



void TextTooltip::CheckZones()
{
	for(const auto &zone : hoverZones)
		if(zone.Contains(hoverPoint))
		{
			SetLabel(zone.Value());
			return;
		}
	
	SetLabel("");
}



WrappedText &TextTooltip::Text()
{
	return tooltipText;
}


int TextTooltip::HoverCount() const
{
	return hoverCount;
}
