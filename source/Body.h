/* Body.h
Copyright (c) 2016 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef BODY_H_
#define BODY_H_

#include <cstdint>
#include <string>

#include "Angle.h"
#include "Point.h"

class DataNode;
class DataWriter;
class Government;
class Mask;
class Sprite;



// Class representing any object in the game that has a position, velocity, and
// facing direction and usually also has a sprite.
class Body {
public:
	// Constructors.
	Body() = default;
	Body(const Sprite *sprite, Point position, Point velocity = Point(), Angle facing = Angle(), double zoom = 1.);
	Body(const Body &sprite, Point position, Point velocity = Point(), Angle facing = Angle(), double zoom = 1.);
	
	// Check that this Body has a sprite and that the sprite has at least one frame.
	bool HasSprite() const;
	// Access the underlying Sprite object.
	const Sprite *GetSprite() const;
	// Get the dimensions of the sprite.
	double Width() const;
	double Height() const;
	// Get the farthest a part of this sprite can be from its center.
	double Radius() const;
	// Which color swizzle should be applied to the sprite?
	int GetSwizzle() const;
	// Get the sprite and mask for the given time step.
	float GetFrame(int step = -1) const;
	const Mask &GetMask(int step = -1) const;
	
	// Positional attributes.
	const Point &Position() const;
	const Point &Velocity() const;
	const Angle &Facing() const;
	Point Unit() const;
	double Zoom() const;
	
	// Check if this object is marked for removal from the game.
	bool ShouldBeRemoved() const;
	
	// Store the government here too, so that collision detection that is based
	// on the Body class can figure out which objects will collide.
	const Government *GetGovernment() const;
	
	// Sprite serialization.
	void LoadSprite(const DataNode &node);
	void SaveSprite(DataWriter &out, const std::string &tag = "sprite") const;
	// Set the sprite.
	void SetSprite(const Sprite *sprite);
	// Set the color swizzle.
	void SetSwizzle(int swizzle);
	
	
protected:
	// Adjust the frame rate.
	void SetFrameRate(double framesPerSecond);
	void AddFrameRate(double framesPerSecond);
	void PauseAnimation();
	// Mark this object to be removed from the game.
	void MarkForRemoval();
	// Mark that this object should not be removed (e.g. a launched fighter).
	void UnmarkForRemoval();
	
	
protected:
	// Basic positional attributes.
	Point position;
	Point velocity;
	Angle angle;
	// A zoom of 1 means the sprite should be drawn at half size. For objects
	// whose sprites should be full size, use zoom = 2.
	float zoom = 1.f;
	
	// Government, for use in collision checks.
	const Government *government = nullptr;
	
	
private:
	// Set what animation step we're on. This affects future calls to GetMask()
	// and GetFrame().
	void SetStep(int step) const;
	
	
private:
	// Animation parameters.
	const Sprite *sprite = nullptr;
	// Allow objects based on this one to adjust their frame rate and swizzle.
	int swizzle = 0;
	
	float frameRate = 2.f / 60.f;
	int delay = 0;
	// The chosen frame will be (step * frameRate) + frameOffset.
	mutable float frameOffset = 0.f;
	mutable bool startAtZero = false;
	mutable bool randomize = false;
	bool repeat = true;
	bool rewind = false;
	int pause = 0;
	
	// Record when this object is marked for removal from the game.
	bool shouldBeRemoved = false;
	
	// Cache the frame calculation so it doesn't have to be repeated if given
	// the same step over and over again.
	mutable int currentStep = -1;
	mutable float frame = 0.f;
};



#endif
