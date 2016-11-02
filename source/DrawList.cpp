/* DrawList.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "DrawList.h"

#include "Body.h"
#include "Preferences.h"
#include "Screen.h"
#include "Sprite.h"
#include "SpriteSet.h"
#include "SpriteShader.h"

#include <cmath>

using namespace std;



// Default constructor.
DrawList::DrawList()
{
	Clear();
}



// Clear the list.
void DrawList::Clear(int step)
{
	items.clear();
	this->step = step;
}



void DrawList::SetCenter(const Point &center, const Point &centerVelocity)
{
	this->center = center;
	this->centerVelocity = centerVelocity;
}



// Add an object based on the Body class.
bool DrawList::Add(const Body &body, double cloak)
{
	Point position = body.Position() - center;
	Point blur = body.Velocity() - centerVelocity;
	if(Cull(body, position, blur) || cloak >= 1.)
		return false;
	
	items.emplace_back(body, position, blur, cloak, 1., body.GetSwizzle(), step);
	return true;
}



bool DrawList::Add(const Body &body, Point position)
{
	position -= center;
	Point blur = body.Velocity() - centerVelocity;
	if(Cull(body, position, blur))
		return false;
	
	items.emplace_back(body, position, blur, 0., 1., body.GetSwizzle(), step);
	return true;
}



bool DrawList::AddUnblurred(const Body &body)
{
	Point position = body.Position() - center;
	Point blur;
	if(Cull(body, position, blur))
		return false;
	
	items.emplace_back(body, position, blur, 0., 1., body.GetSwizzle(), step);
	return true;
}



bool DrawList::AddProjectile(const Body &body, const Point &adjustedVelocity, double clip)
{
	Point position = body.Position() + .5 * body.Velocity() - center;
	Point blur = adjustedVelocity - centerVelocity;
	if(Cull(body, position, blur) || clip <= 0.)
		return false;
	
	items.emplace_back(body, position, blur, 0., clip, body.GetSwizzle(), step);
	return true;
}



bool DrawList::AddSwizzled(const Body &body, int swizzle)
{
	Point position = body.Position() - center;
	Point blur = body.Velocity() - centerVelocity;
	if(Cull(body, position, blur))
		return false;
	
	items.emplace_back(body, position, blur, 0., 1., swizzle, step);
	return true;
}



// Draw all the items in this list.
void DrawList::Draw() const
{
	bool showBlur = Preferences::Has("Render motion blur");
	SpriteShader::Bind();

	for(const Item &item : items)
		SpriteShader::Add(item.Texture0(), item.Texture1(),
			item.Position(), item.Transform(),
			item.Swizzle(), item.Clip(), item.Fade(), showBlur ? item.Blur() : nullptr);

	SpriteShader::Unbind();
}



bool DrawList::Cull(const Body &body, const Point &position, const Point &blur)
{
	if(!body.HasSprite() || !body.Zoom())
		return true;
	
	Point unit = body.Facing().Unit();
	// Cull sprites that are completely off screen, to reduce the number of draw
	// calls that we issue (which may be the bottleneck on some systems).
	Point size(
		.5 * (fabs(unit.X() * body.Height()) + fabs(unit.Y() * body.Width()) + fabs(blur.X())),
		.5 * (fabs(unit.X() * body.Width()) + fabs(unit.Y() * body.Height()) + fabs(blur.Y())));
	Point topLeft = position - size;
	Point bottomRight = position + size;
	if(bottomRight.X() < Screen::Left() || bottomRight.Y() < Screen::Top())
		return true;
	if(topLeft.X() > Screen::Right() || topLeft.Y() > Screen::Bottom())
		return true;
	
	return false;
}



DrawList::Item::Item(const Body &body, Point pos, Point blur, float cloak, float clip, int swizzle, int step)
	: position{static_cast<float>(pos.X()), static_cast<float>(pos.Y())}, clip(clip), flags(swizzle)
{
	Body::Frame frame = body.GetFrame(step);
	tex0 = frame.first;
	tex1 = frame.second;
	flags |= static_cast<uint32_t>(frame.fade * 256.f) << 8;
	
	double width = body.Width();
	double height = body.Height();
	Point unit = body.Facing().Unit();
	Point uw = unit * width;
	Point uh = unit * height;
	
	if(clip < 1.)
	{
		// "clip" is the fraction of its height that we're clipping the sprite
		// to. We still want it to start at the same spot, though.
		pos -= uh * ((1. - clip) * .5);
		position[0] = static_cast<float>(pos.X());
		position[1] = static_cast<float>(pos.Y());
		uh *= clip;
	}
	
	// (0, -1) means a zero-degree rotation (since negative Y is up).
	transform[0] = -uw.Y();
	transform[1] = uw.X();
	transform[2] = -uh.X();
	transform[3] = -uh.Y();
	
	// Calculate the blur vector, in texture coordinates.
	this->blur[0] = unit.Cross(blur) / (width * 4.);
	this->blur[1] = -unit.Dot(blur) / (height * 4.);

	if(cloak > 0.)
		Cloak(cloak);
}



// Get the texture of this sprite.
uint32_t DrawList::Item::Texture0() const
{
	return tex0;
}



uint32_t DrawList::Item::Texture1() const
{
	return tex1;
}



// These two items can be uploaded directly to the shader:
// Get the (x, y) position of the center of the sprite.
const float *DrawList::Item::Position() const
{
	return position;
}



// Get the [a, b; c, d] size and rotation matrix.
const float *DrawList::Item::Transform() const
{
	return transform;
}



// Get the blur vector, in texture space.
const float *DrawList::Item::Blur() const
{
	return blur;
}


		
// Get the color swizzle.
uint32_t DrawList::Item::Swizzle() const
{
	return (flags & 7);
}



		
float DrawList::Item::Clip() const
{
	return clip;
}



float DrawList::Item::Fade() const
{
	return (flags >> 8) / 256.f;
}



void DrawList::Item::Cloak(double cloak)
{
	tex1 = SpriteSet::Get("ship/cloaked")->Texture();
	flags &= 0xFF;
	flags |= static_cast<uint32_t>(cloak * 256.f) << 8;
}
