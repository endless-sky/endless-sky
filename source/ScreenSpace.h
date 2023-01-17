/* ScreenSpace.h
Copyright (c) 2023 by Daniel Weiner

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef SCREENSPACE_H_
#define SCREENSPACE_H_

#include "Point.h"

#include <memory>

// A representation of screen dimensions. Depending on implementation, this can either
// be affected by user scaling settings, or ignore those settings entirely. Can only
// be used in the drawing thread.
class ScreenSpace {
public:
	// Zoom level as specified by the user.
	virtual int UserZoom() = 0;
	// Effective zoom level, as restricted by the current resolution / window size.
	virtual int Zoom() = 0;
	virtual void SetZoom(int percent) = 0;

	// Specify that this is a high-DPI window.
	virtual void SetHighDPI(bool isHighDPI = true) = 0;
	// This is true if the screen is high DPI, or if the zoom is above 100%.
	virtual bool IsHighResolution() = 0;

	virtual Point Dimensions() = 0;

	virtual int Width() const = 0;
	virtual int Height() const = 0;

	// Get the positions of the edges and corners of the viewport.
	virtual int Left() = 0;
	virtual int Top() = 0;
	virtual int Right() = 0;
	virtual int Bottom() = 0;

	virtual Point TopLeft() = 0;
	virtual Point TopRight() = 0;
	virtual Point BottomLeft() = 0;
	virtual Point BottomRight() = 0;

	virtual ~ScreenSpace() {};

	// Get the conversion factor between two screen spaces. Useful when rendering to one screen
	// space base on the coordinates of another screen space.
	template<typename T, typename U>
	static double ConversionFactor();

	// Get the coordinates on one screen space of an equivalent point in another screen space.
	template<typename T, typename U>
	static Point ConvertPoint(const Point &point);

	template<typename T>
	struct Variant {
		// Get a singleton instance of AbsoluteScreenSpace.
		static std::shared_ptr<ScreenSpace> instance();
	};
};

template<typename T, typename U>
double ScreenSpace::ConversionFactor()
{
	std::shared_ptr<ScreenSpace> from = ScreenSpace::Variant<T>::instance();
	std::shared_ptr<ScreenSpace> to = ScreenSpace::Variant<U>::instance();

	return static_cast<double>(from->Zoom()) / static_cast<double>(to->Zoom());
}

template<typename T, typename U>
Point ScreenSpace::ConvertPoint(const Point &point)
{
	return point * ConversionFactor<T, U>();
}


template<typename T>
std::shared_ptr<ScreenSpace> ScreenSpace::Variant<T>::instance()
{
	return T::instance();
}


#endif
