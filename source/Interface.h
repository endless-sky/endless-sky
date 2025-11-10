/* Interface.h
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

#pragma once

#include "text/Alignment.h"
#include "Color.h"
#include "Point.h"
#include "Rectangle.h"
#include "text/Truncate.h"
#include "text/WrappedText.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

class DataNode;
class Information;
class Panel;
class Sprite;



// Class representing a user interface, specified in a data file and filled with
// the contents of an Information object.
class Interface {
public:
	void Load(const DataNode &node);

	// Draw this interface. If the given panel is not null, also register any
	// buttons in this interface with the panel's list of clickable zones.
	void Draw(const Information &info, Panel *panel = nullptr) const;

	// Get the location of a named point or box.
	bool HasPoint(const std::string &name) const;
	Point GetPoint(const std::string &name) const;
	Rectangle GetBox(const std::string &name) const;

	// Get a named value.
	double GetValue(const std::string &name) const;

	// Get a named list.
	const std::vector<double> &GetList(const std::string &name) const;


private:
	class AnchoredPoint {
	public:
		// Get the point's location, given the current screen dimensions.
		Point Get() const;
		// Get the point's location, treating the Region within the Information as the screen area.
		Point Get(const Information &info) const;
		void Set(const Point &position, const Point &anchor);

	private:
		Point position;
		Point anchor;
	};

	class Element {
	public:
		// State enumeration:
		static const int INACTIVE = 0;
		static const int ACTIVE = 1;
		static const int HOVER = 2;

	public:
		// Make sure the destructor is virtual, because classes derived from
		// this one will be used in a polymorphic list.
		Element() = default;
		virtual ~Element() = default;

		// Create a new element. The alignment of the interface that contains
		// this element is used to calculate the element's position.
		void Load(const DataNode &node, const Point &globalAnchor);

		// Draw this element, relative to the given anchor point. If this is a
		// button, it will add a clickable zone to the given panel.
		void Draw(const Information &info, Panel *panel) const;

		// Set the conditions that control when this element is visible and active.
		// An empty string means it is always visible or active.
		void SetConditions(const std::string &visible, const std::string &active);

		// Get the bounding rectangle, given the current screen dimensions.
		Rectangle Bounds() const;
		// Get the bounding rectangle, treating the Region within the Information as the screen area.
		Rectangle Bounds(const Information &info) const;

	protected:
		// Parse the given data line: one that is not recognized by Element
		// itself. This returns false if it does not recognize the line, either.
		virtual bool ParseLine(const DataNode &node);
		// Report the actual dimensions of the object that will be drawn.
		virtual Point NativeDimensions(const Information &info, int state) const;
		// Draw this element in the given rectangle.
		virtual void Draw(const Rectangle &rect, const Information &info, int state) const;
		// Add any click handlers needed for this element. This will only be
		// called if the element is visible and active.
		virtual void Place(const Rectangle &bounds, Panel *panel) const;

	protected:
		AnchoredPoint from;
		AnchoredPoint to;
		Point alignment;
		Point padding;
		std::string visibleIf;
		std::string activeIf;
	};

	// This class handles "sprite", "image", and "outline" elements.
	class ImageElement : public Element {
	public:
		ImageElement(const DataNode &node, const Point &globalAnchor);

	protected:
		// Parse the given data line: one that is not recognized by Element
		// itself. This returns false if it does not recognize the line, either.
		virtual bool ParseLine(const DataNode &node) override;
		// Report the actual dimensions of the object that will be drawn.
		virtual Point NativeDimensions(const Information &info, int state) const override;
		// Draw this element in the given rectangle.
		virtual void Draw(const Rectangle &rect, const Information &info, int state) const override;

	private:
		const Sprite *GetSprite(const Information &info, int state) const;

	private:
		// If a name is given, look up the sprite with that name and draw it.
		std::string name;
		// Otherwise, draw a sprite. Which sprite is drawn depends on the current
		// state of this element: inactive, active, or hover.
		const Sprite *sprite[3] = {nullptr, nullptr, nullptr};
		// If this flag is set, draw the sprite as an outline:
		bool isOutline = false;
		// Store whether the outline should be colored.
		bool isColored = false;
	};

	// This class contains common members of both text element categories.
	class TextElement : public Element {
	public:
		TextElement(const DataNode &node, const Point &globalAnchor);

	protected:
		// Parse the given data line: one that is not recognized by Element
		// itself. This returns false if it does not recognize the line, either.
		virtual bool ParseLine(const DataNode &node) override;
		// Add any click handlers needed for this element. This will only be
		// called if the element is visible and active.
		virtual void Place(const Rectangle &bounds, Panel *panel) const override;

		// Fill in any undefined state colors.
		void FinishLoadingColors();
		// Get text contents of this element.
		std::string GetString(const Information &info) const;

	protected:
		// The string may either be a name of a dynamic string, or static text.
		std::string str;
		// Color for inactive, active, and hover states.
		const Color *color[3] = {nullptr, nullptr, nullptr};
		int fontSize = 14;
		char buttonKey = '\0';
		bool isDynamic = false;
		Truncate truncate = Truncate::NONE;
	};

	// This class handles "label", "string", "button", and "dynamic button" elements.
	class BasicTextElement : public TextElement {
	public:
		BasicTextElement(const DataNode &node, const Point &globalAnchor);

	protected:
		// Report the actual dimensions of the object that will be drawn.
		virtual Point NativeDimensions(const Information &info, int state) const override;
		// Draw this element in the given rectangle.
		virtual void Draw(const Rectangle &rect, const Information &info, int state) const override;
	};

	// This class handles "wrapped label", "wrapped string",
	// "wrapped button", and "wrapped dynamic button" elements.
	class WrappedTextElement : public TextElement {
	public:
		WrappedTextElement(const DataNode &node, const Point &globalAnchor);

	protected:
		// Parse the given data line: one that is not recognized by Element
		// itself. This returns false if it does not recognize the line, either.
		virtual bool ParseLine(const DataNode &node) override;
		// Report the actual dimensions of the object that will be drawn.
		virtual Point NativeDimensions(const Information &info, int state) const override;
		// Draw this element in the given rectangle.
		virtual void Draw(const Rectangle &rect, const Information &info, int state) const override;

	private:
		mutable WrappedText text;
		Alignment textAlignment = Alignment::LEFT;
	};

	// This class handles "bar" and "ring" elements.
	class BarElement : public Element {
	public:
		BarElement(const DataNode &node, const Point &globalAnchor);

	protected:
		// Parse the given data line: one that is not recognized by Element
		// itself. This returns false if it does not recognize the line, either.
		virtual bool ParseLine(const DataNode &node) override;
		// Draw this element in the given rectangle.
		virtual void Draw(const Rectangle &rect, const Information &info, int state) const override;

	private:
		std::string name;
		const Color *fromColor = nullptr;
		const Color *toColor = nullptr;
		float width = 2.f;
		bool reversed = false;
		bool isRing = false;
		double spanAngle = 360.;
		double startAngle = 0.;
	};


	// This class handles "pointer" elements.
	class PointerElement : public Element {
	public:
		PointerElement(const DataNode &node, const Point &globalAnchor);

	protected:
		// Parse the given data line: one that is not recognized by Element
		// itself. This returns false if it does not recognize the line, either.
		virtual bool ParseLine(const DataNode &node) override;
		// Draw this element in the given rectangle.
		virtual void Draw(const Rectangle &rect, const Information &info, int state) const override;

	private:
		const Color *color = nullptr;
		Point orientation;
	};


	// This class handles "fill" elements.
	class FillElement : public Element {
	public:
		FillElement(const DataNode &node, const Point &globalAnchor);

	protected:
		// Parse the given data line: one that is not recognized by Element
		// itself. This returns false if it does not recognize the line, either.
		virtual bool ParseLine(const DataNode &node) override;
		// Draw this element in the given rectangle.
		virtual void Draw(const Rectangle &rect, const Information &info, int state) const override;

	private:
		const Color *color = nullptr;
	};


private:
	std::vector<std::unique_ptr<Element>> elements;
	std::map<std::string, Element> points;
	std::map<std::string, double> values;
	std::map<std::string, std::vector<double>> lists;
};
