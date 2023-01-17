/* ScaledScreenSpace.h
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

#ifndef SCALEDSCREENSPACE_H_
#define SCALEDSCREENSPACE_H_

#include "Point.h"
#include "Screen.h"
#include "ScreenSpace.h"

#include <memory>

// Implementation of a Screen Space with coordinates relative to the the user's zoom
// level. This is used when rendering UI elements that scale with user scaling
// settings. Can only be used in the drawing thread.
class ScaledScreenSpace : public ScreenSpace
{
public:
	// Zoom level as specified by the user.
	int UserZoom() override;
	// Effective zoom level, as restricted by the current resolution / window size.
	int Zoom() override;
	void SetZoom(int percent) override;

	// Specify that this is a high-DPI window.
	void SetHighDPI(bool isHighDPI = true) override;
	// This is true if the screen is high DPI, or if the zoom is above 100%.
	bool IsHighResolution() override;
	Point Dimensions() override;
	int Width() const override;
	int Height() const override;

	// Get the positions of the edges and corners of the viewport.
	int Left() override;
	int Top() override;
	int Right() override;
	int Bottom() override;

	Point TopLeft() override;
	Point TopRight() override;
	Point BottomLeft() override;
	Point BottomRight() override;

	// Get a singleton instance of ScaledScreenSpace.
	static std::shared_ptr<ScaledScreenSpace> instance();

private:
	ScaledScreenSpace() = default;
};

#endif
