/* InfoTag.cpp
Copyright (c) 2025 by xobes

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "InfoTag.h"

#include "Color.h"
#include "text/FontSet.h"
#include "GameData.h"
#include "Point.h"
#include "shader/PolygonShader.h"
#include "Rectangle.h"
#include "Screen.h"

using namespace std;

namespace {
	WrappedText wrap;

	// Default border color
	const Color white(1.f, 1.f, 1.f, 1.f);

	pair<Rectangle, vector<Point>> CreateBox(const Point &anchor, const Point &boxSize,
		InfoTag::Direction facing, InfoTag::Affinity affinity, const double &earLength)
	{
		// Starting with a box that is down and to the right from the anchor.
		Rectangle box = Rectangle::FromCorner(anchor, boxSize);

		// The points vector will always start at and end at the anchor, but not include the anchor for that reason.
		vector<Point> points;

		double earWidth = 7.;
		Point ccw_offset;
		Point cw_offset;
		Point center_offset;
		Point leg1_rel;
		Point leg2_rel;

		// Shift the box left and/or up accordingly with the chosen direction
		// which the ear will be facing as well as its affinity.
		if(facing == InfoTag::Direction::NORTH)
		{
			box += Point(0, earLength);
			leg1_rel = Point(-earWidth, earLength);
			leg2_rel = Point(earWidth, earLength);
			ccw_offset = Point(-earWidth, 0);
			cw_offset = Point(boxSize.X(), 0) + ccw_offset;
			center_offset = Point(-boxSize.X() / 2, 0);
		}
		else if(facing == InfoTag::Direction::SOUTH)
		{
			box -= Point(0, boxSize.Y() + earLength);
			leg1_rel = Point(earWidth, -earLength);
			leg2_rel = Point(-earWidth, -earLength);
			ccw_offset = Point(-boxSize.X() + earWidth, 0);
			cw_offset = Point(boxSize.X(), 0) + ccw_offset;
			center_offset = Point(-boxSize.X() / 2, 0);
		}
		else if(facing == InfoTag::Direction::WEST)
		{
			box += Point(earLength, 0);
			leg1_rel = Point(earLength, earWidth);
			leg2_rel = Point(earLength, -earWidth);
			ccw_offset = Point(0, -(boxSize.Y() - earWidth));
			cw_offset = Point(0, boxSize.Y()) + ccw_offset;
			center_offset = Point(0, -boxSize.Y() / 2);
		}
		else
		{
			box -= Point(boxSize.X() + earLength, 0);
			leg1_rel = Point(-earLength, -earWidth);
			leg2_rel = Point(-earLength, earWidth);
			ccw_offset = Point(0, -earWidth);
			cw_offset = Point(0, boxSize.Y()) + ccw_offset;
			center_offset = Point(0, -boxSize.Y() / 2);
		}

		if(affinity == InfoTag::Affinity::CCW)
			box += ccw_offset;
		else if(affinity == InfoTag::Affinity::CW)
			box -= cw_offset;
		else
			box += center_offset;

		// Collect the points that will be used to draw the border:
		points.emplace_back(anchor + leg1_rel);
		if(facing == InfoTag::Direction::NORTH)
		{
			if(affinity != InfoTag::Affinity::CCW)
				points.emplace_back(box.TopLeft());
			points.emplace_back(box.BottomLeft());
			points.emplace_back(box.BottomRight());
			if(affinity != InfoTag::Affinity::CW)
				points.emplace_back(box.TopRight());
		}
		else if(facing == InfoTag::Direction::SOUTH)
		{
			if(affinity != InfoTag::Affinity::CCW)
				points.emplace_back(box.BottomRight());
			points.emplace_back(box.TopRight());
			points.emplace_back(box.TopLeft());
			if(affinity != InfoTag::Affinity::CW)
				points.emplace_back(box.BottomLeft());
		}
		else if(facing == InfoTag::Direction::WEST)
		{
			if(affinity != InfoTag::Affinity::CCW)
				points.emplace_back(box.BottomLeft());
			points.emplace_back(box.BottomRight());
			points.emplace_back(box.TopRight());
			if(affinity != InfoTag::Affinity::CW)
				points.emplace_back(box.TopLeft());
		}
		else
		{
			if(affinity != InfoTag::Affinity::CCW)
				points.emplace_back(box.TopRight());
			points.emplace_back(box.TopLeft());
			points.emplace_back(box.BottomLeft());
			if(affinity != InfoTag::Affinity::CW)
				points.emplace_back(box.BottomRight());
		}
		points.emplace_back(anchor + leg2_rel);

		return make_pair(box, points);
	}

	InfoTag::Affinity Opposite(InfoTag::Affinity affinity)
	{
		if(affinity == InfoTag::Affinity::CCW)
			return InfoTag::Affinity::CW;
		if(affinity == InfoTag::Affinity::CW)
			return InfoTag::Affinity::CCW;
		return InfoTag::Affinity::CENTER;
	}

	// Determine where this InfoTag should be positioned. Account for whether the
	// specified settings would generate a InfoTag that goes off-screen, and create
	// an adjusted InfoTag position if this occurs.
	pair<Rectangle, vector<Point>> PositionBox(const Point &anchor, const Point &boxSize,
		InfoTag::Direction facing, InfoTag::Affinity affinity, const double &earLength)
	{
		// Generate a InfoTag box from the given parameters.
		pair boxPoints = CreateBox(anchor, boxSize, facing, affinity, earLength);
		Rectangle box = boxPoints.first;

		// If the InfoTag goes off one of the edges of the screen, swap the draw
		// direction to go the other way and/or swap the affinity of the ear so the
		// InfoTag is drawn in a way that it fits on the screen.
		bool onScreen = true;
		if(box.Left() < Screen::Left())
		{
			onScreen = false;
			if(facing == InfoTag::Direction::EAST)
			{
				facing = InfoTag::Direction::WEST;
				affinity = Opposite(affinity);
			}
			else if(facing != InfoTag::Direction::WEST)
				affinity = Opposite(affinity);
		}
		else if(box.Right() > Screen::Right())
		{
			onScreen = false;
			if(facing == InfoTag::Direction::WEST)
			{
				facing = InfoTag::Direction::EAST;
				affinity = Opposite(affinity);
			}
			else if(facing != InfoTag::Direction::EAST)
				affinity = Opposite(affinity);
		}
		if(box.Top() < Screen::Top())
		{
			onScreen = false;
			if(facing == InfoTag::Direction::NORTH)
			{
				facing = InfoTag::Direction::SOUTH;
				affinity = Opposite(affinity);
			}
			else if(facing != InfoTag::Direction::SOUTH)
				affinity = Opposite(affinity);
		}
		else if(box.Bottom() > Screen::Bottom())
		{
			onScreen = false;
			if(facing == InfoTag::Direction::SOUTH)
			{
				facing = InfoTag::Direction::NORTH;
				affinity = Opposite(affinity);
			}
			else if(facing != InfoTag::Direction::NORTH)
				affinity = Opposite(affinity);
		}

		// If the initial box doesn't fit on screen, generate a new one with
		// a different draw location. Don't bother checking if this second box
		// fits on screen, because if it doesn't, that means that the screen
		// is simply too small to fit this box.
		return onScreen ? boxPoints : CreateBox(anchor, boxSize, facing, affinity, earLength);
	}
}



InfoTag::InfoTag(int width, Alignment alignment, Direction facing, Affinity affinity,
		const Color *backColor, const Color *fontColor, const Color *borderColor,
		double earLength, double border)
	: width(width), facing(facing), affinity(affinity), earLength(earLength), border(border),
		backColor(backColor), fontColor(fontColor), borderColor(borderColor)
{
	text.SetFont(FontSet::Get(14));
	// 10 pixels of padding will be left on either side of the InfoTag box.
	text.SetWrapWidth(width - 20);
	text.SetAlignment(alignment);
}



void InfoTag::SetAnchor(const Point &anchor)
{
	this->anchor = anchor;
}



void InfoTag::SetText(const string &newText, bool shrink)
{
	// Reset the wrap width each time we set text in case the WrappedText
	// was previously shrunk to the size of the text.
	text.SetWrapWidth(width - 20);
	text.Wrap(newText);
	if(shrink)
	{
		// Shrink the InfoTag width to fit the length of the text.
		int longest = text.LongestLineWidth();
		if(longest < text.WrapWidth())
		{
			text.SetWrapWidth(longest);
			text.Wrap(newText);
		}
	}
}



bool InfoTag::HasText() const
{
	return text.Height();
}



void InfoTag::Clear()
{
	text.Wrap("");
}



void InfoTag::SetBackgroundColor(const Color *backColor)
{
	this->backColor = backColor;
}



void InfoTag::SetFontColor(const Color *fontColor)
{
	this->fontColor = fontColor;
}



void InfoTag::Draw() const
{
	// Determine the InfoTag's size and location.
	Point textSize(text.WrapWidth(), text.Height(false));
	Point boxSize = textSize + Point(20., 20.);

	pair boxPoints = PositionBox(anchor, boxSize, facing, affinity, earLength);
	Rectangle box = boxPoints.first;
	vector<Point> points = boxPoints.second;

	points.emplace_back(anchor);
	PolygonShader::Draw(points, *backColor, *borderColor, border);
	text.Draw(box.TopLeft() + Point(10., 10.), *fontColor);
}



void InfoTag::Draw(Point anchor, std::string text, int width, Alignment alignment, Direction facing, Affinity affinity,
	const Color *backColor, const Color *fontColor, const Color *borderColor,
	bool shrink, double earLength, double border)
{
	wrap.SetFont(FontSet::Get(14));
	// 10 pixels of padding will be left on either side of the InfoTag box.
	wrap.SetWrapWidth(width - 20);
	wrap.SetAlignment(alignment);
	wrap.Wrap(text);

	// Shrink the InfoTag width to fit the length of the text.
	if(shrink)
	{
		int longest = wrap.LongestLineWidth();
		if(longest < wrap.WrapWidth())
		{
			wrap.SetWrapWidth(longest);
			wrap.Wrap(text);
		}
	}

	// Determine the InfoTag's size and location.
	Point textSize(wrap.WrapWidth(), wrap.Height(false));
	Point boxSize = textSize + Point(20., 20.);

	pair boxPoints = PositionBox(anchor, boxSize, facing, affinity, earLength);
	Rectangle box = boxPoints.first;
	vector<Point> points = boxPoints.second;

	// Handle default colors
	if(!backColor)
		backColor = GameData::Colors().Get("tooltip background");
	if(!fontColor)
		fontColor = GameData::Colors().Get("medium");
	if(!borderColor)
	{
		borderColor = &white;
	}

	points.emplace_back(anchor);
	PolygonShader::Draw(points, *backColor, *borderColor, border);
	wrap.Draw(box.TopLeft() + Point(10., 10.), *fontColor);
}
