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

#include "Point.h"

#include "AbsoluteScreenSpace.h"
#include "BatchShader.h"
#include "Body.h"
#include "ScaledScreenSpace.h"
#include "ScreenSpace.h"
#include "Sprite.h"

#include <cmath>
#include <map>
#include <memory>
#include <vector>

using namespace std;

// This class collects a set of OpenGL draw commands to issue and groups them by
// sprite, so all instances of each sprite can be drawn with a single command.
class BatchDrawList {
private:
	template<typename T>
		class DrawListImpl {
			public:
		// Clear the list, also setting the global time step for animation.
		void Clear(int step = 0, double zoom = 1.);
		void SetCenter(const Point &center);

		// Add an unswizzled object based on the Body class.
		bool Add(const Body &body, float clip = 1.f);
		bool AddVisual(const Body &visual);

		// Draw all the items in this list.
		void Draw() const;


	private:
		void Push(vector<float> &v, const Point &pos, float s, float t, float frame);

		// Determine if the given body should be drawn at all.
		bool Cull(const Body &body, const Point &position) const;

		// Add the given body at the given position.
		bool Add(const Body &body, Point position, float clip);


	private:
		int step = 0;
		double zoom = 1.;
		bool isHighDPI = false;
		Point center;

		// Each sprite consists of six vertices (four vertices to form a quad and
		// two dummy vertices to mark the break in between them). Each of those
		// vertices has five attributes: (x, y) position in pixels, (s, t) texture
		// coordinates, and the index of the sprite frame.
		std::map<const Sprite *, std::vector<float>> data;
	};
public:
	typedef typename BatchDrawList::DrawListImpl<AbsoluteScreenSpace> ViewSpace;
	typedef typename BatchDrawList::DrawListImpl<ScaledScreenSpace> UISpace;
};

// Clear the list, also setting the global time step for animation.
template<typename T>
void BatchDrawList::DrawListImpl<T>::Clear(int step, double zoom)
{
	std::shared_ptr<ScreenSpace> screenSpace = ScreenSpace::Variant<T>::instance();
	data.clear();
	this->step = step;
	this->zoom = zoom;
	isHighDPI = (screenSpace->IsHighResolution() ? zoom > .5 : zoom > 1.);
}



template<typename T>
void BatchDrawList::DrawListImpl<T>::SetCenter(const Point &center)
{
	this->center = center;
}



// Add an unswizzled object based on the Body class.
template<typename T>
bool BatchDrawList::DrawListImpl<T>::Add(const Body &body, float clip)
{
	// TODO: Rather than compensate using 1/2 the Visual | Projectile velocity, we should
	// extend the Sprite class to know its reference point. For most sprites, this will be
	// the horizontal and vertical middle of the sprite, but for "laser" projectiles, this
	// would be the middle of a sprite end. Adding such support will then help resolve issues
	// with drawing things such as very large effects that simulate projectiles. This offset
	// exists because we use the current position of a projectile, but have varied expectations
	// of what that position means. For a "laser" projectile, it is created at the ship hardpoint but
	// we want it to be drawn with its center halfway to the target. For longer-lived projectiles, we
	// expect the position to be the actual location of the projectile at that point in time.
	Point position = (body.Position() + .5 * body.Velocity() - center) * zoom;
	return Add(body, position, clip);
}



// TODO: Once we have sprite reference positions, this method will not be needed.
template<typename T>
bool BatchDrawList::DrawListImpl<T>::AddVisual(const Body &visual)
{
	return Add(visual, (visual.Position() - center) * zoom, 1.f);
}



// Draw all the items in this list.
template<typename T>
void BatchDrawList::DrawListImpl<T>::Draw() const
{
	BatchShader::ShaderImpl<T>::Bind();

	for(const pair<const Sprite * const, vector<float>> &it : data)
		BatchShader::ShaderImpl<T>::Add(it.first, isHighDPI, it.second);

	BatchShader::ShaderImpl<T>::Unbind();
}

template <typename T>
inline void BatchDrawList::DrawListImpl<T>::Push(vector<float> &v, const Point &pos, float s, float t, float frame)
{
	v.push_back(pos.X());
	v.push_back(pos.Y());
	v.push_back(s);
	v.push_back(t);
	v.push_back(frame);
}

template <typename T>
bool BatchDrawList::DrawListImpl<T>::Cull(const Body &body, const Point &position) const
{
	std::shared_ptr<ScreenSpace> screenSpace = ScreenSpace::Variant<T>::instance();
	if(!body.HasSprite() || !body.Zoom())
		return true;

	Point unit = body.Unit();
	// Cull sprites that are completely off screen, to reduce the number of draw
	// calls that we issue (which may be the bottleneck on some systems).
	Point size(
		fabs(unit.X() * body.Height()) + fabs(unit.Y() * body.Width()),
		fabs(unit.X() * body.Width()) + fabs(unit.Y() * body.Height()));
	Point topLeft = position - size * zoom;
	Point bottomRight = position + size * zoom;
	if(bottomRight.X() < screenSpace->Left() || bottomRight.Y() < screenSpace->Top())
		return true;
	if(topLeft.X() > screenSpace->Right() || topLeft.Y() > screenSpace->Bottom())
		return true;

	return false;
}



template<typename T>
bool BatchDrawList::DrawListImpl<T>::Add(const Body &body, Point position, float clip)
{
	if(Cull(body, position))
		return false;

	// Get the data vector for this particular sprite.
	vector<float> &v = data[body.GetSprite()];
	// The sprite frame is the same for every vertex.
	float frame = body.GetFrame(step);

	// Get unit vectors in the direction of the object's width and height.
	Point unit = body.Unit() * zoom;
	Point uw = Point(-unit.Y(), unit.X()) * body.Width();
	Point uh = unit * body.Height();

	// Get the "bottom" corner, the one that won't be clipped.
	Point topLeft = position - (uw + uh);
	// Scale the vectors and apply clipping to the "height" of the sprite.
	uw *= 2.;
	uh *= 2.f * clip;

	// Calculate the other three corners.
	Point topRight = topLeft + uw;
	Point bottomLeft = topLeft + uh;
	Point bottomRight = bottomLeft + uw;

	// Push two copies of the first and last vertices to mark the break between
	// the sprites.
	Push(v, topLeft, 0.f, 1.f, frame);
	Push(v, topLeft, 0.f, 1.f, frame);
	Push(v, topRight, 1.f, 1.f, frame);
	Push(v, bottomLeft, 0.f, 1.f - clip, frame);
	Push(v, bottomRight, 1.f, 1.f - clip, frame);
	Push(v, bottomRight, 1.f, 1.f - clip, frame);

	return true;
}

#endif
