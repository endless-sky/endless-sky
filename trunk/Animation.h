/* Animation.h
Michael Zahniser, 14 Dec 2013

Class representing an animation, i.e. a series of sprites.
*/

#ifndef ANIMATION_H_INCLUDED
#define ANIMATION_H_INCLUDED

#include "DataFile.h"

class Mask;
class Sprite;



class Animation {
public:
	class Frame {
	public:
		Frame();
		
		uint32_t first;
		uint32_t second;
		float fade;
	};
	
	
public:
	// Default constructor.
	Animation();
	Animation(const Sprite *sprite, float frameRate);
	
	// Load the animation.
	void Load(const DataFile::Node &node);
	
	// Check if this animation contains any frames.
	bool IsEmpty() const;
	// Get the characteristics of the sprite.
	int Width() const;
	int Height() const;
	// Get the sprite itself.
	const Sprite *GetSprite() const;
	
	// Set or get the color swizzle.
	void SetSwizzle(int swizzle);
	int GetSwizzle() const;
	
	// Get the parameters for a frame at the given time step.
	Frame Get(int step) const;
	// Get the mask for the given step.
	const Mask &GetMask(int step) const;
	
	
private:
	void DoFirst(int step) const;
	
	
private:
	const Sprite *sprite;
	int swizzle;
	
	float frameRate;
	// The chosen frame will be (step * frameRate) + frameOffset.
	mutable float frameOffset;
	mutable bool startAtZero;
	mutable bool randomize;
	bool repeat;
	bool rewind;
};



#endif
