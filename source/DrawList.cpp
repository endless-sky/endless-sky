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
#include "LightSpriteShader.h"
#include "Preferences.h"
#include "Screen.h"
#include "Sprite.h"
#include "SpriteSet.h"
#include "SpriteShader.h"

#include <cmath>

using namespace std;

const uint32_t DrawList::Item::SWIZZLE_MASK = 0xFF;
const uint32_t DrawList::Item::FADE_MASK = 0xFF00;
const float DrawList::Item::FADE_FACTOR = 256.0f;
const int DrawList::Item::FADE_SHIFT = 8;
const uint32_t DrawList::Item::NORMAL_MASK = 0xFF<<16;
const float DrawList::Item::NORMAL_FACTOR = 255.0f;
const int DrawList::Item::NORMAL_SHIFT = 16;
const uint32_t DrawList::Item::LIGHT_MASK = 1<<24;
const int  DrawList::Item::LIGHT_SHIFT = 24;

DrawList::DrawList(){
	std::copy(LightSpriteShader::DEF_AMBIENT, LightSpriteShader::DEF_AMBIENT+3, lightAmbient);
}

// Clear the list.
void DrawList::Clear(int step, double zoom)
{
	items.clear();
	lightsPos.clear();
	lightsEmit.clear();
	this->step = step;
	this->zoom = zoom;
	isHighDPI = (Screen::IsHighResolution() ? zoom > .5 : zoom > 1.);
}



void DrawList::SetCenter(const Point &center, const Point &centerVelocity)
{
	this->center = center;
	this->centerVelocity = centerVelocity;
}


void DrawList::SetAmbient(const float ambient[3]){
	std::copy(ambient, ambient+3, lightAmbient);
}


// Add an object based on the Body class.
bool DrawList::Add(const Body &body, double cloak, float normUse)
{
	if(cloak >= 1.)
		return false;
	return CullPush(body, body.Position(), body.Velocity(), cloak, 1., body.GetSwizzle(), true, normUse);
}

bool DrawList::Add(const Body &body, Point position, float normUse)
{
	return CullPush(body, position, body.Velocity(), 0., 1., body.GetSwizzle(), true, normUse);
}

bool DrawList::AddUnblurred(const Body &body, float normUse)
{
	return CullPush(body, body.Position(), centerVelocity, 0., 1., body.GetSwizzle(), true, normUse);
}

bool DrawList::AddProjectile(const Body &body, const Point &adjustedVelocity, double clip, bool lighted, float normUse)
{
	Point position = body.Position() + .5 * body.Velocity();
	if(clip <= 0.)
		return false;
	
	return CullPush(body, position, adjustedVelocity, 0., clip, body.GetSwizzle(), lighted, normUse);
}



bool DrawList::AddSwizzled(const Body &body, int swizzle, float normUse)
{
	return CullPush(body, body.Position(), body.Velocity(), 0., 1., swizzle, true, normUse);
}

bool DrawList::AddUnlighted(const Body &body)
{
	return CullPush(body, body.Position(), body.Velocity(), 0., 1., body.GetSwizzle(), false);
}

bool DrawList::AddEffect(const Body& body){
	return CullPush(body, body.Position(), centerVelocity, 0., 1., body.GetSwizzle(), false);
}

bool DrawList::AddStar(const Body& body, bool blur){	
	return CullPush(body, body.Position(), blur ? body.Velocity() : centerVelocity, 0., 1., body.GetSwizzle(), false);
}


bool DrawList::AddLightSource(const float pos[3], const float emit[3]){
	if(lightsPos.size()/3 >= (unsigned int)(LightSpriteShader::MaxNbLights())) return false;
	for(int i=0; i<3; i++)
	{
		lightsPos.push_back(pos[i]);
		lightsEmit.push_back(emit[i]);
	}
	return true;
}

// Draw all the items in this list.
void DrawList::Draw(bool lights) const
{
	bool showBlur = Preferences::Has("Render motion blur");
	if(!lights || !LightSpriteShader::IsAvailable())
	{
	
		SpriteShader::Bind();

		for(const Item &item : items)
			SpriteShader::Add(
				item.tex0, item.tex1, item.position, item.transform,
				item.Swizzle(), item.Clip(), item.Fade(), showBlur ? item.blur : nullptr);

		SpriteShader::Unbind();
		
	}
	else
	{
	
		LightSpriteShader::Bind();

		for(const Item &item : items)
			LightSpriteShader::Add(
				item.tex0, item.tex1, item.position, item.transform,
				item.Swizzle(), item.Clip(), item.Fade(), showBlur ? item.blur : nullptr,
				item.posGS, item.transformGS, 
				(item.Lighted()&&lights) ? (lightsPos.size()/3) : -1, lightAmbient, 
				lightsPos.data(), lightsEmit.data(), item.NormalUse(),
				1.f, item.texL);

		LightSpriteShader::Unbind();
		
	}
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

void DrawList::Push(const Body &body, Point pos, Point posGS, Point blur, double cloak, double clip, int swizzle, bool lighted, float normUse)
{
	Item item;
	
	Body::Frame frame = body.GetFrame(step, isHighDPI);
	item.tex0 = frame.first;
	item.tex1 = frame.second;
	item.texL = frame.light;
	
	item.flags = swizzle 
		| (static_cast<uint32_t>(frame.fade * Item::FADE_FACTOR) << Item::FADE_SHIFT) 
		| (static_cast<uint32_t>(normUse * Item::NORMAL_FACTOR) << Item::NORMAL_SHIFT) 
		| (static_cast<uint32_t>(lighted) << Item::LIGHT_SHIFT);
	
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
	item.posGS[0] = posGS.X();
	item.posGS[1] = posGS.Y();
	
	// (0, -1) means a zero-degree rotation (since negative Y is up).
	item.transformGS[0] = -uw.Y();
	item.transformGS[1] = uw.X();
	item.transformGS[2] = -uh.X();
	item.transformGS[3] = -uh.Y();
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
bool DrawList::CullPush(const Body &body, Point posGS, Point vel, double cloak, double clip, int swizzle, bool lighted, float normUse){
	Point posRel = posGS - center;
	Point blur = vel - centerVelocity;
	if(Cull(body, posRel, blur))
		return false;
	Push(body, posRel, posGS, blur, cloak, clip, swizzle, lighted, normUse);
	return true;
}


		
// Get the color swizzle.
uint32_t DrawList::Item::Swizzle() const
{
	return (flags & SWIZZLE_MASK);
}


		
float DrawList::Item::Clip() const
{
	return clip;
}



float DrawList::Item::Fade() const
{
	return ( (flags & FADE_MASK) >> FADE_SHIFT) / FADE_FACTOR;
}



void DrawList::Item::Cloak(double cloak)
{
	tex1 = SpriteSet::Get("ship/cloaked")->Texture();
	flags &= ~FADE_MASK;
	flags |= static_cast<uint32_t>(cloak * FADE_FACTOR) << FADE_SHIFT;
}

bool DrawList::Item::Lighted() const
{
	return flags & LIGHT_MASK;
}

float DrawList::Item::NormalUse() const
{
	return ( (flags & NORMAL_MASK) >> NORMAL_SHIFT) / NORMAL_FACTOR;
}
