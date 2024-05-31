/* Interface.cpp
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

#include "Interface.h"

#include "Angle.h"
#include "DataNode.h"
#include "text/DisplayText.h"
#include "FillShader.h"
#include "text/Font.h"
#include "text/FontSet.h"
#include "GameData.h"
#include "Information.h"
#include "text/layout.hpp"
#include "LineShader.h"
#include "MapPanel.h"
#include "OutlineShader.h"
#include "Panel.h"
#include "PointerShader.h"
#include "Rectangle.h"
#include "RingShader.h"
#include "Screen.h"
#include "Sprite.h"
#include "SpriteSet.h"
#include "SpriteShader.h"
#include "UI.h"

#include <algorithm>
#include <cmath>

using namespace std;

namespace {
	// Parse a set of tokens that specify horizontal and vertical alignment.
	Point ParseAlignment(const DataNode &node, int i = 1)
	{
		Point alignment;
		for( ; i < node.Size(); ++i)
		{
			if(node.Token(i) == "left")
				alignment.X() = -1.;
			else if(node.Token(i) == "top")
				alignment.Y() = -1.;
			else if(node.Token(i) == "right")
				alignment.X() = 1.;
			else if(node.Token(i) == "bottom")
				alignment.Y() = 1.;
			else if(node.Token(i) != "center")
				node.PrintTrace("Skipping unrecognized alignment:");
		}
		return alignment;
	}
}



// Destructor, which frees the memory used by the polymorphic list of elements.
Interface::~Interface()
{
	for(Element *element : elements)
		delete element;
}



// Load an interface.
void Interface::Load(const DataNode &node)
{
	// Skip unnamed interfaces.
	if(node.Size() < 2)
		return;
	// Re-loading an interface always clears the previous interface, rather than
	// appending new elements to the end of it.
	elements.clear();
	points.clear();
	values.clear();
	lists.clear();

	// First, figure out the anchor point of this interface.
	Point anchor = ParseAlignment(node, 2);

	// Now, parse the elements in it.
	string visibleIf;
	string activeIf;
	for(const DataNode &child : node)
	{
		if(child.Token(0) == "anchor")
			anchor = ParseAlignment(child);
		else if(child.Token(0) == "value" && child.Size() >= 3)
			values[child.Token(1)] = child.Value(2);
		else if((child.Token(0) == "point" || child.Token(0) == "box") && child.Size() >= 2)
		{
			// This node specifies a named point where custom drawing is done.
			points[child.Token(1)].Load(child, anchor);
		}
		else if(child.Token(0) == "list" && child.Size() >= 2)
		{
			auto &list = lists[child.Token(1)];
			for(const auto &grand : child)
				list.emplace_back(grand.Value(0));
		}
		else if(child.Token(0) == "visible" || child.Token(0) == "active")
		{
			// This node alters the visibility or activation of future nodes.
			string &str = (child.Token(0) == "visible" ? visibleIf : activeIf);
			if(child.Size() >= 3 && child.Token(1) == "if")
				str = child.Token(2);
			else
				str.clear();
		}
		else
		{
			// Check if this node specifies a known element type.
			if(child.Token(0) == "sprite" || child.Token(0) == "image" || child.Token(0) == "outline")
				elements.push_back(new ImageElement(child, anchor));
			else if(child.Token(0) == "label" || child.Token(0) == "string" || child.Token(0) == "button"
					|| child.Token(0) == "dynamic button")
				elements.push_back(new TextElement(child, anchor));
			else if(child.Token(0) == "bar" || child.Token(0) == "ring")
				elements.push_back(new BarElement(child, anchor));
			else if(child.Token(0) == "pointer")
				elements.push_back(new PointerElement(child, anchor));
			else if(child.Token(0) == "line")
				elements.push_back(new LineElement(child, anchor));
			else
			{
				child.PrintTrace("Skipping unrecognized element:");
				continue;
			}

			// If we get here, a new element was just added.
			elements.back()->SetConditions(visibleIf, activeIf);
		}
	}
}



// Draw this interface.
void Interface::Draw(const Information &info, Panel *panel) const
{
	for(const Element *element : elements)
		element->Draw(info, panel);
}



// Check if a named point exists.
bool Interface::HasPoint(const string &name) const
{
	return points.count(name);
}



// Get the center of the named point.
Point Interface::GetPoint(const string &name) const
{
	auto it = points.find(name);
	if(it == points.end())
		return Point();

	return it->second.Bounds().Center();
}



Rectangle Interface::GetBox(const string &name) const
{
	auto it = points.find(name);
	if(it == points.end())
		return Rectangle();

	return it->second.Bounds();
}



// Get a named value.
double Interface::GetValue(const string &name) const
{
	auto it = values.find(name);
	return (it == values.end() ? 0. : it->second);
}



// Get a named list.
const vector<double> &Interface::GetList(const string &name) const
{
	static vector<double> EMPTY;
	auto it = lists.find(name);
	return (it == lists.end() ? EMPTY : it->second);
}



// Members of the AnchoredPoint class:

// Get the point's location, given the current screen dimensions.
Point Interface::AnchoredPoint::Get() const
{
	return position + .5 * Screen::Dimensions() * anchor;
}



Point Interface::AnchoredPoint::Get(const Information &info) const
{
	const Rectangle &region = info.GetCustomRegion();
	return position + region.Center() + .5 * region.Dimensions() * anchor;
}



void Interface::AnchoredPoint::Set(const Point &position, const Point &anchor)
{
	this->position = position;
	this->anchor = anchor;
}



// Members of the Element base class:

// Create a new element. The alignment of the interface that contains
// this element is used to calculate the element's position.
void Interface::Element::Load(const DataNode &node, const Point &globalAnchor)
{
	// A location can be specified as:
	// center (+ dimensions):
	bool hasCenter = false;
	Point dimensions;

	// from (+ dimensions):
	Point fromPoint;
	Point fromAnchor = globalAnchor;

	// from + to:
	bool hasTo = false;
	Point toPoint;
	Point toAnchor = globalAnchor;

	// Assume that the subclass constructor already parsed this line of data.
	for(const DataNode &child : node)
	{
		const string &key = child.Token(0);
		if(key == "align" && child.Size() > 1)
			alignment = ParseAlignment(child);
		else if(key == "dimensions" && child.Size() >= 3)
			dimensions = Point(child.Value(1), child.Value(2));
		else if(key == "width" && child.Size() >= 2)
			dimensions.X() = child.Value(1);
		else if(key == "height" && child.Size() >= 2)
			dimensions.Y() = child.Value(1);
		else if(key == "center" && child.Size() >= 3)
		{
			if(child.Size() > 3)
				fromAnchor = toAnchor = ParseAlignment(child, 3);

			// The "center" key implies "align center."
			alignment = Point();
			fromPoint = toPoint = Point(child.Value(1), child.Value(2));
			hasCenter = true;
		}
		else if(key == "from" && child.Size() >= 6 && child.Token(3) == "to")
		{
			// Anything after the coordinates is an anchor point override.
			if(child.Size() > 6)
				fromAnchor = toAnchor = ParseAlignment(child, 6);

			fromPoint = Point(child.Value(1), child.Value(2));
			toPoint = Point(child.Value(4), child.Value(5));
			hasTo = true;
		}
		else if(key == "from" && child.Size() >= 3)
		{
			// Anything after the coordinates is an anchor point override.
			if(child.Size() > 3)
				fromAnchor = ParseAlignment(child, 3);

			fromPoint = Point(child.Value(1), child.Value(2));
		}
		else if(key == "to" && child.Size() >= 3)
		{
			// Anything after the coordinates is an anchor point override.
			if(child.Size() > 3)
				toAnchor = ParseAlignment(child, 3);

			toPoint = Point(child.Value(1), child.Value(2));
			hasTo = true;
		}
		else if(key == "pad" && child.Size() >= 3)
		{
			// Add this much padding when aligning the object within its bounding box.
			padding = Point(child.Value(1), child.Value(2));
		}
		else if(!ParseLine(child))
			child.PrintTrace("Skipping unrecognized attribute:");
	}

	// The "standard" way to specify a region is from + to. If it was specified
	// in a different way, convert it to that format:
	if(hasCenter)
	{
		// Center alone or center + dimensions.
		fromPoint -= .5 * dimensions;
		toPoint += .5 * dimensions;
	}
	else if(!hasTo)
	{
		// From alone or from + dimensions.
		toPoint = fromPoint + dimensions;
		toAnchor = fromAnchor;
	}
	from.Set(fromPoint, fromAnchor);
	to.Set(toPoint, toAnchor);
}



// Draw this element, relative to the given anchor point. If this is a
// button, it will add a clickable zone to the given panel.
void Interface::Element::Draw(const Information &info, Panel *panel) const
{
	if(!info.HasCondition(visibleIf))
		return;

	// Get the bounding box of this element, relative to the anchor point.
	Rectangle box = (info.HasCustomRegion() ? Bounds(info) : Bounds());
	// Check if this element is active.
	int state = info.HasCondition(activeIf);
	// Check if the mouse is hovering over this element.
	state += (state && box.Contains(UI::GetMouse()));
	// Place buttons even if they are inactive, in case the UI wants to show a
	// message explaining why the button is inactive.
	if(panel)
		Place(box, panel);

	// Figure out how the element should be aligned within its bounding box.
	Point nativeDimensions = NativeDimensions(info, state);
	Point slack = .5 * (box.Dimensions() - nativeDimensions) - padding;
	Rectangle rect(box.Center() + alignment * slack, nativeDimensions);

	Draw(rect, info, state);
}



// Set the conditions that control when this element is visible and active.
// An empty string means it is always visible or active.
void Interface::Element::SetConditions(const string &visible, const string &active)
{
	visibleIf = visible;
	activeIf = active;
}



// Get the bounding rectangle, relative to the anchor point.
Rectangle Interface::Element::Bounds() const
{
	return Rectangle::WithCorners(from.Get(), to.Get());
}



Rectangle Interface::Element::Bounds(const Information &info) const
{
	return Rectangle::WithCorners(from.Get(info), to.Get(info));
}



// Parse the given data line: one that is not recognized by Element
// itself. This returns false if it does not recognize the line, either.
bool Interface::Element::ParseLine(const DataNode &node)
{
	return false;
}



// Report the actual dimensions of the object that will be drawn.
Point Interface::Element::NativeDimensions(const Information &info, int state) const
{
	return Bounds().Dimensions();
}



// Draw this element in the given rectangle.
void Interface::Element::Draw(const Rectangle &rect, const Information &info, int state) const
{
}



// Add any click handlers needed for this element. This will only be
// called if the element is visible and active.
void Interface::Element::Place(const Rectangle &bounds, Panel *panel) const
{
}



// Members of the ImageElement class:

// Constructor.
Interface::ImageElement::ImageElement(const DataNode &node, const Point &globalAnchor)
{
	if(node.Size() < 2)
		return;

	// Remember whether this is an outline element.
	isOutline = (node.Token(0) == "outline");
	// If this is a "sprite," look up the sprite with the given name. Otherwise,
	// the sprite path will be dynamically supplied by the Information object.
	if(node.Token(0) == "sprite")
		sprite[Element::ACTIVE] = SpriteSet::Get(node.Token(1));
	else
		name = node.Token(1);

	// This function will call ParseLine() for any line it does not recognize.
	Load(node, globalAnchor);

	// Fill in any undefined state sprites.
	if(sprite[Element::ACTIVE])
	{
		if(!sprite[Element::INACTIVE])
			sprite[Element::INACTIVE] = sprite[Element::ACTIVE];
		if(!sprite[Element::HOVER])
			sprite[Element::HOVER] = sprite[Element::ACTIVE];
	}
}



// Parse the given data line: one that is not recognized by Element
// itself. This returns false if it does not recognize the line, either.
bool Interface::ImageElement::ParseLine(const DataNode &node)
{
	// The "inactive" and "hover" sprite only applies to non-dynamic images.
	// The "colored" tag only applies to outlines.
	if(node.Token(0) == "inactive" && node.Size() >= 2 && name.empty())
		sprite[Element::INACTIVE] = SpriteSet::Get(node.Token(1));
	else if(node.Token(0) == "hover" && node.Size() >= 2 && name.empty())
		sprite[Element::HOVER] = SpriteSet::Get(node.Token(1));
	else if(isOutline && node.Token(0) == "colored")
		isColored = true;
	else
		return false;

	return true;
}



// Report the actual dimensions of the object that will be drawn.
Point Interface::ImageElement::NativeDimensions(const Information &info, int state) const
{
	const Sprite *sprite = GetSprite(info, state);
	if(!sprite || !sprite->Width() || !sprite->Height())
		return Point();

	Point size(sprite->Width(), sprite->Height());
	Rectangle bounds = Bounds();
	if(!bounds.Dimensions())
		return size;

	// If one of the dimensions is zero, it means the sprite's size is not
	// constrained in that dimension.
	double xScale = !bounds.Width() ? 1000. : bounds.Width() / size.X();
	double yScale = !bounds.Height() ? 1000. : bounds.Height() / size.Y();
	return size * min(xScale, yScale);
}



// Draw this element in the given rectangle.
void Interface::ImageElement::Draw(const Rectangle &rect, const Information &info, int state) const
{
	const Sprite *sprite = GetSprite(info, state);
	if(!sprite || !sprite->Width() || !sprite->Height())
		return;

	float frame = info.GetSpriteFrame(name);
	Point unit = info.GetSpriteUnit(name);
	if(isOutline)
	{
		Color color = (isColored ? info.GetOutlineColor() : Color(1.f, 1.f));
		OutlineShader::Draw(sprite, rect.Center(), rect.Dimensions(), color, unit, frame);
	}
	else
		SpriteShader::Draw(sprite, rect.Center(), rect.Width() / sprite->Width(), 0, frame, unit);
}



const Sprite *Interface::ImageElement::GetSprite(const Information &info, int state) const
{
	return name.empty() ? sprite[state] : info.GetSprite(name);
}



// Members of the TextElement class:

// Constructor.
Interface::TextElement::TextElement(const DataNode &node, const Point &globalAnchor)
{
	if(node.Size() < 2)
		return;

	isDynamic = (node.Token(0) == "string" || node.Token(0) == "dynamic button");
	if(node.Token(0) == "button" || node.Token(0) == "dynamic button")
	{
		buttonKey = node.Token(1).front();
		if(node.Size() >= 3)
			str = node.Token(2);
	}
	else
		str = node.Token(1);

	// This function will call ParseLine() for any line it does not recognize.
	Load(node, globalAnchor);

	// Fill in any undefined state colors. By default labels are "medium", strings
	// are "bright", and button brightness depends on its activation state.
	if(!color[Element::ACTIVE] && !buttonKey)
		color[Element::ACTIVE] = GameData::Colors().Get(isDynamic ? "bright" : "medium");

	if(!color[Element::ACTIVE])
	{
		// If no color is specified and this is a button, use the default colors.
		color[Element::ACTIVE] = GameData::Colors().Get("active");
		if(!color[Element::INACTIVE])
			color[Element::INACTIVE] = GameData::Colors().Get("inactive");
		if(!color[Element::HOVER])
			color[Element::HOVER] = GameData::Colors().Get("hover");
	}
	else
	{
		// If a base color was specified, also use it for any unspecified states.
		if(!color[Element::INACTIVE])
			color[Element::INACTIVE] = color[Element::ACTIVE];
		if(!color[Element::HOVER])
			color[Element::HOVER] = color[Element::ACTIVE];
	}
}



// Parse the given data line: one that is not recognized by Element
// itself. This returns false if it does not recognize the line, either.
bool Interface::TextElement::ParseLine(const DataNode &node)
{
	if(node.Token(0) == "size" && node.Size() >= 2)
		fontSize = node.Value(1);
	else if(node.Token(0) == "color" && node.Size() >= 2)
		color[Element::ACTIVE] = GameData::Colors().Get(node.Token(1));
	else if(node.Token(0) == "inactive" && node.Size() >= 2)
		color[Element::INACTIVE] = GameData::Colors().Get(node.Token(1));
	else if(node.Token(0) == "hover" && node.Size() >= 2)
		color[Element::HOVER] = GameData::Colors().Get(node.Token(1));
	else if(node.Token(0) == "truncate" && node.Size() >= 2)
	{
		if(node.Token(1) == "none")
			truncate = Truncate::NONE;
		else if(node.Token(1) == "front")
			truncate = Truncate::FRONT;
		else if(node.Token(1) == "middle")
			truncate = Truncate::MIDDLE;
		else if(node.Token(1) == "back")
			truncate = Truncate::BACK;
		else
			return false;
	}
	else
		return false;

	return true;
}



// Report the actual dimensions of the object that will be drawn.
Point Interface::TextElement::NativeDimensions(const Information &info, int state) const
{
	const Font &font = FontSet::Get(fontSize);
	const auto layout = Layout(static_cast<int>(Bounds().Width() - padding.X()), truncate);
	return Point(font.FormattedWidth({GetString(info), layout}), font.Height());
}



// Draw this element in the given rectangle.
void Interface::TextElement::Draw(const Rectangle &rect, const Information &info, int state) const
{
	// Avoid crashes for malformed interface elements that are not fully loaded.
	if(!color[state])
		return;

	const auto layout = Layout(static_cast<int>(rect.Width()), truncate);
	FontSet::Get(fontSize).Draw({GetString(info), layout}, rect.TopLeft(), *color[state]);
}



// Add any click handlers needed for this element. This will only be
// called if the element is visible and active.
void Interface::TextElement::Place(const Rectangle &bounds, Panel *panel) const
{
	if(buttonKey && panel)
		panel->AddZone(bounds, buttonKey);
}



string Interface::TextElement::GetString(const Information &info) const
{
	return isDynamic ? info.GetString(str) : str;
}



// Members of the BarElement class:

// Constructor.
Interface::BarElement::BarElement(const DataNode &node, const Point &globalAnchor)
{
	if(node.Size() < 2)
		return;

	// Get the name of the element and find out what type it is (bar or ring).
	name = node.Token(1);
	isRing = (node.Token(0) == "ring");

	// This function will call ParseLine() for any line it does not recognize.
	Load(node, globalAnchor);

	// Fill in a default color if none is specified.
	if(!color)
		color = GameData::Colors().Get("active");
}



// Parse the given data line: one that is not recognized by Element
// itself. This returns false if it does not recognize the line, either.
bool Interface::BarElement::ParseLine(const DataNode &node)
{
	if(node.Token(0) == "color" && node.Size() >= 2)
		color = GameData::Colors().Get(node.Token(1));
	else if(node.Token(0) == "map color" && node.Size() >= 2)
		mapColor = node.Value(1);
	else if(node.Token(0) == "size" && node.Size() >= 2)
		width = node.Value(1);
	else if(node.Token(0) == "span angle" && node.Size() >= 2)
		spanAngle = max(0., min(360., node.Value(1)));
	else if(node.Token(0) == "start angle" && node.Size() >= 2)
		startAngle = max(0., min(360., node.Value(1)));
	else if(node.Token(0) == "reversed")
		reversed = true;
	else
		return false;

	return true;
}



// Draw this element in the given rectangle.
void Interface::BarElement::Draw(const Rectangle &rect, const Information &info, int state) const
{
	// Get the current settings for this bar or ring.
	double value = info.BarValue(name);
	double segments = info.BarSegments(name);
	if(segments <= 1.)
		segments = 0.;

	// Avoid crashes for malformed interface elements that are not fully loaded.
	if((!color && std::isnan(mapColor)) || !width || !value)
		return;

	Color trueColor;
	// if(color)
	// 	trueColor = *color;
	// else
		trueColor = MapPanel::MapColor(mapColor);

	if(isRing)
	{
		if(!rect.Width() || !rect.Height())
			return;

		double fraction = value * spanAngle / 360.;
		RingShader::Draw(rect.Center(), .5 * rect.Width(), width, fraction, trueColor, segments, startAngle);
	}
	else
	{
		// Figue out where the line should be drawn from and to.
		// Note: the default start position is the bottom right.
		// If "reversed" was specified, the top left will be used instead.
		Point start = reversed ? rect.TopLeft() : rect.BottomRight();
		Point dimensions = -rect.Dimensions();
		if(reversed)
			dimensions *= -1.;
		double length = dimensions.Length();

		// We will have (segments - 1) gaps between the segments.
		double empty = segments ? (width / length) : 0.;
		double filled = segments ? (1. - empty * (segments - 1.)) / segments : 1.;

		// Draw segments until we've drawn the desired length.
		double v = 0.;
		while(v < value)
		{
			Point from = start + v * dimensions;
			v += filled;
			Point to = start + min(v, value) * dimensions;
			v += empty;

			LineShader::Draw(from, to, width, trueColor);
		}
	}
}



// Members of the PointerElement class:

// Constructor.
Interface::PointerElement::PointerElement(const DataNode &node, const Point &globalAnchor)
{
	Load(node, globalAnchor);

	// Fill in a default color if none is specified.
	if(!color)
		color = GameData::Colors().Get("medium");

	// Set a default orientation if none is specified.
	if(!orientation)
		orientation = Point(0., -1.);
}



// Parse the given data line: one that is not recognized by Element
// itself. This returns false if it does not recognize the line, either.
bool Interface::PointerElement::ParseLine(const DataNode &node)
{
	if(node.Token(0) == "color" && node.Size() >= 2)
		color = GameData::Colors().Get(node.Token(1));
	else if(node.Token(0) == "orientation angle" && node.Size() >= 2)
	{
		const Angle direction(node.Value(1));
		orientation = direction.Unit();
	}
	else if(node.Token(0) == "orientation vector" && node.Size() >= 3)
	{
		orientation.X() = node.Value(1);
		orientation.Y() = node.Value(2);
	}
	else
		return false;

	return true;
}



void Interface::PointerElement::Draw(const Rectangle &rect, const Information &info, int state) const
{
	const Point center = rect.Center();
	const float width = rect.Width();
	const float height = rect.Height();
	PointerShader::Draw(center, orientation, width, height, 0.f, *color);
}



// Members of the LineElement class:

// Constructor.
Interface::LineElement::LineElement(const DataNode &node, const Point &globalAnchor)
{
	// This function will call ParseLine() for any line it does not recognize.
	Load(node, globalAnchor);

	// Fill in a default color if none is specified.
	if(!color)
		color = GameData::Colors().Get("medium");
}



// Parse the given data line: one that is not recognized by Element
// itself. This returns false if it does not recognize the line, either.
bool Interface::LineElement::ParseLine(const DataNode &node)
{
	if(node.Token(0) == "color" && node.Size() >= 2)
		color = GameData::Colors().Get(node.Token(1));
	else
		return false;

	return true;
}



// Draw this element in the given rectangle.
void Interface::LineElement::Draw(const Rectangle &rect, const Information &info, int state) const
{
	// Avoid crashes for malformed interface elements that are not fully loaded.
	if(!from.Get() && !to.Get())
		return;
	FillShader::Fill(rect.Center(), rect.Dimensions(), *color);
}
