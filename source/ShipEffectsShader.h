/* ShipEffectsShader.h
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
class Ship;
class Government;

#include "Body.h"

#include <cstdint>
#include <memory>
#include <vector>





// Class for drawing ship effects, such as shield or heat.
// It's a clone of the SpriteShader, with a few extra doohickeys.
class ShipEffectsShader {
public:
	class EffectItem {
	public:
		uint32_t texture = 0;
		float frame = 0.f;
		float frameCount = 1.f;
		float position[2] = { 0.f, 0.f };
		float transform[4] = { 0.f, 0.f, 0.f, 0.f };
		float blur[2] = { 0.f, 0.f };
		float clip = 1.f;
		float alpha = 1.f;
		float recentHitPoints[64];
		float recentHitDamage[32];
		float shieldColor[4] = { 0.f, 0.f, 0.f, 0.f };
		size_t recentHits = 0;
		float ratio = 1.f;
		float size = 80.f;
	};


public:
	// Initialize the shaders.
	static void Init();
	static void SetCenter(Point newCenter);

	// Draw a sprite.
	static void Draw(const Ship* body, const Point& position, const std::vector<std::pair<Point, double>>* recentHits,
		const float zoom = 1.f, const float frame = 0.f, const std::vector<std::pair<std::string, double>>& shieldColor = {});
	static EffectItem Prepare(const Ship *body, const Point &position,
		const std::vector<std::pair<Point, double>>* recentHits, const float zoom = 1.f, const float frame = 0.f,
		const std::vector<std::pair<std::string, double>>& shieldColor = {});

	static void Bind();
	static void Add(const EffectItem &item, bool withBlur = false);
	static void Unbind();

	static Point center;

};



#endif
