/* Tooltip.cpp
Copyright (c) 2025 by Amazinite

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "Tooltip.h"

#include "shader/FillShader.h"
#include "text/FontSet.h"
#include "Point.h"
#include "Preferences.h"
#include "Screen.h"

using namespace std;

namespace {
	Rectangle CreateBox(const Rectangle &zone, const Point &boxSize,
		Tooltip::Direction direction, Tooltip::Corner corner)
	{
		// Find the anchor point that the tooltip should be created from.
		Point anchor;
		if(corner == Tooltip::Corner::TOP_LEFT)
			anchor = zone.TopLeft();
		else if(corner == Tooltip::Corner::TOP_RIGHT)
			anchor = zone.TopRight();
		else if(corner == Tooltip::Corner::BOTTOM_LEFT)
			anchor = zone.BottomLeft();
		else
			anchor = zone.BottomRight();

		Rectangle box = Rectangle::FromCorner(anchor, boxSize);

		// The default box has a direction of DOWN_RIGHT, so shift the box left
		// or up accordingly with the chosen direction.
		if(direction == Tooltip::Direction::UP_LEFT || direction == Tooltip::Direction::DOWN_LEFT)
			box -= Point(boxSize.X(), 0);
		if(direction == Tooltip::Direction::UP_LEFT || direction == Tooltip::Direction::UP_RIGHT)
			box -= Point(0, boxSize.Y());

		return box;
	}

	// Determine where this tooltip should be positioned. Account for whether the
	// default settings would generate a tooltip that goes off screen, and create
	// an adjusted tooltip position if this occurs.
	Rectangle PositionBox(const Rectangle &zone, const Point &boxSize,
		Tooltip::Direction direction, Tooltip::Corner corner)
	{
		// Generate a tooltip box from the given parameters.
		Rectangle box = CreateBox(zone, boxSize, direction, corner);

		// If the tooltip goes off one of the edges of the screen, swap the draw
		// direction to go the other way. Also swap the corner that the tooltip
		// is being drawn from as to not overlap the hover zone.
		bool onScreen = true;
		if(box.Left() < Screen::Left())
		{
			onScreen = false;
			if(direction == Tooltip::Direction::UP_LEFT)
			{
				direction = Tooltip::Direction::UP_RIGHT;
				if(corner == Tooltip::Corner::BOTTOM_LEFT)
					corner = Tooltip::Corner::BOTTOM_RIGHT;
			}
			else if(direction == Tooltip::Direction::DOWN_LEFT)
			{
				direction = Tooltip::Direction::DOWN_RIGHT;
				if(corner == Tooltip::Corner::TOP_LEFT)
					corner = Tooltip::Corner::TOP_RIGHT;
			}
		}
		else if(box.Right() > Screen::Right())
		{
			onScreen = false;
			if(direction == Tooltip::Direction::UP_RIGHT)
			{
				direction = Tooltip::Direction::UP_LEFT;
				if(corner == Tooltip::Corner::BOTTOM_RIGHT)
					corner = Tooltip::Corner::BOTTOM_LEFT;
			}
			else if(direction == Tooltip::Direction::DOWN_RIGHT)
			{
				direction = Tooltip::Direction::DOWN_LEFT;
				if(corner == Tooltip::Corner::TOP_RIGHT)
					corner = Tooltip::Corner::TOP_LEFT;
			}
		}

		if(box.Top() < Screen::Top())
		{
			onScreen = false;
			if(direction == Tooltip::Direction::UP_RIGHT)
			{
				direction = Tooltip::Direction::DOWN_RIGHT;
				if(corner == Tooltip::Corner::TOP_LEFT)
					corner = Tooltip::Corner::BOTTOM_LEFT;
			}
			else if(direction == Tooltip::Direction::UP_LEFT)
			{
				direction = Tooltip::Direction::DOWN_LEFT;
				if(corner == Tooltip::Corner::TOP_RIGHT)
					corner = Tooltip::Corner::BOTTOM_RIGHT;
			}
		}
		else if(box.Bottom() > Screen::Bottom())
		{
			onScreen = false;
			if(direction == Tooltip::Direction::DOWN_RIGHT)
			{
				direction = Tooltip::Direction::UP_RIGHT;
				if(corner == Tooltip::Corner::BOTTOM_LEFT)
					corner = Tooltip::Corner::TOP_LEFT;
			}
			else if(direction == Tooltip::Direction::DOWN_LEFT)
			{
				direction = Tooltip::Direction::UP_LEFT;
				if(corner == Tooltip::Corner::BOTTOM_RIGHT)
					corner = Tooltip::Corner::TOP_RIGHT;
			}
		}

		// If the initial box doesn't fit on screen, generate a new one with
		// a different draw location. Don't bother checking if this second box
		// fits on screen, because if it doesn't, that means that the screen
		// is simply too small to fit this box.
		return onScreen ? box : CreateBox(zone, boxSize, direction, corner);
	}
}



Tooltip::Tooltip(int width, Alignment alignment, Direction direction, Corner corner,
		const Color *backColor, const Color *fontColor)
	: width(width), direction(direction), corner(corner), backColor(backColor), fontColor(fontColor)
{
	text.SetFont(FontSet::Get(14));
	// 10 pixels of padding will be left on either side of the tooltip box.
	text.SetWrapWidth(width - 20);
	text.SetAlignment(alignment);
	UpdateActivationCount();
}



void Tooltip::IncrementCount()
{
	if(hoverCount < activationHover)
		++hoverCount;
}



void Tooltip::DecrementCount()
{
	if(hoverCount)
		--hoverCount;
}



void Tooltip::ResetCount()
{
	hoverCount = 0;
}



bool Tooltip::ShouldDraw() const
{
	return hoverCount >= activationHover;
}



void Tooltip::SetZone(const Point &center, const Point &dimensions)
{
	zone = Rectangle(center, dimensions);
}



void Tooltip::SetZone(const Rectangle &zone)
{
	this->zone = zone;
}



void Tooltip::SetText(const string &newText, bool shrink)
{
	// Reset the wrap width each time we set text in case the WrappedText
	// was previously shrunk to the size of the text.
	text.SetWrapWidth(width - 20);
	text.Wrap(newText);
	if(shrink)
	{
		// Shrink the tooltip width to fit the length of the text.
		int longest = text.LongestLineWidth();
		if(longest < text.WrapWidth())
		{
			text.SetWrapWidth(longest);
			text.Wrap(newText);
		}
	}
}



bool Tooltip::HasText() const
{
	return text.Height();
}



void Tooltip::Clear()
{
	text.Wrap("");
}



void Tooltip::SetBackgroundColor(const Color *backColor)
{
	this->backColor = backColor;
}



void Tooltip::SetFontColor(const Color *fontColor)
{
	this->fontColor = fontColor;
}



void Tooltip::Draw(bool forceDraw) const
{
	if((!forceDraw && !ShouldDraw()) || !HasText())
		return;

	// Determine the tooltip's size and location.
	Point textSize(text.WrapWidth(), text.Height(false));
	Point boxSize = textSize + Point(20., 20.);
	Rectangle box = PositionBox(zone, boxSize, direction, corner);

	FillShader::Fill(box, *backColor);
	text.Draw(box.TopLeft() + Point(10., 10.), *fontColor);
}



void Tooltip::UpdateActivationCount()
{
	activationHover = Preferences::TooltipActivation();
}
