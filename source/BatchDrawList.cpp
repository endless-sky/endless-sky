/* BatchDrawList.cpp
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
#include "Projectile.h"
#include "Screen.h"
#include "Sprite.h"
#include "Visual.h"

#include <cmath>

using namespace std;

namespace {
	void Push(int offset, array<float, 6 * 5> &v, const Point &pos, float s, float t, float frame)
	{
		v[offset + 0] = pos.X();
		v[offset + 1] = pos.Y();
		v[offset + 2] = s;
		v[offset + 3] = t;
		v[offset + 4] = frame;
	}
}



// Clear the list, also setting the global time step for animation.
void BatchDrawList::Clear(int step)
{
	state.batchData.clear();
	this->step = step;
}



void BatchDrawList::SetCenter(const Point &center)
{
	this->center = center;
}



void BatchDrawList::UpdateZoom(double zoom)
{
	this->zoom = zoom;
	isHighDPI = (Screen::IsHighResolution() ? zoom > .5 : zoom > 1.);
}



// Add an unswizzled object based on the Body class.
bool BatchDrawList::Add(const Projectile &body, float clip)
{
	// TODO: Rather than compensate using 1/2 the Visual | Projectile velocity, we should
	// extend the Sprite class to know its reference point. For most sprites, this will be
	// the horizontal and vertical middle of the sprite, but for "laser" projectiles, this
	// would be the middle of a sprite end. Adding such support will then help resolve issues
	// with drawing things such as very large effects that simulate projectiles. This offset
	// exists because we use the current position of a projectile, but have varied expectations
	// of what that position means. For a "laser" projectile, it is created at the ship hardpoint but
	// we want it to be drawn with its center halfway to the target. For longer-lived projectiles, we
	// expect the position to be the actual location of the projectile at that point in time.
	Point position = body.Position() + .5 * body.Velocity() - center;
	return Add(body, position, clip, body);
}



// TODO: Once we have sprite reference positions, this method will not be needed.
bool BatchDrawList::AddVisual(const Visual &visual)
{
	return Add(visual, visual.Position() - center, 1.f, visual);
}



// Draw all the items in this list.
void BatchDrawList::Draw() const
{
	BatchShader::Bind();
	
	for(const auto &it : RenderState::interpolated.batchData)
	{
		// Merge the vertices together.
		vector<float> data;
		data.reserve(30 * it.second.size());
		for(const auto &jt : it.second)
			data.insert(data.end(), jt.vertices.begin(), jt.vertices.end());
		BatchShader::Add(it.first, isHighDPI, std::move(data), zoom);
	}
	
	BatchShader::Unbind();
}



RenderState BatchDrawList::ConsumeState()
{
	return std::move(state);
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
	Point topLeft = position * zoom - size * zoom;
	Point bottomRight = position * zoom + size * zoom;
	if(bottomRight.X() < Screen::Left() || bottomRight.Y() < Screen::Top())
		return true;
	if(topLeft.X() > Screen::Right() || topLeft.Y() > Screen::Bottom())
		return true;
	
	return false;
}



bool BatchDrawList::Add(const Body &body, Point position, float clip, unsigned id)
{
	if(Cull(body, position))
		return false;
	
	// Get the data array for this particular sprite.
	auto &d = state.batchData[body.GetSprite()];
	d.emplace_back();
	d.back().object = id;
	auto &v = d.back().vertices;

	// The sprite frame is the same for every vertex.
	float frame = body.GetFrame(step);
	
	// Get unit vectors in the direction of the object's width and height.
	Point unit = body.Unit();
	Point uw = Point(unit.Y(), -unit.X()) * body.Width();
	Point uh = unit * body.Height();
	
	// Get the "bottom" corner, the one that won't be clipped.
	Point topLeft = position - (uw + uh);
	// Scale the vectors and apply clipping to the "height" of the sprite.
	uw *= 2.;
	uh *= 2.f * clip;
	
	// Calculate the other three corners.
	Point topRight = topLeft + uw;
	Point bottomLeft = topLeft + uh;
	Point bottomRight = bottomLeft + uw;
	
	// Push two copies of the first and last vertices to mark the break between
	// the sprites.
	Push(0, v, topLeft, 0.f, 1.f, frame);
	Push(5, v, topLeft, 0.f, 1.f, frame);
	Push(10, v, topRight, 1.f, 1.f, frame);
	Push(15, v, bottomLeft, 0.f, 1.f - clip, frame);
	Push(20, v, bottomRight, 1.f, 1.f - clip, frame);
	Push(25, v, bottomRight, 1.f, 1.f - clip, frame);
	
	return true;
}
