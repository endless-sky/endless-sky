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

#include "Animation.h"
#include "BlurShader.h"
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



// Add an animation.
bool DrawList::Add(const Animation &animation, Point pos, Point unit, Point blur, double clip)
{
	if(animation.IsEmpty() || !unit)
		return false;
	
	// Cull sprites that are completely off screen, to reduce the number of draw
	// calls that we issue (which may be the bottleneck on some systems).
	Point size(
		.5 * (fabs(unit.X() * animation.Height()) + fabs(unit.Y() * animation.Width()) + fabs(blur.X())),
		.5 * (fabs(unit.X() * animation.Width()) + fabs(unit.Y() * animation.Height()) + fabs(blur.Y())));
	Point topLeft = pos - size;
	Point bottomRight = pos + size;
	if(bottomRight.X() < Screen::Left() || bottomRight.Y() < Screen::Top())
		return false;
	if(topLeft.X() > Screen::Right() || topLeft.Y() > Screen::Bottom())
		return false;
		
	items.emplace_back(animation, pos, unit, blur, clip, step);
	return true;
}



// Add a single sprite.
bool DrawList::Add(const Sprite *sprite, Point pos, Point unit, Point blur, double cloak, int swizzle)
{
	if(cloak >= 1.)
		return false;
	
	Animation animation(sprite, 1.f);
	animation.SetSwizzle(swizzle);
	if(!Add(animation, pos, unit, blur))
		return false;
	
	if(cloak > 0.)
		items.back().Cloak(cloak);
	
	return true;
}



// Draw all the items in this list.
void DrawList::Draw() const
{
	if(Preferences::Has("Render motion blur"))
	{
		BlurShader::Bind();
	
		for(const Item &item : items)
			BlurShader::Add(item.Texture0(), item.Texture1(),
				item.Position(), item.Transform(),
				item.Swizzle(), item.Clip(), item.Fade(), item.Blur());
	
		BlurShader::Unbind();
	}
	else
	{
		SpriteShader::Bind();
	
		for(const Item &item : items)
			SpriteShader::Add(item.Texture0(), item.Texture1(),
				item.Position(), item.Transform(),
				item.Swizzle(), item.Clip(), item.Fade());
	
		SpriteShader::Unbind();
	}
}



DrawList::Item::Item(const Animation &animation, Point pos, Point unit, Point blur, float clip, int step)
	: position{static_cast<float>(pos.X()), static_cast<float>(pos.Y())},
	clip(clip), flags(animation.GetSwizzle())
{
	Animation::Frame frame = animation.Get(step);
	tex0 = frame.first;
	tex1 = frame.second;
	flags |= static_cast<uint32_t>(frame.fade * 256.f) << 8;
	
	double width = animation.Width();
	double height = animation.Height();
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
	
	// Calculate the blur vector, in texture coordinates. This should be done by
	// projecting the blur vector onto the unit vector and then scaling it based
	// on the sprite size. But, the unit vector first has to be normalized (i.e.
	// divided by the unit vector length), and the sprite size also has to be
	// multiplied by the unit vector size, so:
	double zoomCorrection = 4. * unit.LengthSquared();
	this->blur[0] = unit.Cross(blur) / (width * zoomCorrection);
	this->blur[1] = -unit.Dot(blur) / (height * zoomCorrection);
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
