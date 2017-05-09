/* Information.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef INFORMATION_H_
#define INFORMATION_H_

#include "Color.h"
#include "Point.h"

#include <map>
#include <set>
#include <string>

class Sprite;



// Class representing information to be displayed in a user interface, independent
// of how that information is laid out or shown.
class Information {
public:
	void SetSprite(const std::string &name, const Sprite *sprite, const Point &unit = Point(0., -1.), int frame = 0);
	const Sprite *GetSprite(const std::string &name) const;
	const Point &GetSpriteUnit(const std::string &name) const;
	int GetSpriteFrame(const std::string &namme) const;
	
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
	std::map<std::string, const Sprite *> sprites;
	std::map<std::string, Point> spriteUnits;
	std::map<std::string, int> spriteFrames;
	std::map<std::string, std::string> strings;
	std::map<std::string, double> bars;
	std::map<std::string, double> barSegments;
	
	std::set<std::string> conditions;
	
	Color outlineColor;
};



#endif
