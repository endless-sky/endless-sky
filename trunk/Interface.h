/* Interface.h
Michael Zahniser, 24 Oct 2013

Class representing a user interface, specified in a data file and filled with
the contents of an Information object.
*/

#ifndef INTERFACE_H_INCLUDED
#define INTERFACE_H_INCLUDED

#include "Color.h"
#include "DataFile.h"
#include "Set.h"
#include "Sprite.h"
#include "Point.h"
#include "Information.h"

#include <string>
#include <vector>



class Interface {
public:
	void Load(const DataFile::Node &node, const Set<Color> &colors);
	
	void Draw(const Information &info) const;
	
	char OnClick(const Point &point) const;
	
	
private:
	class SpriteSpec {
	public:
		SpriteSpec(const std::string &str, const Point &position);
		SpriteSpec(const Sprite *sprite, const Point &position);
		
		std::string name;
		const Sprite *sprite;
		Point position;
		Point size;
		
		std::string condition;
	};
	
	class StringSpec {
	public:
		StringSpec(const std::string &str, const Point &position);
		
		std::string str;
		Point position;
		double align;
		int size;
		const Color *color;
		
		std::string condition;
	};
	
	class BarSpec {
	public:
		BarSpec(const std::string &name, const Point &position);
		
		std::string name;
		Point position;
		Point size;
		const Color *color;
		float width;
		
		std::string condition;
	};
	
	class ButtonSpec {
	public:
		ButtonSpec(char key, const Point &position);
		
		Point position;
		Point size;
		char key;
		
		std::string condition;
	};
	
	class RadarSpec {
	public:
		RadarSpec(const Point &position = Point());
		
		Point position;
		double scale;
		double radius;
		double pointerRadius;
		
		std::string condition;
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
	
	std::vector<RadarSpec> radars;
};



#endif
