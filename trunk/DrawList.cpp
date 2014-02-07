/* DrawList.cpp
Michael Zahniser, 17 Oct 2013

Function definitions for the DrawList class.
*/

#include "DrawList.h"

#include "Animation.h"
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
void DrawList::Add(const Animation &animation, Point pos, Point unit, double clip)
{
	if(!animation.IsEmpty())
		items.emplace_back(animation, pos, unit, clip, step);
}



// Add a single sprite.
void DrawList::Add(const Sprite *sprite, Point pos, Point unit)
{
	Animation animation(sprite, 1.f);
	Add(animation, pos, unit);
}



// Draw all the items in this list.
void DrawList::Draw() const
{
	SpriteShader::Bind();
	
	for(const Item &item : items)
		SpriteShader::Add(item.Texture0(), item.Texture1(),
			item.Position(), item.Transform(),
			item.Swizzle(), item.Clip(), item.Fade());
	
	SpriteShader::Unbind();
}



DrawList::Item::Item(const Animation &animation, Point pos, Point unit, float clip, int step)
	: position{static_cast<float>(pos.X()), static_cast<float>(pos.Y())},
	clip(clip), flags(animation.GetSwizzle())
{
	Animation::Frame frame = animation.Get(step);
	tex0 = frame.first;
	tex1 = frame.second;
	flags |= static_cast<uint32_t>(frame.fade * 256.f) << 8;
	
	Point uw = unit * animation.Width();
	Point uh = unit * animation.Height();
	
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
