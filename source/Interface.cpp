/* Interface.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "Interface.h"

#include "DataNode.h"
#include "Font.h"
#include "FontSet.h"
#include "GameData.h"
#include "Information.h"
#include "LineShader.h"
#include "OutlineShader.h"
#include "Panel.h"
#include "Rectangle.h"
#include "RingShader.h"
#include "Screen.h"
#include "Sprite.h"
#include "SpriteSet.h"
#include "SpriteShader.h"

#include <algorithm>
#include <cmath>

using namespace std;



void Interface::Load(const DataNode &node)
{
	position = Point();
	sprites.clear();
	outlines.clear();
	labels.clear();
	
	string condition;
	
	for(const DataNode &child : node)
	{
		const string &key = child.Token(0);
		if(key == "if" && child.Size() >= 2)
			condition = child.Token(1);
		else if(key == "if" || key == "endif")
			condition.clear();
		else if(key == "position")
		{
			for(int i = 1; i < child.Size(); ++i)
			{
				const string &token = child.Token(i);
				if(token == "left")
					position += Point(-.5, 0.);
				else if(token == "top")
					position += Point(0., -.5);
				else if(token == "right")
					position += Point(.5, 0.);
				else if(token == "bottom")
					position += Point(0., .5);
				else if(token != "center")
					child.PrintTrace("Skipping unrecognized attribute:");
			}
		}
		else if((key == "sprite" || key == "outline") && child.Size() >= 4)
		{
			vector<SpriteSpec> &vec = (key == "sprite") ? sprites : outlines;
			
			Point position(child.Value(2), child.Value(3));
			if(child.Size() == 4 || child.Token(4) != "dynamic")
				vec.emplace_back(SpriteSet::Get(child.Token(1)), position);
			else
				vec.emplace_back(child.Token(1), position);
			
			for(const DataNode &grand : child)
			{
				if(grand.Token(0) == "size" && grand.Size() >= 3)
					vec.back().size = Point(
						grand.Value(1), grand.Value(2));
				else if(grand.Token(0) == "colored")
					vec.back().isColored = true;
				else
					grand.PrintTrace("Skipping unrecognized attribute:");
			}
			
			vec.back().condition = condition;
		}
		else if((key == "label" || key == "string") && child.Size() >= 4)
		{
			vector<StringSpec> &vec = ((key == "label") ? labels : strings);
			
			Point position(child.Value(2), child.Value(3));
			vec.emplace_back(child.Token(1), position);
			
			for(const DataNode &grand : child)
			{
				if(grand.Token(0) == "color" && grand.Size() >= 2)
					vec.back().color = GameData::Colors().Get(grand.Token(1));
				else if(grand.Token(0) == "align" && grand.Size() >= 2)
					vec.back().align =
						(grand.Token(1) == "center") ? .5 :
						(grand.Token(1) == "right") ? 1. : 0.;
				else if(grand.Token(0) == "size" && grand.Size() >= 2)
					vec.back().size =
						static_cast<int>(grand.Value(1));
				else
					grand.PrintTrace("Skipping unrecognized attribute:");
			}
			
			vec.back().condition = condition;
		}
		else if((key == "bar" || key == "ring") && child.Size() >= 4)
		{
			vector<BarSpec> &vec = ((key == "bar") ? bars : rings);
			
			Point position(child.Value(2), child.Value(3));
			vec.emplace_back(child.Token(1), position);
			
			for(const DataNode &grand : child)
			{
				if(grand.Token(0) == "color" && grand.Size() >= 2)
					vec.back().color = GameData::Colors().Get(grand.Token(1));
				else if(grand.Token(0) == "size" && grand.Size() >= 3)
					vec.back().size = Point(
						grand.Value(1), grand.Value(2));
				else if(grand.Token(0) == "width" && grand.Size() >= 2)
					vec.back().width =
						static_cast<float>(grand.Value(1));
				else
					grand.PrintTrace("Skipping unrecognized attribute:");
			}
			
			vec.back().condition = condition;
		}
		else if(key == "button" && child.Size() >= 4)
		{
			Point position(child.Value(2), child.Value(3));
			buttons.emplace_back(child.Token(1).front(), position);
			
			for(const DataNode &grand : child)
			{
				if(grand.Token(0) == "size" && grand.Size() >= 3)
					buttons.back().size = Point(
						grand.Value(1), grand.Value(2));
				else
					grand.PrintTrace("Skipping unrecognized attribute:");
			}
		}
		else if(key == "point" && child.Size() >= 2)
		{
			PointSpec &spec = points[child.Token(1)];
			for(const DataNode &grand : child)
			{
				if(grand.Token(0) == "position" && grand.Size() >= 3)
					spec.position = Point(grand.Value(1), grand.Value(2));
				else if(grand.Token(0) == "size" && grand.Size() >= 3)
					spec.size = Point(grand.Value(1), grand.Value(2));
				else
					grand.PrintTrace("Skipping unrecognized attribute:");
			}
		}
		else
			child.PrintTrace("Skipping unrecognized attribute:");
	}
}



void Interface::Draw(const Information &info, Panel *panel) const
{
	Point corner(Screen::Width() * position.X(), Screen::Height() * position.Y());
	
	for(const SpriteSpec &sprite : sprites)
	{
		if(!info.HasCondition(sprite.condition))
			continue;
		
		const Sprite *s = sprite.sprite;
		if(!s)
			s = info.GetSprite(sprite.name);
		if(!s)
			continue;
		
		Point offset(
			s->Width() * position.X(),
			s->Height() * position.Y());
		
		double zoom = 1.;
		if(sprite.size.X() && sprite.size.Y())
			zoom = min(1., min(sprite.size.X() / s->Width(), sprite.size.Y() / s->Height()));
		
		SpriteShader::Draw(s, sprite.position + corner - offset, zoom);
	}
	for(const SpriteSpec &outline : outlines)
	{
		if(!info.HasCondition(outline.condition))
			continue;
		
		const Sprite *s = outline.sprite;
		if(!s)
			s = info.GetSprite(outline.name);
		if(!s)
			continue;
		
		Point size(s->Width(), s->Height());
		if(outline.size.X() && outline.size.Y())
			size *= min(outline.size.X() / s->Width(), outline.size.Y() / s->Height());
		
		Point pos = outline.position + corner - outline.size * position;
		OutlineShader::Draw(s, pos, size,
			outline.isColored ? info.GetOutlineColor() : Color(1., 1.),
			info.GetSpriteUnit(outline.name));
	}
	
	double defaultAlign = position.X() + .5;
	for(const StringSpec &spec : labels)
	{
		if(!info.HasCondition(spec.condition) || !spec.color)
			continue;
		
		const string &str = spec.str;
		
		const Font &font = FontSet::Get(spec.size);
		double a = (spec.align >= 0.) ? spec.align : defaultAlign;
		Point align(font.Width(str) * a, 0.);
		font.Draw(str, corner - align + spec.position, *spec.color);
	}
	for(const StringSpec &spec : strings)
	{
		if(!info.HasCondition(spec.condition) || !spec.color)
			continue;
		
		const string &str = info.GetString(spec.str);
		
		const Font &font = FontSet::Get(spec.size);
		double a = (spec.align >= 0.) ? spec.align : defaultAlign;
		Point align(font.Width(str) * a, 0.);
		font.Draw(str, corner - align + spec.position, *spec.color);
	}
	
	for(const BarSpec &spec : bars)
	{
		if(!info.HasCondition(spec.condition) || !spec.color)
			continue;
		
		double length = spec.size.Length();
		if(!length || !spec.width)
			continue;
		
		double value = info.BarValue(spec.name);
		double segments = info.BarSegments(spec.name);
		if(!value)
			continue;
		
		// We will have (segments - 1) gaps between the segments.
		double empty = segments ? (spec.width / length) : 0.;
		double filled = segments ? (1. - empty * (segments - 1.)) / segments : 1.;
		
		double v = 0.;
		Point start = spec.position + corner;
		while(v < value)
		{
			Point from = start + v * spec.size;
			v += filled;
			Point to = start + min(v, value) * spec.size;
			v += empty;
			
			LineShader::Draw(from, to, spec.width, *spec.color);
		}
	}
	for(const BarSpec &spec : rings)
	{
		if(!info.HasCondition(spec.condition) || !spec.color)
			continue;
		
		if(!spec.size.X() || !spec.size.Y() || !spec.width)
			continue;
		
		double value = info.BarValue(spec.name);
		double segments = info.BarSegments(spec.name);
		if(!value)
			continue;
		if(segments <= 1.)
			segments = 0.;
		
		Point center = spec.position + corner - spec.size * position;
		RingShader::Draw(center, .5 * spec.size.X(), spec.width, value, *spec.color, segments);
	}
	if(panel)
		for(const ButtonSpec &button : buttons)
		{
			Point offset(
				button.size.X() * position.X(),
				button.size.Y() * position.Y());
		
			panel->AddZone(Rectangle(button.position + corner - offset, button.size), button.key);
		}
}



bool Interface::HasPoint(const string &name) const
{
	return points.count(name);
}



Point Interface::GetPoint(const string &name) const
{
	Point corner(Screen::Width() * position.X(), Screen::Height() * position.Y());
	auto it = points.find(name);
	return corner + (it == points.end() ? Point() : it->second.position);
}



Point Interface::GetSize(const string &name) const
{
	auto it = points.find(name);
	return (it == points.end() ? Point() : it->second.size);
}



Interface::SpriteSpec::SpriteSpec(const string &str, const Point &position)
	: name(str), sprite(nullptr), position(position), isColored(false)
{
}



Interface::SpriteSpec::SpriteSpec(const Sprite *sprite, const Point &position)
	: sprite(sprite), position(position), isColored(false)
{
}



Interface::StringSpec::StringSpec(const string &str, const Point &position)
	: str(str), position(position), align(-1.), size(14)
{
}



Interface::BarSpec::BarSpec(const string &name, const Point &position)
	: name(name), position(position), width(0.0)
{
}



Interface::ButtonSpec::ButtonSpec(char key, const Point &position)
	: position(position), key(key)
{
}
