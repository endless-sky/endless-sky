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



// Clear the list.
void DrawList::Clear(int step, double zoom)
{
	items.clear();
	this->step = step;
	this->zoom = zoom;
	isHighDPI = (Screen::IsHighResolution() ? zoom > .5 : zoom > 1.);
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
	
	Push(body, position, blur, cloak, 1., body.GetSwizzle());
	return true;
}



bool DrawList::Add(const Body &body, Point position)
{
	position -= center;
	Point blur = body.Velocity() - centerVelocity;
	if(Cull(body, position, blur))
		return false;
	
	Push(body, position, blur, 0., 1., body.GetSwizzle());
	return true;
}



bool DrawList::AddUnblurred(const Body &body)
{
	Point position = body.Position() - center;
	Point blur;
	if(Cull(body, position, blur))
		return false;
	
	Push(body, position, blur, 0., 1., body.GetSwizzle());
	return true;
}



bool DrawList::AddProjectile(const Body &body, const Point &adjustedVelocity, double clip)
{
	Point position = body.Position() + .5 * body.Velocity() - center;
	Point blur = adjustedVelocity - centerVelocity;
	if(Cull(body, position, blur) || clip <= 0.)
		return false;
	
	Push(body, position, blur, 0., clip, body.GetSwizzle());
	return true;
}



bool DrawList::AddSwizzled(const Body &body, int swizzle)
{
	Point position = body.Position() - center;
	Point blur = body.Velocity() - centerVelocity;
	if(Cull(body, position, blur))
		return false;
	
	Push(body, position, blur, 0., 1., swizzle);
	return true;
}



// Draw all the items in this list.
void DrawList::Draw() const
{
	bool showBlur = Preferences::Has("Render motion blur");
	SpriteShader::Bind();

	for(const Item &item : items)
		SpriteShader::Add(
			item.tex0, item.tex1, item.position, item.transform,
			item.Swizzle(), item.Clip(), item.Fade(), showBlur ? item.blur : nullptr);

	SpriteShader::Unbind();
}



bool DrawList::Cull(const Body &body, const Point &position, const Point &blur) const
{
	if(!body.HasSprite() || !body.Zoom())
		return true;
	
	Point unit = body.Facing().Unit();
	// Cull sprites that are completely off screen, to reduce the number of draw
	// calls that we issue (which may be the bottleneck on some systems).
	Point size(
		.5 * (fabs(unit.X() * body.Height()) + fabs(unit.Y() * body.Width()) + fabs(blur.X())),
		.5 * (fabs(unit.X() * body.Width()) + fabs(unit.Y() * body.Height()) + fabs(blur.Y())));
	Point topLeft = (position - size) * zoom;
	Point bottomRight = (position + size) * zoom;
	if(bottomRight.X() < Screen::Left() || bottomRight.Y() < Screen::Top())
		return true;
	if(topLeft.X() > Screen::Right() || topLeft.Y() > Screen::Bottom())
		return true;
	
	return false;
}



void DrawList::Push(const Body &body, Point pos, Point blur, double cloak, double clip, int swizzle)
{
	Item item;
	
	Body::Frame frame = body.GetFrame(step, isHighDPI);
	item.tex0 = frame.first;
	item.tex1 = frame.second;
	item.flags = swizzle | (static_cast<uint32_t>(frame.fade * 256.f) << 8);
	
	// Get unit vectors in the direction of the object's width and height.
	double width = body.Width();
	double height = body.Height();
	Point unit = body.Facing().Unit();
	Point uw = unit * width;
	Point uh = unit * height;
	
	item.clip = clip;
	if(clip < 1.)
	{
		// "clip" is the fraction of its height that we're clipping the sprite
		// to. We still want it to start at the same spot, though.
		pos -= uh * ((1. - clip) * .5);
		uh *= clip;
	}
	item.position[0] = static_cast<float>(pos.X() * zoom);
	item.position[1] = static_cast<float>(pos.Y() * zoom);
	
	// (0, -1) means a zero-degree rotation (since negative Y is up).
	uw *= zoom;
	uh *= zoom;
	item.transform[0] = -uw.Y();
	item.transform[1] = uw.X();
	item.transform[2] = -uh.X();
	item.transform[3] = -uh.Y();
	
	// Calculate the blur vector, in texture coordinates.
	blur *= zoom;
	item.blur[0] = unit.Cross(blur) / (width * 4.);
	item.blur[1] = -unit.Dot(blur) / (height * 4.);

	if(cloak > 0.f)
		item.Cloak(cloak);
	
	items.push_back(item);
}


		
// Get the color swizzle.
uint32_t DrawList::Item::Swizzle() const
{
	return (flags & 0xFF);
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
