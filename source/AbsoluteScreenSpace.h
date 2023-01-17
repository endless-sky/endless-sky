/* AbsoluteScreenSpace.h
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

#ifndef ABSOLUTESCREENSPACE_H_
#define ABSOLUTESCREENSPACE_H_


#include "Point.h"
#include "ScreenSpace.h"

#include <memory>

// Implementation of a Screen Space with coordinates in the user's raw pixel screen
// dimensions. This is used when rendering elements that are unaffected by user
// scaling settings, such as the main space view. Can only be used in the drawing
// thread.
class AbsoluteScreenSpace : public ScreenSpace {
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

	// Get a singleton instance of AbsoluteScreenSpace.
    [[nodiscard]] static std::shared_ptr<AbsoluteScreenSpace> instance();
private:
	bool highDpi = false;
	int zoom = 100;

	AbsoluteScreenSpace() = default;
};


#endif
