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
#include "shader/FillShader.h"
#include "text/Font.h"
#include "text/FontSet.h"
#include "GameData.h"
#include "Information.h"
#include "text/Layout.h"
#include "shader/LineShader.h"
#include "shader/OutlineShader.h"
#include "Panel.h"
#include "shader/PointerShader.h"
#include "Rectangle.h"
#include "shader/RingShader.h"
#include "Screen.h"
#include "image/Sprite.h"
#include "image/SpriteSet.h"
#include "shader/SpriteShader.h"
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
		const string &key = child.Token(0);
		bool hasValue = child.Size() >= 2;
		if(key == "anchor")
			anchor = ParseAlignment(child);
		else if(key == "value" && child.Size() >= 3)
			values[child.Token(1)] = child.Value(2);
		else if((key == "point" || key == "box") && hasValue)
		{
			// This node specifies a named point where custom drawing is done.
			points[child.Token(1)].Load(child, anchor);
		}
		else if(key == "list" && hasValue)
		{
			auto &list = lists[child.Token(1)];
			for(const auto &grand : child)
				list.emplace_back(grand.Value(0));
		}
		else if(key == "visible" || key == "active")
		{
			// This node alters the visibility or activation of future nodes.
			string &str = (key == "visible" ? visibleIf : activeIf);
			if(child.Size() >= 3 && child.Token(1) == "if")
				str = child.Token(2);
			else
				str.clear();
		}
		else
		{
			// Check if this node specifies a known element type.
			if(key == "sprite" || key == "image" || key == "outline")
				elements.push_back(make_unique<ImageElement>(child, anchor));
			else if(key == "label" || key == "string" || key == "button" || key == "dynamic button")
				elements.push_back(make_unique<BasicTextElement>(child, anchor));
			else if(key == "wrapped label" || key == "wrapped string"
					|| key == "wrapped button" || key == "wrapped dynamic button")
				elements.push_back(make_unique<WrappedTextElement>(child, anchor));
			else if(key == "bar" || key == "ring")
				elements.push_back(make_unique<BarElement>(child, anchor));
			else if(key == "pointer")
				elements.push_back(make_unique<PointerElement>(child, anchor));
			else if(key == "fill" || key == "line")
			{
				if(key == "line")
					child.PrintTrace("\"line\" is deprecated, use \"fill\" instead:");
				elements.push_back(make_unique<FillElement>(child, anchor));
			}
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
	for(const unique_ptr<Element> &element : elements)
		element->Draw(info, panel);
}



// Check if a named point exists.
bool Interface::HasPoint(const string &name) const
{
	return points.contains(name);
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
		bool hasValue = child.Size() >= 2;
		if(key == "align" && hasValue)
			alignment = ParseAlignment(child);
		else if(key == "dimensions" && child.Size() >= 3)
			dimensions = Point(child.Value(1), child.Value(2));
		else if(key == "width" && hasValue)
			dimensions.X() = child.Value(1);
		else if(key == "height" && hasValue)
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

	const string &key = node.Token(0);
	// Remember whether this is an outline element.
	isOutline = (key == "outline");
	// If this is a "sprite," look up the sprite with the given name. Otherwise,
	// the sprite path will be dynamically supplied by the Information object.
	if(key == "sprite")
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
	const string &key = node.Token(0);
	bool hasValue = node.Size() >= 2;
	if(key == "inactive" && hasValue && name.empty())
		sprite[Element::INACTIVE] = SpriteSet::Get(node.Token(1));
	else if(key == "hover" && hasValue && name.empty())
		sprite[Element::HOVER] = SpriteSet::Get(node.Token(1));
	else if(isOutline && key == "colored")
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
	{
		const Swizzle *swizzle = info.GetSwizzle(name);
		SpriteShader::Draw(sprite, rect.Center(), rect.Width() / sprite->Width(), swizzle, frame, unit);
	}
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

	const string &key = node.Token(0);
	isDynamic = (key.ends_with("string") || key.ends_with("dynamic button"));
	if(key.ends_with("button") || key.ends_with("dynamic button"))
	{
		buttonKey = node.Token(1).front();
		if(node.Size() >= 3)
			str = node.Token(2);
	}
	else
		str = node.Token(1);
}



