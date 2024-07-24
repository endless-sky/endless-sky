/* BatchDrawList.h
Copyright (c) 2017 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef BATCH_DRAW_LIST_H_
#define BATCH_DRAW_LIST_H_

#include "concurrent/Parallel.h"
#include "Point.h"
#include "concurrent/ResourceProvider.h"

#include <map>
#include <vector>

class Body;
class Projectile;
class Sprite;
class Visual;



// This class collects a set of OpenGL draw commands to issue and groups them by
// sprite, so all instances of each sprite can be drawn with a single command.
class BatchDrawList {
public:
	BatchDrawList();
	// Clear the list, also setting the global time step for animation.
	void Clear(int step = 0, double zoom = 1.);
	void SetCenter(const Point &center);

	// Add an unswizzled object based on the Body class.
	bool Add(const Body &body, float clip = 1.f);
	bool AddProjectile(const Projectile &body);
	bool AddVisual(const Body &visual);

	// Draw all the items in this list.
	void Draw() const;

	template<template<class, class...> class Container, class Item, class ...Params>
	std::enable_if_t<std::is_base_of_v<Projectile, Item> || std::is_base_of_v<Visual, Item>>
	AddBatch(const Container<Item, Params...> &batch);


private:
	// Determine if the given body should be drawn at all.
	bool Cull(const Body &body, const Point &position) const;

	// Add the given body at the given position.
	bool Add(const Body &body, Point position, float clip, std::vector<float> &data);

	// Add an unswizzled object based on the Body class.
	bool Add(const Body &body, float clip, std::vector<float> &data);
	bool AddProjectile(const Projectile &body, std::vector<float> &data);
	bool AddVisual(const Body &visual, std::vector<float> &data);

private:
	int step = 0;
	double zoom = 1.;
	bool isHighDPI = false;
	Point center;

	// Each sprite consists of six vertices (four vertices to form a quad and
	// two dummy vertices to mark the break in between them). Each of those
	// vertices has six attributes: (x, y) position in pixels, (s, t) texture
	// coordinates, the index of the sprite frame, and the alpha value.
	// We keep multiple vectors for each sprite for better concurrent access.
	using DataType = std::map<const Sprite *, std::vector<std::vector<float>>>;
	DataType data;
	ResourceProvider<DataType> resourceProvider;
};



template<template<class, class...> class Container, class Item, class ...Params>
std::enable_if_t<std::is_base_of_v<Projectile, Item> || std::is_base_of_v<Visual, Item>>
BatchDrawList::AddBatch(const Container<Item, Params...> &batch)
{
	// Windows doesn't support thread locals in block scope.
	#if defined(__WIN32__) || defined(__WIN64__)
		for(const Item &item : batch)
			if constexpr(std::is_base_of_v<Projectile, Item>)
				AddProjectile(item);
			else
				AddVisual(item);
	#else
	for_each_mt(batch.begin(), batch.end(), [&](const auto &item) {
		const thread_local auto lock = resourceProvider.Lock();

		std::vector<std::vector<float>> &data = lock.get<0>()[item.GetSprite()];
		std::vector<float> &inner = data.empty() ? data.emplace_back() : data[0];
		inner.reserve(576); // Each body uses 6*6*4=144 bytes
		if constexpr(std::is_base_of_v<Projectile, Item>)
			AddProjectile(item, inner);
		else
			AddVisual(item, inner);
	});
	#endif
}


#endif
