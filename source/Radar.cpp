/* Radar.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "Radar.h"

#include "PointerShader.h"
#include "RingShader.h"

using namespace std;

const int Radar::PLAYER = 0;
const int Radar::FRIENDLY = 1;
const int Radar::UNFRIENDLY = 2;
const int Radar::HOSTILE = 3;
const int Radar::INACTIVE = 4;
const int Radar::SPECIAL = 5;
const int Radar::ANOMALOUS = 6;
static const int SIZE = 7;

namespace {
	// Colors.
	static const Color color[SIZE] = {
		Color(.2, 1., 0., 0.), // PLAYER: green
		Color(.4, .6, 1., 0.), // FRIENDLY: blue
		Color(.8, .8, .4, 0.), // UNFRIENDLY: yellow
		Color(1., .6, .4, 0.), // HOSTILE: red
		Color(.4, .4, .4, 0.), // INACTIVE: grey
		Color(1., 1., 1., 0.),  // SPECIAL: white
		Color(.7, 0., 1., 0.)  // ANOMALOUS: magenta
	};
}



void Radar::Clear()
{
	objects.clear();
	pointers.clear();
}



void Radar::SetCenter(const Point &center)
{
	this->center = center;
}



// Add an object. If "inner" is 0 it is a dot; otherwise, it is a ring. The
// given position should be in world units (not shrunk to radar units).
void Radar::Add(int type, Point position, double outer, double inner)
{
	if(type < 0 || type >= SIZE)
		return;
	
	objects.emplace_back(color[type].Opaque(), position - center, outer, inner);
}



// Add a pointer, pointing in the direction of the given vector.
void Radar::AddPointer(int type, const Point &position)
{
	if(type < 0 || type >= SIZE)
		return;
	
	pointers.emplace_back(color[type], position.Unit());
}



// Draw the radar display at the given coordinates.
void Radar::Draw(const Point &center, double scale, double radius, double pointerRadius) const
{
	RingShader::Bind();
	for(const Object &object : objects)
	{
		Point position = object.position * scale;
		double length = position.Length();
		if(length > radius)
			position *= radius / length;
		position += center;
		
		RingShader::Add(position, object.outer, object.inner, object.color);
	}
	RingShader::Unbind();
	
	PointerShader::Bind();
	for(const Pointer &pointer : pointers)
		PointerShader::Add(center, pointer.unit, 10., 10., pointerRadius, pointer.color);
	PointerShader::Unbind();
}



const Color &Radar::GetColor(int type)
{
	if(type < 0 || type >= SIZE)
		type = INACTIVE;
	
	return color[type];
}



Radar::Object::Object(const Color &color, const Point &pos, double out, double in)
	: color(color), position(pos), outer(out), inner(in)
{
}



Radar::Pointer::Pointer(const Color &color, const Point &unit)
	: color(color), unit(unit)
{
}
