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

#ifndef SHIP_FX_SHADER_H_
#define SHIP_FX_SHADER_H_

class Sprite;
class Point;

#include <cstdint>
#include <vector>



// Class for drawing ship effects, such as shield or heat.
// It's a clone of the SpriteShader, with a few extra doohickeys.
class ShipEffectsShader {
public:
	enum class EffectType {
		EFFECT_HIT_SHIELD,
	};

	class Item {
	public:
		uint32_t texture = 0;
		uint32_t swizzle = 0;
		float frame = 0.f;
		float frameCount = 1.f;
		float position[2] = {0.f, 0.f};
		float transform[4] = {0.f, 0.f, 0.f, 0.f};
		float blur[2] = {0.f, 0.f};
		float clip = 1.f;
		float alpha = 1.f;
		std::vector<float> recentHitPoints = std::vector<float>(32 * 2);
		int recentHits = 0;
	};


public:
	// Initialize the shaders.
	static void Init(bool useShaderSwizzle);

	// Draw a sprite.
	static void Draw(const Sprite *sprite, const Point &position, float zoom = 1.f, int swizzle = 0, float frame = 0.f);
	static Item Prepare(const Sprite *sprite, const Point &position, float zoom = 1.f, int swizzle = 0, float frame = 0.f);

	static void Bind();
	static void Add(const Item &item, bool withBlur = false);
	static void Unbind();


private:
	static bool useShaderSwizzle;
};



#endif
