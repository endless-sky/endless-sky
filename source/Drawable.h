/* Drawable.h
Copyright (c) 2026 by Endless Sky contributors

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include "Angle.h"
#include "Point.h"
#include "Swizzle.h"

#include <string>

class DataNode;
class DataWriter;
class Sprite;



// Class representing any object in the game that usually also has a sprite
// that can be animated.
class Drawable {
public:
	// Constructors.
	Drawable() = default;
	explicit Drawable(const Sprite *sprite, double zoom = 1., Point scale = Point(1., 1.), double alpha = 1.);
	Drawable(const Drawable &other, double zoom, Point scale, double alpha);

	// Check that this Drawable has a sprite and that the sprite has dimensions to it.
	// The sprite may be unloaded, though.
	bool HasSprite() const;
	// Access the underlying Sprite object.
	const Sprite *GetSprite() const;
	// Get the dimensions of the sprite.
	double Width() const;
	double Height() const;
	// Get the farthest a part of this sprite can be from its center.
	double Radius() const;
	// Which color swizzle should be applied to the sprite?
	const Swizzle *GetSwizzle() const;
	bool InheritsParentSwizzle() const;
	// Get the sprite frame for the given time step.
	float GetFrame(int step = -1) const;

	// Positional attributes.
	double Zoom() const;
	Point Scale() const;

	// Sprite serialization.
	void LoadSprite(const DataNode &node);
	void SaveSprite(DataWriter &out, const std::string &tag = "sprite") const;
	// Set the sprite.
	void SetSprite(const Sprite *sprite);
	// Set the color swizzle.
	void SetSwizzle(const Swizzle *swizzle);

	double Alpha() const;


protected:
	// Adjust the frame rate.
	void SetFrameRate(float framesPerSecond);
	void AddFrameRate(float framesPerSecond);
	void PauseAnimation();
	// Set what animation step we're on. This affects future calls to Body::GetMask()
	// and Drawable::GetFrame().
	void SetStep(int step) const;


protected:
	// Animation parameters.
	const Sprite *sprite = nullptr;
	mutable float frame = 0.f;

	// The point that is considered to be the center of the sprite.
	Point center;
	// A zoom of 1 means the sprite should be drawn at half size. For objects
	// whose sprites should be full size, use zoom = 2.
	float zoom = 1.f;
	Point scale = Point{1., 1.};

	double alpha = 1.;


private:
	// Allow objects based on this one to adjust their frame rate and swizzle.
	const Swizzle *swizzle = Swizzle::None();
	bool inheritsParentSwizzle = false;

	float frameRate = 2.f / 60.f;
	int delay = 0;
	// The chosen frame will be (step * frameRate) + frameOffset.
	mutable float frameOffset = 0.f;
	mutable bool startAtZero = false;
	mutable bool randomize = false;
	bool repeat = true;
	bool rewind = false;
	int pause = 0;

	// Cache the frame calculation so it doesn't have to be repeated if given
	// the same step over and over again.
	mutable int currentStep = -1;
};
