/* Body.h
Copyright (c) 2016 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "Body.h"

#include "DataNode.h"
#include "DataWriter.h"
#include "Random.h"
#include "Sprite.h"
#include "SpriteSet.h"

#include <algorithm>
#include <cmath>

using namespace std;



// Constructor.
Body::Body(const Sprite *sprite, Point position, Point velocity, Angle facing, double zoom)
	: position(position), velocity(velocity), angle(facing), zoom(zoom), sprite(sprite), randomize(true)
{
}



Body::Body(const Body &sprite, Point position, Point velocity, Angle facing, double zoom)
{
	*this = sprite;
	this->position = position;
	this->velocity = velocity;
	this->angle = facing;
	this->zoom = zoom;
}



// Check that this Body has a sprite and that the sprite has at least one frame.
bool Body::HasSprite() const
{
	return (sprite && sprite->Frames());
}



// Access the underlying Sprite object.
const Sprite *Body::GetSprite() const
{
	return sprite;
}



// Get the dimensions of the sprite.
double Body::Width() const
{
	return sprite ? (.5 * zoom) * sprite->Width() : 0.;
}



double Body::Height() const
{
	return sprite ? (.5 * zoom) * sprite->Height() : 0.;
}



// Get the farthest a part of this sprite can be from its center.
double Body::Radius() const
{
	return .5 * Point(Width(), Height()).Length();
}



// Which color swizzle should be applied to the sprite?
int Body::GetSwizzle() const
{
	return swizzle;
}



// Get the sprite and mask for the given time step.
Body::Frame Body::GetFrame(int step) const
{
	SetStep(step);
	return frame;
}



const Mask &Body::GetMask(int step) const
{
	static const Mask empty;
	
	SetStep(step);
	return (mask ? *mask : empty);
}



// Positional attributes.
const Point &Body::Position() const
{
	return position;
}



const Point &Body::Velocity() const
{
	return velocity;
}



const Angle &Body::Facing() const
{
	return angle;
}



Point Body::Unit() const
{
	return angle.Unit() * (.5 * Zoom());
}



double Body::Zoom() const
{
	return max(zoom, 0.);
}



// Store the government here too, so that collision detection that is based
// on the Body class can figure out which objects will collide.
const Government *Body::GetGovernment() const
{
	return government;
}



// Sprite serialization.
void Body::LoadSprite(const DataNode &node)
{
	if(node.Size() < 2)
		return;
	sprite = SpriteSet::Get(node.Token(1));
	
	// The only time the animation does not start on a specific frame is if no
	// start frame is specified and it repeats. Since a frame that does not
	// start at zero starts when the game started, it does not make sense for it
	// to do that unless it is repeating endlessly.
	for(const DataNode &child : node)
	{
		if(child.Token(0) == "frame rate" && child.Size() >= 2 && child.Value(1) >= 0.)
			frameRate = child.Value(1) / 60.;
		else if(child.Token(0) == "frame time" && child.Size() >= 2 && child.Value(1) > 0.)
			frameRate = 1. / child.Value(1);
		else if(child.Token(0) == "delay" && child.Size() >= 2 && child.Value(1) > 0.)
			delay = child.Value(1);
		else if(child.Token(0) == "start frame" && child.Size() >= 2)
		{
			frameOffset += child.Value(1);
			startAtZero = true;
		}
		else if(child.Token(0) == "random start frame")
			randomize = true;
		else if(child.Token(0) == "no repeat")
		{
			repeat = false;
			startAtZero = true;
		}
		else if(child.Token(0) == "rewind")
			rewind = true;
		else
			child.PrintTrace("Skipping unrecognized attribute:");
	}
}



void Body::SaveSprite(DataWriter &out) const
{
	if(!sprite)
		return;
	
	out.Write("sprite", sprite->Name());
	out.BeginChild();
	{
		if(frameRate != static_cast<float>(2. / 60.))
			out.Write("frame rate", frameRate * 60.);
		if(delay)
			out.Write("delay", delay);
		if(randomize)
			out.Write("random start frame");
		if(!repeat)
			out.Write("no repeat");
		if(rewind)
			out.Write("rewind");
	}
	out.EndChild();
}



// Set the sprite.
void Body::SetSprite(const Sprite *sprite)
{
	this->sprite = sprite;
}



// Set the color swizzle.
void Body::SetSwizzle(int swizzle)
{
	this->swizzle = swizzle;
}



void Body::SetFrameRate(double framesPerSecond)
{
	frameRate = framesPerSecond / 60.;
}



void Body::AddFrameRate(double framesPerSecond)
{
	frameRate += framesPerSecond / 60.;
}



void Body::SetStep(int step) const
{
	if(step < 0 || !sprite || !sprite->Frames())
		return;
	
	// If the sprite only has one frame, no need to animate anything.
	int frames = sprite->Frames();
	if(frames <= 1)
	{
		frame.first = sprite->Texture();
		mask = &sprite->GetMask();
		return;
	}
	
	// If this is the very first step, fill in some values that we could not set
	// until we knew the sprite's frame count and the starting step.
	if(randomize)
	{
		randomize = false;
		frameOffset += Random::Int(sprite->Frames() + delay);
	}
	else if(startAtZero)
	{
		startAtZero = false;
		// Adjust frameOffset so that this step's frame is exactly 0 (no fade).
		frameOffset -= frameRate * step;
	}
	
	// Figure out what fraction of the way in between frames we are. Avoid any
	// possible floating-point glitches that might result in a negative frame.
	float frameF = max(0.f, frameRate * step + frameOffset);
	frame.fade = modf(frameF, &frameF);
	step = frameF;
	
	int firstIndex = 0;
	int secondIndex = 0;
	if(!rewind)
	{
		if(!repeat && step >= frames - 1)
		{
			firstIndex = frames - 1;
			frame.fade = 0.f;
		}
		else
		{
			step %= (frames + delay);
			if(step >= frames)
				frame.fade = 0.f;
			else
			{
				firstIndex = step;
				secondIndex = step + 1;
			}
		}
	}
	else
	{
		if(!repeat && step >= 2 * (frames - 1))
			frame.fade = 0.f;
		else
		{
			step %= 2 * (frames - 1) + delay;
			if(step < frames - 1)
			{
				firstIndex = step;
				secondIndex = step + 1;
			}
			else if(step < 2 * (frames - 1))
			{
				step = 2 * (frames - 1) - step;
				firstIndex = step;
				secondIndex = step - 1;
			}
			else
				frame.fade = 0.f;
		}
	}
	frame.first = sprite->Texture(firstIndex);
	frame.second = sprite->Texture(secondIndex);
	mask = &sprite->GetMask(frame.fade > .5f ? secondIndex : firstIndex);
}
