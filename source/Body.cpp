/* Body.cpp
Copyright (c) 2016 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "Body.h"

#include "DataNode.h"
#include "DataWriter.h"
#include "GameData.h"
#include "image/Mask.h"
#include "image/MaskManager.h"
#include "pi.h"
#include "Random.h"
#include "image/Sprite.h"
#include "image/SpriteSet.h"

#include <algorithm>
#include <cmath>

using namespace std;



// Constructor, based on a Sprite.
Body::Body(const Sprite *sprite, Point position, Point velocity, Angle facing, double zoom, Point scale, double alpha)
	: position(position), velocity(velocity), angle(facing), scale(scale), zoom(zoom),
	alpha(alpha), sprite(sprite), randomize(true)
{
}



// Constructor, based on the animation from another Body object.
Body::Body(const Body &sprite, Point position, Point velocity, Angle facing, double zoom, Point scale, double alpha)
{
	*this = sprite;
	this->position = position;
	this->velocity = velocity;
	this->angle = facing;
	this->zoom = zoom;
	this->scale = scale * sprite.Scale();
	this->alpha = alpha * sprite.alpha;
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



// Get the width of this object, in world coordinates (i.e. taking zoom and scale into account).
double Body::Width() const
{
	return static_cast<double>(sprite ? (.5f * zoom) * scale.X() * sprite->Width() : 0.f);
}



// Get the height of this object, in world coordinates (i.e. taking zoom and scale into account).
double Body::Height() const
{
	return static_cast<double>(sprite ? (.5f * zoom) * scale.Y() * sprite->Height() : 0.f);
}



// Get the farthest a part of this sprite can be from its center.
double Body::Radius() const
{
	return .5 * Point(Width(), Height()).Length();
}



// Which color swizzle should be applied to the sprite?
const Swizzle *Body::GetSwizzle() const
{
	return swizzle;
}



bool Body::InheritsParentSwizzle() const
{
	return inheritsParentSwizzle;
}



// Get the frame index for the given time step. If no time step is given, this
// will return the frame from the most recently given step.
float Body::GetFrame(int step) const
{
	if(step >= 0)
		SetStep(step);

	return frame;
}



// Get the mask for the given time step. If no time step is given, this will
// return the mask from the most recently given step.
const Mask &Body::GetMask(int step) const
{
	if(step >= 0)
		SetStep(step);

	static const Mask EMPTY;
	int current = round(frame);
	if(!sprite || current < 0)
		return EMPTY;

	const vector<Mask> &masks = GameData::GetMaskManager().GetMasks(sprite, Scale());

	// Assume that if a masks array exists, it has the right number of frames.
	return masks.empty() ? EMPTY : masks[current % masks.size()];
}



// Position, in world coordinates (zero is the system center).
const Point &Body::Position() const
{
	return position;
}



// Velocity, in pixels per second.
const Point &Body::Velocity() const
{
	return velocity;
}



const Point Body::Center() const
{
	return -rotatedCenter + position;
}



// Direction this Body is facing in.
const Angle &Body::Facing() const
{
	return angle;
}



// Unit vector in the direction this body is facing. This represents the scale
// and transform that should be applied to the sprite before drawing it.
Point Body::Unit() const
{
	return angle.Unit() * (.5 * Zoom());
}



// Zoom factor. This controls how big the sprite should be drawn.
double Body::Zoom() const
{
	return max(zoom, 0.f);
}



Point Body::Scale() const
{
	return scale;
}



// Check if this object is marked for removal from the game.
bool Body::ShouldBeRemoved() const
{
	return shouldBeRemoved;
}



// Store the government here too, so that collision detection that is based
// on the Body class can figure out which objects will collide.
const Government *Body::GetGovernment() const
{
	return government;
}



// Load the sprite specification, including all animation attributes.
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
		const string &key = child.Token(0);
		bool hasValue = child.Size() >= 2;
		if(key == "frame rate" && hasValue && child.Value(1) >= 0.)
			frameRate = child.Value(1) / 60.;
		else if(key == "frame time" && hasValue && child.Value(1) > 0.)
			frameRate = 1. / child.Value(1);
		else if(key == "delay" && hasValue && child.Value(1) > 0.)
			delay = child.Value(1);
		else if(key == "scale" && hasValue && child.Value(1) > 0.)
		{
			double scaleY = (child.Size() >= 3 && child.Value(2) > 0.) ? child.Value(2) : child.Value(1);
			scale = Point(child.Value(1), scaleY);
		}
		else if(key == "start frame" && hasValue)
		{
			frameOffset += static_cast<float>(child.Value(1));
			startAtZero = true;
		}
		else if(key == "random start frame")
			randomize = true;
		else if(key == "no repeat")
		{
			repeat = false;
			startAtZero = true;
		}
		else if(key == "rewind")
			rewind = true;
		else if(key == "center" && child.Size() >= 3)
			center = Point(child.Value(1), child.Value(2));
		else if(key == "inherits parent swizzle")
			inheritsParentSwizzle = true;
		else
			child.PrintTrace("Skipping unrecognized attribute:");
	}

	if(scale != Point(1., 1.))
		GameData::GetMaskManager().RegisterScale(sprite, Scale());
}



// Save the sprite specification, including all animation attributes.
void Body::SaveSprite(DataWriter &out, const string &tag) const
{
	if(!sprite)
		return;

	out.Write(tag, sprite->Name());
	out.BeginChild();
	{
		if(frameRate != static_cast<float>(2. / 60.))
			out.Write("frame rate", frameRate * 60.);
		if(delay)
			out.Write("delay", delay);
		if(scale != Point(1., 1.))
			out.Write("scale", scale.X(), scale.Y());
		if(randomize)
			out.Write("random start frame");
		if(!repeat)
			out.Write("no repeat");
		if(rewind)
			out.Write("rewind");
		if(center)
			out.Write("center", center.X(), center.Y());
		if(inheritsParentSwizzle)
			out.Write("inherits parent swizzle");
	}
	out.EndChild();
}



// Set the sprite.
void Body::SetSprite(const Sprite *sprite)
{
	this->sprite = sprite;
	currentStep = -1;
}



// Set the color swizzle.
void Body::SetSwizzle(const Swizzle *swizzle)
{
	this->swizzle = swizzle;
}



double Body::Alpha(const Point &drawCenter) const
{
	return alpha * DistanceAlpha(drawCenter);
}



double Body::DistanceAlpha(const Point &drawCenter) const
{
	if(!distanceInvisible)
		return 1.;
	double distance = (drawCenter - position).Length();
	return clamp<double>((distance - distanceInvisible) / (distanceVisible - distanceInvisible), 0., 1.);
}



bool Body::IsVisible(const Point &drawCenter) const
{
	return DistanceAlpha(drawCenter) > 0.;
}



// Set the frame rate of the sprite. This is used for objects that just specify
// a sprite instead of a full animation data structure.
void Body::SetFrameRate(float framesPerSecond)
{
	frameRate = framesPerSecond / 60.f;
}



// Add the given amount to the frame rate.
void Body::AddFrameRate(float framesPerSecond)
{
	frameRate += framesPerSecond / 60.f;
}



void Body::PauseAnimation()
{
	++pause;
}



// Mark this object to be removed from the game.
void Body::MarkForRemoval()
{
	shouldBeRemoved = true;
}



// Mark this object to not be removed from the game.
void Body::UnmarkForRemoval()
{
	shouldBeRemoved = false;
}



// Turn this object around its center of rotation.
void Body::Turn(double amount)
{
	angle += amount;
	if(!center)
		return;

	auto RotatePointAroundOrigin = [](Point &toRotate, double radians) -> Point {
		float si = sin(radians);
		float co = cos(radians);
		float newX = toRotate.X() * co - toRotate.Y() * si;
		float newY = toRotate.X() * si + toRotate.Y() * co;
		return Point(newX, newY);
	};

	rotatedCenter = -RotatePointAroundOrigin(center, (angle - amount).Degrees() * TO_RAD);

	position -= rotatedCenter;

	rotatedCenter = RotatePointAroundOrigin(rotatedCenter, Angle(amount).Degrees() * TO_RAD);

	position += rotatedCenter;
}



void Body::Turn(const Angle &amount)
{
	Turn(amount.Degrees());
}



// Set the current time step.
void Body::SetStep(int step) const
{
	// If the animation is paused, reduce the step by however many frames it has
	// been paused for.
	step -= pause;

	// If the step is negative or there is no sprite, do nothing. This updates
	// and caches the mask and the frame so that if further queries are made at
	// this same time step, we don't need to redo the calculations.
	if(step == currentStep || step < 0 || !sprite || !sprite->Frames())
		return;
	currentStep = step;

	// If the sprite only has one frame, no need to animate anything.
	float frames = sprite->Frames();
	if(frames <= 1.f)
	{
		frame = 0.f;
		return;
	}
	float lastFrame = frames - 1.f;
	// This is the number of frames per full cycle. If rewinding, a full cycle
	// includes the first and last frames once and every other frame twice.
	float cycle = (rewind ? 2.f * lastFrame : frames) + delay;

	// If this is the very first step, fill in some values that we could not set
	// until we knew the sprite's frame count and the starting step.
	if(randomize)
	{
		randomize = false;
		// The random offset can be a fractional frame.
		frameOffset += static_cast<float>(Random::Real()) * cycle;
	}
	else if(startAtZero)
	{
		startAtZero = false;
		// Adjust frameOffset so that this step's frame is exactly 0 (no fade).
		frameOffset -= frameRate * step;
	}

	// Figure out what fraction of the way in between frames we are. Avoid any
	// possible floating-point glitches that might result in a negative frame.
	frame = max(0.f, frameRate * step + frameOffset);
	// If repeating, wrap the frame index by the total cycle time.
	if(repeat)
		frame = fmod(frame, cycle);

	if(!rewind)
	{
		// If not repeating, frame should never go higher than the index of the
		// final frame.
		if(!repeat)
			frame = min(frame, lastFrame);
		else if(frame >= frames)
		{
			// If we're in the delay portion of the loop, set the frame to 0.
			frame = 0.f;
		}
	}
	else if(frame >= lastFrame)
	{
		// In rewind mode, once you get to the last frame, count backwards.
		// Regardless of whether we're repeating, if the frame count gets to
		// be less than 0, clamp it to 0.
		frame = max(0.f, lastFrame * 2.f - frame);
	}
}
