/* Information.h
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

#include "Color.h"
#include "Point.h"
#include "Rectangle.h"
#include "Swizzle.h"

#include <map>
#include <set>
#include <string>

class Sprite;



// Class representing information to be displayed in a user interface, independent
// of how that information is laid out or shown.
class Information {
public:
	void SetRegion(const Rectangle &rect);
	const Rectangle &GetCustomRegion() const;
	bool HasCustomRegion() const;

	void SetSprite(const std::string &name, const Sprite *sprite, const Point &unit = Point(0., -1.), float frame = 0.f,
		const Swizzle *swizzle = Swizzle::None());
	const Sprite *GetSprite(const std::string &name) const;
	const Point &GetSpriteUnit(const std::string &name) const;
	float GetSpriteFrame(const std::string &name) const;
	const Swizzle *GetSwizzle(const std::string &name) const;

	void SetString(const std::string &name, const std::string &value);
	const std::string &GetString(const std::string &name) const;

	void SetBar(const std::string &name, double value, double segments = 0.);
	double BarValue(const std::string &name) const;
	double BarSegments(const std::string &name) const;

	void SetCondition(const std::string &condition);
	bool HasCondition(const std::string &condition) const;

	void SetOutlineColor(const Color &color);
	const Color &GetOutlineColor() const;


private:
	Rectangle region;
	bool hasCustomRegion = false;

	std::map<std::string, const Sprite *> sprites;
	std::map<std::string, Point> spriteUnits;
	std::map<std::string, float> spriteFrames;
	std::map<std::string, const Swizzle *> spriteSwizzles;
	std::map<std::string, std::string> strings;
	std::map<std::string, double> bars;
	std::map<std::string, double> barSegments;

	std::set<std::string> conditions;

	Color outlineColor;
};