// Parse the given data line: one that is not recognized by Element
// itself. This returns false if it does not recognize the line, either.
bool Interface::TextElement::ParseLine(const DataNode &node)
{
	const string &key = node.Token(0);
	bool hasValue = node.Size() >= 2;
	if(key == "size" && hasValue)
		fontSize = node.Value(1);
	else if(key == "color" && hasValue)
		color[Element::ACTIVE] = GameData::Colors().Get(node.Token(1));
	else if(key == "inactive" && hasValue)
		color[Element::INACTIVE] = GameData::Colors().Get(node.Token(1));
	else if(key == "hover" && hasValue)
		color[Element::HOVER] = GameData::Colors().Get(node.Token(1));
	else if(key == "truncate" && hasValue)
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



// Add any click handlers needed for this element. This will only be
// called if the element is visible and active.
void Interface::TextElement::Place(const Rectangle &bounds, Panel *panel) const
{
	if(buttonKey && panel)
		panel->AddZone(bounds, buttonKey);
}



// Fill in any undefined state colors.
void Interface::TextElement::FinishLoadingColors()
{
	// By default, labels are "medium", strings
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



// Get text contents of this element.
string Interface::TextElement::GetString(const Information &info) const
{
	return isDynamic ? info.GetString(str) : str;
}



// Members of the BasicTextElement class:

// Constructor.
Interface::BasicTextElement::BasicTextElement(const DataNode &node, const Point &globalAnchor)
	: TextElement(node, globalAnchor)
{
	// This function will call ParseLine() for any line it does not recognize.
	Load(node, globalAnchor);

	FinishLoadingColors();
}



// Report the actual dimensions of the object that will be drawn.
Point Interface::BasicTextElement::NativeDimensions(const Information &info, int state) const
{
	const Font &font = FontSet::Get(fontSize);
	const auto layout = Layout(static_cast<int>(Bounds().Width() - padding.X()), truncate);
	return Point(font.FormattedWidth({GetString(info), layout}), font.Height());
}



// Draw this element in the given rectangle.
void Interface::BasicTextElement::Draw(const Rectangle &rect, const Information &info, int state) const
{
	// Avoid crashes for malformed interface elements that are not fully loaded.
	if(!color[state])
		return;

	const auto layout = Layout(static_cast<int>(rect.Width()), truncate);
	FontSet::Get(fontSize).Draw({GetString(info), layout}, rect.TopLeft(), *color[state]);
}



// Members of the WrappedElement class:

// Constructor.
Interface::WrappedTextElement::WrappedTextElement(const DataNode &node, const Point &globalAnchor)
	: TextElement(node, globalAnchor)
{
	// This function will call ParseLine() for any line it does not recognize.
	Load(node, globalAnchor);

	FinishLoadingColors();

	// Initialize the WrappedText.
	text.SetAlignment(textAlignment);
	text.SetTruncate(truncate);
	text.SetWrapWidth(Bounds().Width());
}



// Parse the given data line: one that is not recognized by Element
// itself. This returns false if it does not recognize the line, either.
bool Interface::WrappedTextElement::ParseLine(const DataNode &node)
{
	if(TextElement::ParseLine(node))
		return true;
	if(node.Token(0) == "alignment")
	{
		const string &value = node.Token(1);
		if(value == "left")
			textAlignment = Alignment::LEFT;
		else if(value == "center")
			textAlignment = Alignment::CENTER;
		else if(value == "right")
			textAlignment = Alignment::RIGHT;
		else if(value == "justified")
			textAlignment = Alignment::JUSTIFIED;
		else
			return false;
	}
	else
		return false;

	return true;
}



// Report the actual dimensions of the object that will be drawn.
Point Interface::WrappedTextElement::NativeDimensions(const Information &info, int state) const
{
	text.SetFont(FontSet::Get(fontSize));
	text.Wrap(GetString(info));
	return Point(text.WrapWidth(), text.Height());
}



// Draw this element in the given rectangle.
void Interface::WrappedTextElement::Draw(const Rectangle &rect, const Information &info, int state) const
{
	// The text has already been wrapped in NativeDimensions called by Element::Draw.
	text.Draw(rect.TopLeft(), *color[state]);
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
	if(!fromColor)
		fromColor = toColor = GameData::Colors().Get("active");
}



// Parse the given data line: one that is not recognized by Element
// itself. This returns false if it does not recognize the line, either.
bool Interface::BarElement::ParseLine(const DataNode &node)
{
	const string &key = node.Token(0);
	bool hasValue = node.Size() >= 2;
	if(key == "color" && hasValue)
	{
		fromColor = GameData::Colors().Get(node.Token(1));
		toColor = node.Size() >= 3 ? GameData::Colors().Get(node.Token(2)) : fromColor;
	}
	else if(key == "size" && hasValue)
		width = node.Value(1);
	else if(key == "span angle" && hasValue)
		spanAngle = max(0., min(360., node.Value(1)));
	else if(key == "start angle" && hasValue)
		startAngle = max(0., min(360., node.Value(1)));
	else if(key == "reversed")
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
	if(!fromColor || !toColor || !width || !value)
		return;

	if(isRing)
	{
		if(!rect.Width() || !rect.Height())
			return;


		double fraction = value * spanAngle / 360.;
		RingShader::Draw(rect.Center(), .5 * rect.Width(), width, fraction, *fromColor, segments, startAngle);
	}
	else
	{
		// Figure out where the line should be drawn from and to.
		// Note: the default start position is the bottom right.
		// If "reversed" was specified, the top left will be used instead.
		Point start = reversed ? rect.TopLeft() : rect.BottomRight();
		Point dimensions = -rect.Dimensions();
		if(reversed)
			dimensions *= -1.;
		double length = dimensions.Length();
		Point unit = dimensions.Unit();

		// We will have (segments - 1) gaps between the segments.
		double empty = segments ? (width / length) : 0.;
		double filled = segments ? (1. - empty * (segments - 1.)) / segments : 1.;

		// Draw segments until we've drawn the desired length.
		double v = 0.;
		while(v < value)
		{
			Color nFromColor = Color::Combine(1 - v, *fromColor, v, *toColor);
			Point from = start + v * dimensions;
			v += filled;
			double lim = min(v, value);
			Point to = start + lim * dimensions;
			Color nToColor = Color::Combine(1 - lim, *fromColor, lim, *toColor);
			v += empty;

			// Rounded lines have a bit of padding, so account for that here.
			float d = (to - from).Length() / 2.;
			float twidth = d < width ? width * d / 2. : width;
			from += unit * twidth;
			to -= unit * twidth;

			LineShader::DrawGradient(from, to, twidth, nFromColor, nToColor);
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
	const string &key = node.Token(0);
	bool hasValue = node.Size() >= 2;
	if(key == "color" && hasValue)
		color = GameData::Colors().Get(node.Token(1));
	else if(key == "orientation angle" && hasValue)
	{
		const Angle direction(node.Value(1));
		orientation = direction.Unit();
	}
	else if(key == "orientation vector" && node.Size() >= 3)
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



// Members of the FillElement class:

// Constructor.
Interface::FillElement::FillElement(const DataNode &node, const Point &globalAnchor)
{
	// This function will call ParseLine() for any line it does not recognize.
	Load(node, globalAnchor);

	// Fill in a default color if none is specified.
	if(!color)
		color = GameData::Colors().Get("medium");
}



// Parse the given data line: one that is not recognized by Element
// itself. This returns false if it does not recognize the line, either.
bool Interface::FillElement::ParseLine(const DataNode &node)
{
	if(node.Token(0) == "color" && node.Size() >= 2)
		color = GameData::Colors().Get(node.Token(1));
	else
		return false;

	return true;
}



// Draw this element in the given rectangle.
void Interface::FillElement::Draw(const Rectangle &rect, const Information &info, int state) const
{
	// Avoid crashes for malformed interface elements that are not fully loaded.
	if(!from.Get() && !to.Get())
		return;
	FillShader::Fill(rect, *color);
}
