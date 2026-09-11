/* DrawList.cpp
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

#include "DrawList.h"

#include "../Angle.h"
#include "../Body.h"
#include "../Drawable.h"
#include "../Preferences.h"
#include "../Screen.h"
#include "../image/Sprite.h"
#include "SpriteShader.h"

#include <cmath>

using namespace std;



// Clear the list.
void DrawList::Clear(int step, double zoom)
{
	items.clear();
	this->step = step;
	this->zoom = zoom;
}



void DrawList::SetCenter(const Point &center, const Point &centerVelocity)
{
	this->center = center;
	this->centerVelocity = centerVelocity;
}



// Add an object based on the Body class.
bool DrawList::Add(const Body &body, double cloak)
{
	return Add(body, body.Position(), cloak);
}



bool DrawList::Add(const Body &body, Point position, double cloak)
{
	position -= center;
	Point blur = body.Velocity() - centerVelocity;
	if(Cull(body, position, body.Facing(), blur))
		return false;

	Push(body, position, body.Facing(), blur, body.Alpha(center), cloak, body.GetSwizzle());
	return true;
}



bool DrawList::Add(const Drawable &drawable, Point position, const Angle &facing, double cloak)
{
	position -= center;
	Point blur = -centerVelocity;
	if(Cull(drawable, position, facing, blur))
		return false;

	Push(drawable, position, facing, blur, drawable.Alpha(), cloak, drawable.GetSwizzle());
	return true;
}



bool DrawList::AddUnblurred(const Body &body)
{
	Point position = body.Position() - center;
	Point blur;
	if(Cull(body, position, body.Facing(), blur))
		return false;

	Push(body, position, body.Facing(), blur, body.Alpha(center), 0., body.GetSwizzle());
	return true;
}



bool DrawList::AddUnblurred(const Drawable &drawable, Point position, const Angle &facing)
{
	position -= center;
	Point blur;
	if(Cull(drawable, position, facing, blur))
		return false;

	Push(drawable, position, facing, blur, drawable.Alpha(), 0., drawable.GetSwizzle());
	return true;
}



bool DrawList::AddSwizzled(const Body &body, const Swizzle *swizzle, double cloak)
{
	Point position = body.Position() - center;
	Point blur = body.Velocity() - centerVelocity;
	if(Cull(body, position, body.Facing(), blur))
		return false;

	Push(body, position, body.Facing(), blur, body.Alpha(center), cloak, swizzle);
	return true;
}



bool DrawList::AddSwizzled(const Drawable &drawable, Point position, const Angle &facing, const Swizzle *swizzle,
	double cloak)
{
	position -= center;
	Point blur = -centerVelocity;
	if(Cull(drawable, position, facing, blur))
		return false;

	Push(drawable, position, facing, blur, drawable.Alpha(), cloak, swizzle);
	return true;
}



// Draw all the items in this list.
void DrawList::Draw() const
{
	SpriteShader::Bind();

	bool withBlur = Preferences::Has("Render motion blur");
	for(const SpriteShader::Item &item : items)
		SpriteShader::Add(item, withBlur);

	SpriteShader::Unbind();
}



bool DrawList::Cull(const Drawable &drawable, const Point &position, const Angle &facing, const Point &blur) const
{
	if(!drawable.HasSprite() || !drawable.Zoom())
		return true;

	Point unit = facing.Unit();
	// Cull sprites that are completely off screen, to reduce the number of draw
	// calls that we issue (which may be the bottleneck on some systems).
	Point size(
		.5 * (fabs(unit.X() * drawable.Height()) + fabs(unit.Y() * drawable.Width()) + fabs(blur.X())),
		.5 * (fabs(unit.X() * drawable.Width()) + fabs(unit.Y() * drawable.Height()) + fabs(blur.Y())));
	Point topLeft = (position - size) * zoom;
	Point bottomRight = (position + size) * zoom;
	if(bottomRight.X() < Screen::Left() || bottomRight.Y() < Screen::Top())
		return true;
	if(topLeft.X() > Screen::Right() || topLeft.Y() > Screen::Bottom())
		return true;

	return false;
}



void DrawList::Push(const Drawable &drawable, const Point &pos, const Angle &facing, Point blur,
	double alpha, double cloak, const Swizzle *swizzle)
{
	SpriteShader::Item item;

	item.texture = drawable.GetSprite()->Texture();
	item.swizzleMask = drawable.GetSprite()->SwizzleMask();
	item.frame = drawable.GetFrame(step);
	item.frameCount = drawable.GetSprite()->Frames();
	item.uniqueSwizzleMaskFrames = drawable.GetSprite()->SwizzleMaskFrames() > 1;

	item.position[0] = static_cast<float>(pos.X() * zoom);
	item.position[1] = static_cast<float>(pos.Y() * zoom);

	// Get unit vectors in the direction of the object's width and height.
	double width = drawable.Width();
	double height = drawable.Height();
	Point unit = facing.Unit();
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

	item.alpha = (1. - cloak) * alpha;
	item.swizzle = swizzle;
	item.clip = 1.;

	items.push_back(item);
}
