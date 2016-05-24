/* Animation.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "Animation.h"

#include "DataNode.h"
#include "DataWriter.h"
#include "Random.h"
#include "Sprite.h"
#include "SpriteSet.h"

#include <algorithm>
#include <cmath>

using namespace std;



Animation::Animation(const Sprite *sprite, float frameRate)
	: sprite(sprite), frameRate(frameRate / 60.), randomize(true)
{
}



// Load the animation.
void Animation::Load(const DataNode &node)
{
	if(node.Size() < 2)
		return;
	spriteName = node.Token(1);
	sprite = SpriteSet::Get(spriteName);
	
	// The only time the animation does not start on a specific frame is if no
	// start frame is specified and it repeats. Since a frame that does not
	// start at zero starts when the game started, it does not make sense for it
	// to do that unless it is repeating endlessly.
	for(const DataNode &child : node)
	{
		if(child.Token(0) == "frame rate" && child.Size() >= 2 && child.Value(1) > 0.)
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



// Save this animation's information to a ship descriptor. Only saves the
// frame rate and the rewind flag if set, not the other settings, since
// those will not generally apply to a ship sprite.
void Animation::Save(DataWriter &out) const
{
	out.Write("sprite", spriteName);
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



// Check if this animation contains any frames.
bool Animation::IsEmpty() const
{
	return (!sprite || !sprite->Frames());
}



// Get the characteristics of the sprite.
int Animation::Width() const
{
	return sprite ? sprite->Width() : 0;
}



int Animation::Height() const
{
	return sprite ? sprite->Height() : 0;
}



// Get the sprite itself.
const Sprite *Animation::GetSprite() const
{
	return sprite;
}



// Set or get the color swizzle.
void Animation::SetSwizzle(int swizzle)
{
	this->swizzle = swizzle;
}



int Animation::GetSwizzle() const
{
	return swizzle;
}


	
Animation::Frame Animation::Get(int step) const
{
	// Deal with special cases: no sprite, or a sprite with only one frame.
	Frame frame;
	if(!sprite)
		return frame;
	int frames = sprite->Frames();
	if(frames <= 1)
	{
		frame.first = sprite->Texture();
		return frame;
	}
	
	// If this is the very first step, fill in some values that we could not set
	// until we knew the sprite's frame count and the starting step.
	if(randomize | startAtZero)
		DoFirst(step);
	
	// Figure out what fraction of the way in between frames we are. Avoid any
	// possible floating-point glitches that might result in a negative frame.
	float frameF = max(0.f, frameRate * step + frameOffset);
	frame.fade = modf(frameF, &frameF);
	step = frameF;
	
	if(!rewind)
	{
		if(!repeat && step >= frames - 1)
		{
			frame.first = sprite->Texture(frames - 1);
			frame.fade = 0.f;
		}
		else
		{
			step %= (frames + delay);
			if(step >= frames)
			{
				frame.first = sprite->Texture(0);
				frame.fade = 0.f;
			}
			else
			{
				frame.first = sprite->Texture(step);
				frame.second = sprite->Texture(step + 1);
			}
		}
	}
	else
	{
		if(!repeat && step >= 2 * (frames - 1))
		{
			frame.first = sprite->Texture(0);
			frame.fade = 0.f;
		}
		else
		{
			step %= 2 * (frames - 1) + delay;
			if(step < frames - 1)
			{
				frame.first = sprite->Texture(step);
				frame.second = sprite->Texture(step + 1);
			}
			else if(step < 2 * (frames - 1))
			{
				step = 2 * (frames - 1) - step;
				frame.first = sprite->Texture(step);
				frame.second = sprite->Texture(step - 1);
			}
			else
			{
				frame.first = sprite->Texture(0);
				frame.fade = 0.f;
			}
		}
	}
	return frame;
}



// Get the mask for the given step.
const Mask &Animation::GetMask(int step) const
{
	static const Mask empty;
	if(!sprite)
		return empty;
	int frames = sprite->Frames();
	if(frames <= 1)
		return sprite->GetMask();
	
	// If this is the very first step, fill in some values that we could not set
	// until we knew the sprite's frame count and the starting step.
	if(randomize | startAtZero)
		DoFirst(step);
	
	// Figure out what frame we are on, rounded to the nearest frame.
	step = static_cast<int>(frameRate * step + frameOffset + .5f);
	// If not repeating, if rewinding, the animation stops at frame (2 * f - 1).
	// Otherwise, it stops at frame (f - 1).
	if(!repeat)
		step = min(step, (rewind + 1) * frames - 1);
	// The cycle size depends on whether we are rewinding or not:
	step %= rewind ? 2 * (frames - 1) : frames;
	// If we are rewinding and in the rewind phase, step backward:
	if(step > frames - 1)
		step = 2 * (frames - 1) - step;
	
	// Return the mask.
	return sprite->GetMask(step);
}



// Modify the frame rate.
void Animation::AddFrameRate(float framesPerSecond)
{
	frameRate = max(0.f, frameRate + framesPerSecond / 60.f);
}



void Animation::DoFirst(int step) const
{
	// If this is the very first step, fill in some values that we could not set
	// until we knew the sprite's frame count and the starting step.
	if(randomize)
	{
		randomize = false;
		if(sprite && sprite->Frames())
			frameOffset += Random::Int(sprite->Frames() + delay);
	}
	else if(startAtZero)
	{
		startAtZero = false;
		// Adjust frameOffset so that this step's frame is exactly 0 (no fade).
		frameOffset -= frameRate * step;
	}
}
