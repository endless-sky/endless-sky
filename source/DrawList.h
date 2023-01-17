/* DrawList.h
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

#ifndef DRAW_LIST_H_
#define DRAW_LIST_H_

#include "AbsoluteScreenSpace.h"
#include "Body.h"
#include "Point.h"
#include "Preferences.h"
#include "ScaledScreenSpace.h"
#include "ScreenSpace.h"
#include "Sprite.h"
#include "SpriteSet.h"
#include "SpriteShader.h"

#include <cmath>
#include <cstdint>
#include <memory>
#include <vector>

using namespace std;

// Class for storing a list of textures to blit to the screen. This allows the
// work of calculating the transformation matrices to be done in a separate
// thread from the graphics thread. However, the SpriteShader class is also
// available for drawing individual sprites in contexts where putting them into
// a DrawList first does not make sense.
class DrawList {
private:
	template<typename T>
	class DrawListImpl {
	public:
		// Clear the list, also setting the global time step for animation.
		void Clear(int step = 0, double zoom = 1.);
		void SetCenter(const Point &center, const Point &centerVelocity = Point());
		// Add an object based on the Body class.
		bool Add(const Body &body, double cloak = 0.);
		// Add an object at the given position (rather than its own).
		bool Add(const Body &body, Point position, double cloak = 0.);
		// Add an object that should not be drawn with motion blur.
		bool AddUnblurred(const Body &body);
		// Add an object using a specific swizzle (rather than its own).
		bool AddSwizzled(const Body &body, int swizzle);

		// Draw all the items in this list.
		void Draw() const;

	private:
		// Determine if the given object should be drawn at all.
		bool Cull(const Body &body, const Point &position, const Point &blur) const;

		void Push(const Body &body, Point pos, Point blur, double cloak, int swizzle);


	private:
		int step = 0;
		double zoom = 1.;
		bool isHighDPI = false;
		std::vector<SpriteShader::Item> items;

		Point center;
		Point centerVelocity;
	};
public:
	typedef typename DrawList::DrawListImpl<AbsoluteScreenSpace> ViewSpace;
	typedef typename DrawList::DrawListImpl<ScaledScreenSpace> UISpace;
};

template<typename T>
// Clear the list.
void DrawList::DrawListImpl<T>::Clear(int step, double zoom)
{
	std::shared_ptr<ScreenSpace> screenSpace = ScreenSpace::Variant<T>::instance();
	items.clear();
	this->step = step;
	this->zoom = zoom;
	isHighDPI = (screenSpace->IsHighResolution() ? zoom > .5 : zoom > 1.);
}


template<typename T>
void DrawList::DrawListImpl<T>::SetCenter(const Point &center, const Point &centerVelocity)
{
	this->center = center;
	this->centerVelocity = centerVelocity;
}


template<typename T>
// Add an object based on the Body class.
bool DrawList::DrawListImpl<T>::Add(const Body &body, double cloak)
{
	return Add(body, body.Position(), cloak);
}


template<typename T>
bool DrawList::DrawListImpl<T>::Add(const Body &body, Point position, double cloak)
{
	position -= center;
	Point blur = body.Velocity() - centerVelocity;
	if(Cull(body, position, blur))
		return false;

	Push(body, std::move(position), std::move(blur), cloak, body.GetSwizzle());
	return true;
}



template<typename T>
bool DrawList::DrawListImpl<T>::AddUnblurred(const Body &body)
{
	Point position = body.Position() - center;
	Point blur;
	if(Cull(body, position, blur))
		return false;

	Push(body, position, blur, 0., body.GetSwizzle());
	return true;
}



template<typename T>
bool DrawList::DrawListImpl<T>::AddSwizzled(const Body &body, int swizzle)
{
	Point position = body.Position() - center;
	Point blur = body.Velocity() - centerVelocity;
	if(Cull(body, position, blur))
		return false;

	Push(body, position, blur, 0., swizzle);
	return true;
}



template<typename T>
// Draw all the items in this list.
void DrawList::DrawListImpl<T>::Draw() const
{
	SpriteShader::ShaderImpl<T>::Bind();

	bool withBlur = Preferences::Has("Render motion blur");
	for(const SpriteShader::Item &item : items)
		SpriteShader::ShaderImpl<T>::Add(item, withBlur);

	SpriteShader::ShaderImpl<T>::Unbind();
}


template<typename T>
bool DrawList::DrawListImpl<T>::Cull(const Body &body, const Point &position, const Point &blur) const
{
	std::shared_ptr<ScreenSpace> screenSpace = ScreenSpace::Variant<T>::instance();
	if(!body.HasSprite() || !body.Zoom())
		return true;

	Point unit = body.Facing().Unit();
	// Cull sprites that are completely off screen, to reduce the number of draw
	// calls that we issue (which may be the bottleneck on some systems).
	Point size(
		.5 * (fabs(unit.X() * body.Height()) + fabs(unit.Y() * body.Width()) + fabs(blur.X())),
		.5 * (fabs(unit.X() * body.Width()) + fabs(unit.Y() * body.Height()) + fabs(blur.Y())));
	Point topLeft = (position - size) * zoom;
	Point bottomRight = (position + size) * zoom;
	if(bottomRight.X() < screenSpace->Left() || bottomRight.Y() < screenSpace->Top())
		return true;
	if(topLeft.X() > screenSpace->Right() || topLeft.Y() > screenSpace->Bottom())
		return true;

	return false;
}


template<typename T>
void DrawList::DrawListImpl<T>::Push(const Body &body, Point pos, Point blur, double cloak, int swizzle)
{
	SpriteShader::Item item;

	item.texture = body.GetSprite()->Texture(isHighDPI);
	item.frame = body.GetFrame(step);
	item.frameCount = body.GetSprite()->Frames();

	item.position[0] = static_cast<float>(pos.X() * zoom);
	item.position[1] = static_cast<float>(pos.Y() * zoom);

	// Get unit vectors in the direction of the object's width and height.
	double width = body.Width();
	double height = body.Height();
	Point unit = body.Facing().Unit();
	Point uw = unit * width;
	Point uh = unit * height;

	// (0, -1) means a zero-degree rotation (since negative Y is up).
	uw *= zoom;
	uh *= zoom;
	item.transform[0] = -uw.Y();
	item.transform[1] = uw.X();
	item.transform[2] = -uh.X();
	item.transform[3] = -uh.Y();

	// Calculate the blur vector, in texture coordinates.
	blur *= zoom;
	item.blur[0] = unit.Cross(blur) / (width * 4.);
	item.blur[1] = -unit.Dot(blur) / (height * 4.);

	item.alpha = 1. - cloak;
	item.swizzle = swizzle;
	item.clip = 1.;

	items.push_back(item);
}

#endif
