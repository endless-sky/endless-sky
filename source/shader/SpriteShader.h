/* SpriteShader.h
Copyright (c) 2014 by Michael Zahniser

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

#include "../Point.h"
#include "../Swizzle.h"

#include <cstdint>

class Sprite;



// Class for drawing sprites. You can optionally draw a sprite with a custom
// zoom level or color swizzle. A more complicated function is also provided for
// adjusting the scale, rotation, clipping, fading, etc. of a sprite; this is
// most often just for use by the DrawList class, which calculates those input
// parameters based on an object's rotation, animation frame, etc.
class SpriteShader {
public:
	class Item {
	public:
		uint32_t texture = 0;
		uint32_t swizzleMask = 0;
		const Swizzle *swizzle = nullptr;
		float frame = 0.f;
		float frameCount = 1.f;
		float position[2] = {0.f, 0.f};
		float transform[4] = {0.f, 0.f, 0.f, 0.f};
		float blur[2] = {0.f, 0.f};
		float clip = 1.f;
		float alpha = 1.f;
	};


public:
	// Initialize the shaders.
	static void Init();

	// Draw a sprite.
	static void Draw(const Sprite *sprite, const Point &position, float zoom = 1.f,
		const Swizzle *swizzle = Swizzle::None(), float frame = 0.f, const Point &unit = Point(0., -1.));
	static Item Prepare(const Sprite *sprite, const Point &position, float zoom = 1.f,
		const Swizzle *swizzle = Swizzle::None(), float frame = 0.f, const Point &unit = Point(0., -1.));

	static void Bind();
	static void Add(const Item &item, bool withBlur = false);
	static void Unbind();
};
