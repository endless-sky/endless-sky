/* BatchDrawList.h
Copyright (c) 2017 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "BatchDrawList.h"

#include "BatchShader.h"
#include "Body.h"
#include "Screen.h"
#include "Sprite.h"

#include <cmath>

using namespace std;

namespace {
	void Push(vector<float> &v, const Point &pos, float s, float t, float frame)
	{
		v.push_back(pos.X());
		v.push_back(pos.Y());
		v.push_back(s);
		v.push_back(t);
		v.push_back(frame);
	}
}



// Clear the list, also setting the global time step for animation.
void BatchDrawList::Clear(int step, double zoom)
{
	data.clear();
	this->step = step;
	this->zoom = zoom;
	isHighDPI = (Screen::IsHighResolution() ? zoom > .5 : zoom > 1.);
}



void BatchDrawList::SetCenter(const Point &center)
{
	this->center = center;
}



// Add an object based on the Body class.
bool BatchDrawList::Add(const Body &body, double clip)
{
	Point position = (body.Position() + .5 * body.Velocity() - center) * zoom;
	if(Cull(body, position))
		return false;
	
	// Get the data vector for this particular sprite.
	vector<float> &v = data[body.GetSprite()];
	// The sprite frame is the same for every vertex.
	float frame = body.GetFrame(step);
	
	// Get unit vectors in the direction of the object's width and height.
	Point unit = body.Unit() * zoom;
	Point uw = Point(unit.Y(), -unit.X()) * body.Width();
	Point uh = unit * body.Height();
	
	// Get the "bottom" corner, the one that won't be clipped.
	Point topLeft = position - (uw + uh);
	// Scale the vectors and apply clipping to the "height" of the sprite.
	uw *= 2.;
	uh *= 2. * clip;
	
	// Calculate the other three corners.
	Point topRight = topLeft + uw;
	Point bottomLeft = topLeft + uh;
	Point bottomRight = bottomLeft + uw;
	
	// Push two copies of the first and last vertices to mark the break between
	// the sprites.
	Push(v, topLeft, 0.f, 1.f, frame);
	Push(v, topLeft, 0.f, 1.f, frame);
	Push(v, topRight, 1.f, 1.f, frame);
	Push(v, bottomLeft, 0.f, 1.f - clip, frame);
	Push(v, bottomRight, 1.f, 1.f - clip, frame);
	Push(v, bottomRight, 1.f, 1.f - clip, frame);
	
	return true;
}



// Draw all the items in this list.
void BatchDrawList::Draw() const
{
	BatchShader::Bind();
	
	for(const pair<const Sprite *, vector<float>> &it : data)
		BatchShader::Add(it.first, isHighDPI, it.second);
	
	BatchShader::Unbind();
}



bool BatchDrawList::Cull(const Body &body, const Point &position) const
{
	if(!body.HasSprite() || !body.Zoom())
		return true;
	
	Point unit = body.Unit();
	// Cull sprites that are completely off screen, to reduce the number of draw
	// calls that we issue (which may be the bottleneck on some systems).
	Point size(
		fabs(unit.X() * body.Height()) + fabs(unit.Y() * body.Width()),
		fabs(unit.X() * body.Width()) + fabs(unit.Y() * body.Height()));
	Point topLeft = position - size * zoom;
	Point bottomRight = position + size * zoom;
	if(bottomRight.X() < Screen::Left() || bottomRight.Y() < Screen::Top())
		return true;
	if(topLeft.X() > Screen::Right() || topLeft.Y() > Screen::Bottom())
		return true;
	
	return false;
}
