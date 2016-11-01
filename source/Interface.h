/* Interface.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef INTERFACE_H_
#define INTERFACE_H_

#include "Color.h"
#include "Point.h"

#include <map>
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
	
	bool HasPoint(const std::string &name) const;
	Point GetPoint(const std::string &name) const;
	Point GetSize(const std::string &name) const;
	
	
private:
	class SpriteSpec {
	public:
		SpriteSpec(const std::string &str, const Point &position);
		SpriteSpec(const Sprite *sprite, const Point &position);
		
		std::string name;
		const Sprite *sprite;
		Point position;
		Point size;
		bool isColored;
		
		std::string condition;
	};
	
	class StringSpec {
	public:
		StringSpec(const std::string &str, const Point &position);
		
		std::string str;
		Point position;
		double align;
		int size;
		const Color *color = nullptr;
		
		std::string condition;
	};
	
	class BarSpec {
	public:
		BarSpec(const std::string &name, const Point &position);
		
		std::string name;
		Point position;
		Point size;
		const Color *color = nullptr;
		float width;
		
		std::string condition;
	};
	
	class ButtonSpec {
	public:
		ButtonSpec(char key, const Point &position);
		
		Point position;
		Point size;
		char key;
	};
	
	class PointSpec {
	public:
		Point position;
		Point size;
	};
	
	
private:
	Point position;
	
	std::vector<SpriteSpec> sprites;
	std::vector<SpriteSpec> outlines;
	
	std::vector<StringSpec> labels;
	std::vector<StringSpec> strings;
	
	std::vector<BarSpec> bars;
	std::vector<BarSpec> rings;
	
	std::vector<ButtonSpec> buttons;
	
	std::map<std::string, PointSpec> points;
};



#endif
