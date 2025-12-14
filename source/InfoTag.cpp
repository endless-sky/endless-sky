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

#include <cmath>

using namespace std;

namespace {
	// Default border color.
	const Color white(1.f, 1.f, 1.f, 1.f);
	// How close to zero is zero-enough.
	double EPSILON = 0.000001;
	// Specifically want to avoid (0, 0) as the non-solution.
	Point NO_SOLUTION = Point(-1000000, 1000000);

	pair<Rectangle, vector<Point>> CreateBoxAndPoints(const Point &anchor, const Point &boxSize,
		InfoTag::Direction facing, InfoTag::Affinity affinity, double earLength, double earWidth)
	{
		// Starting with a box that is down and to the right from the anchor.
		Rectangle box = Rectangle::FromCorner(anchor, boxSize);

		// The points vector will always start at and end at the anchor.
		vector<Point> points;

		Point ccw_offset;
		Point cw_offset;
		Point center_offset;
		Point leg1_rel;
		Point leg2_rel;
		double halfEarWidth = 0.5 * earWidth;

		// Shift the box left and/or up accordingly with the chosen direction
		// which the ear will be facing as well as its affinity.
		if(facing == InfoTag::Direction::NORTH)
		{
			box += Point(0, earLength);
			leg1_rel = Point(-halfEarWidth, earLength);
			leg2_rel = Point(halfEarWidth, earLength);
			ccw_offset = Point(-halfEarWidth, 0);
			cw_offset = Point(boxSize.X(), 0) + ccw_offset;
			center_offset = Point(-boxSize.X() / 2, 0);
		}
		else if(facing == InfoTag::Direction::SOUTH)
		{
			box -= Point(0, boxSize.Y() + earLength);
			leg1_rel = Point(halfEarWidth, -earLength);
			leg2_rel = Point(-halfEarWidth, -earLength);
			ccw_offset = Point(-boxSize.X() + halfEarWidth, 0);
			cw_offset = Point(boxSize.X(), 0) + ccw_offset;
			center_offset = Point(-boxSize.X() / 2, 0);
		}
		else if(facing == InfoTag::Direction::WEST)
		{
			box += Point(earLength, 0);
			leg1_rel = Point(earLength, halfEarWidth);
			leg2_rel = Point(earLength, -halfEarWidth);
			ccw_offset = Point(0, -(boxSize.Y() - halfEarWidth));
			cw_offset = Point(0, boxSize.Y()) + ccw_offset;
			center_offset = Point(0, -boxSize.Y() / 2);
		}
		else
		{
			box -= Point(boxSize.X() + earLength, 0);
			leg1_rel = Point(-earLength, -halfEarWidth);
			leg2_rel = Point(-earLength, halfEarWidth);
			ccw_offset = Point(0, -halfEarWidth);
			cw_offset = Point(0, boxSize.Y()) + ccw_offset;
			center_offset = Point(0, -boxSize.Y() / 2);
		}

		if(affinity == InfoTag::Affinity::CCW)
			box += ccw_offset;
		else if(affinity == InfoTag::Affinity::CW)
			box -= cw_offset;
		else
			box += center_offset;

		// Collect the points that will be used to draw the borderWidth:
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
		points.emplace_back(anchor);

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

	pair<Rectangle, vector<Point>> PositionBoxAndPoints(const Point &anchor, const Point &boxSize,
		InfoTag::Direction facing, InfoTag::Affinity affinity, double earLength, double earWidth)
	{
		// Generate a InfoTag box from the given parameters.
		pair boxAndPoints = CreateBoxAndPoints(anchor, boxSize, facing, affinity, earLength, earWidth);
		Rectangle box = boxAndPoints.first;

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
		return onScreen ? boxAndPoints : CreateBoxAndPoints(anchor, boxSize, facing, affinity, earLength, earWidth);
	}

	Point Intersection(pair<Point, Point> line1, pair<Point, Point> line2, bool returnProjection = false)
	{

		Point ray1 = line1.second - line1.first;
		Point ray2 = line2.second - line2.first;
		double ray1CrossRay2 = ray1.Cross(ray2);

		// Check for parallel lines, no solution.
		if (abs(ray1CrossRay2) < EPSILON)
			return NO_SOLUTION;

		Point ray3 = line2.first - line1.first;
		double s = ray3.Cross(ray2) / ray1CrossRay2;
		double t = ray3.Cross(ray1) / ray1CrossRay2;

		// Check bounds for segments (0 <= s, t <= 1).
		if(returnProjection || (s >= 0 && s <= 1 &&  t >= 0 && t <= 1))
		{
			auto intersection = line1.first + ray1 * s;
			auto p = Point(floor(intersection.X()), floor(intersection.Y()));
			return p;
		}

		return NO_SOLUTION;
	}

	vector<Point> CalculateCalloutPointer(const Rectangle &box, const Point &anchor, double earWidth = 15.) {
		vector<Point> points;
		// Ok, we need to start with finding the intersection of the center-to-anchor with the box (if any)
		// Search the four sides until we find the intersection:
		vector corners = {box.TopLeft(), box.BottomLeft(), box.BottomRight(), box.TopRight()};
		Point intersection;
		// Because `(i - 1) % 4` will return -1 when `i` is 0, we're going to work one loop ahead for modulus' sake.
		int i = 4;
		for( ; i < 8; i++)
		{
			intersection = Intersection({box.Center(), anchor}, {corners[i % 4], corners[(i + 1) % 4]}, false);
			if(intersection != NO_SOLUTION)
				break;
		}

		if(intersection == NO_SOLUTION)
		{
			for(int k = 0; k < 5; k++)
				points.emplace_back(corners[k % 4]);
			return points;
		}

		// Now that we have the intersection, we can find the intersections in the sides of the box. The first one will
		// be on this same side. When the centerline is perpendicular to the edge of the box, these are plus-or-minus
		// <halfEarWidth> to either side of the center line on the same leg of the box which the centerline is
		// intersecting. However, in the case that there is less than <halfEarWidth> remaining to the left or right of
		// the centerline, then one point will be located on the next or previous side of the box.
		//
		//    +---x-----o------->
		//    |    \            >
		//    o     \           >
		//    |_/\/\/\/\/\/\/\/\>
		//
		// However, when intersection is too close to the corner, we must keep the distance between the two points where
		// the ear touches the box <earWidth> apart on the diagonal.

		double halfEarWidth = 0.5 * earWidth;

		pair lineNext = {corners[(i + 1) % 4], corners[(i + 2) % 4]};
		pair linePrev = {corners[(i - 1) % 4], corners[i % 4]};
		pair line = {corners[i % 4], corners[(i + 1) % 4]};

		auto directionNext = (lineNext.second - lineNext.first).Unit();
		auto directionPrev = (linePrev.second - linePrev.first).Unit();
		auto direction = (line.second - line.first).Unit();

		auto distNext = abs(intersection.Distance(line.second));
		auto distPrev = abs(intersection.Distance(line.first));

		Point one = intersection - halfEarWidth * direction;
		Point two = intersection + halfEarWidth * direction;

		int moreCorners = 4;

		if(distNext < halfEarWidth)
		{if((anchor - line.second).Dot(direction) > 0)
			{
				double normal = sqrt(earWidth * earWidth - (distNext + halfEarWidth) * (distNext + halfEarWidth));
				two = line.second + normal * directionNext;
				++i;
				--moreCorners;
			}
			else
			{
				two = line.second;
				one = line.second - earWidth * direction;
			}
		}

		if(distPrev < halfEarWidth)
		{
			if((line.first - anchor).Dot(direction) > 0)
			{
				double normal = sqrt(earWidth * earWidth - (distPrev + halfEarWidth) * (distPrev + halfEarWidth));
				one = line.first - normal * directionPrev;
				--moreCorners;
			}
			else
			{
				one = line.first;
				two = line.first + earWidth * direction;
			}
		}

		points.emplace_back(one);
		points.emplace_back(anchor);
		points.emplace_back(two);

		for(int j = ++i; j < i + moreCorners; j++)
		{
			points.emplace_back(corners[j % 4]);
		}

		// Close the loop.
		points.emplace_back(points[0]);

		return points;
	}
}



void InfoTag::InitShapeAndPlacement(Point anchor, Direction facing, Affinity affinity, std::string text,
	Alignment alignment, double width, bool shrink, double earLength) {
	this->anchor = anchor;
	this->box = {{0, 0}, {width, 0}};
	this->facing = facing;
	this->affinity = affinity;
	this->earLength = earLength;
	this->shrink = shrink;

	this->wrap.SetFont(FontSet::Get(14));
	// 10 pixels of padding will be left on either side of the InfoTag box.
	this->wrap.SetAlignment(alignment);
	SetText(text, shrink);

	Recalculate();
}



void InfoTag::InitShapeAndPlacement(std::string element, Point offset, Direction facing, Affinity affinity,
	std::string text, Alignment alignment, double width, bool shrink, double earLength)
{
	this->element = element;
	this->offset = offset;
	this->box = {{0, 0}, {width, 0}};
	this->facing = facing;
	this->affinity = affinity;
	this->earLength = earLength;
	this->shrink = shrink;

	this->wrap.SetFont(FontSet::Get(14));
	// 10 pixels of padding will be left on either side of the InfoTag box.
	this->wrap.SetAlignment(alignment);
	SetText(text, shrink);

	Recalculate();
}



void InfoTag::InitShapeAndPlacement(Point center, Point anchor, std::string text, Alignment alignment, double width,
	bool shrink, double earWidth)
{
	this->anchor = anchor;
	this->box = {center, {width, 0}};
	this->facing = Direction::NONE;
	this->affinity = Affinity::NONE;
	this->earWidth = earWidth;
	this->shrink = shrink;

	this->wrap.SetFont(FontSet::Get(14));
	this->wrap.SetAlignment(alignment);
	SetText(text, shrink);

	Recalculate();
}



void InfoTag::InitShapeAndPlacement(Point center, std::string element, Point offset, std::string text,
	Alignment alignment, double width, bool shrink, double earWidth)
{
	this->element = element;
	this->offset = offset;
	this->box = {center, {width, 0}};
	this->facing = Direction::NONE;
	this->affinity = Affinity::NONE;
	this->earWidth = earWidth;
	this->shrink = shrink;

	this->wrap.SetFont(FontSet::Get(14));
	// 10 pixels of padding will be left on either side of the InfoTag box.
	this->wrap.SetAlignment(alignment);
	SetText(text, shrink);

	Recalculate();
}



void InfoTag::InitBorderAndFill(const Color *backColor, const Color *fontColor, const Color *borderColor,
	const Color *borderColor2, double borderWidth)
{
	this->borderWidth = borderWidth;

	this->backColor = backColor;
	this->backColor2 = backColor;
	this->fontColor = fontColor;
	this->borderColor = borderColor;
	this->borderColor2 = borderColor2;
}



void InfoTag::SetAnchor(const Point &anchor)
{
	this->anchor = anchor;
	Recalculate();
}



void InfoTag::Draw() const
{
	PolygonShader::Draw(points, *backColor, *borderColor, *borderColor2, box.TopLeft(), box.BottomRight(), borderWidth);
	wrap.Draw(box.TopLeft() + Point(padding, padding), *fontColor);
}



// Determine the InfoTag's shape and location.
void InfoTag::Recalculate()
{
	// First, determine the size of the text in the InfoBox.
	Point boxSize(wrap.WrapWidth() + 2 * padding, wrap.Height(false) + 2 * padding);

	// In some cases the anchor will be calculated (element and offset provided).
	// TODO: do this!

	// In some cases the box must be calculated.
	if(affinity != Affinity::NONE && facing != Direction::NONE)
	{
		pair boxAndPoints = PositionBoxAndPoints(anchor, boxSize, facing, affinity, earLength, earWidth);
		box = boxAndPoints.first;
		points = boxAndPoints.second;
	}
	// Otherwise, the ear/box intersection needs to be computed.
	else
	{
		box = Rectangle(box.Center(), boxSize);
		points = CalculateCalloutPointer(box, anchor, earWidth);
	}
}



void InfoTag::SetText(const string &newText, bool shrink)
{
	this->shrink = shrink;
	SetText(newText);
}



void InfoTag::SetText(const string &newText)
{
	// Reset the wrap width each time we set text in case the WrappedText
	// was previously shrunk to the size of the text.
	wrap.SetWrapWidth(box.Width() - 2 * padding);
	wrap.Wrap(newText);
	if(shrink)
	{
		// Shrink the InfoTag width to fit the length of the text.
		int longest = wrap.LongestLineWidth();
		if(longest < wrap.WrapWidth())
		{
			wrap.SetWrapWidth(longest);
			wrap.Wrap(newText);
		}
	}
}



bool InfoTag::HasText() const
{
	return wrap.Height();
}



void InfoTag::Clear()
{
	wrap.Wrap("");
}



void InfoTag::SetBackgroundColor(const Color *backColor, const Color *backColor2)
{
	this->backColor = backColor;
	if(backColor2)
		this->backColor2 = backColor2;
	else
		this->backColor2 = backColor;
}



void InfoTag::SetFontColor(const Color *fontColor)
{
	this->fontColor = fontColor;
}
