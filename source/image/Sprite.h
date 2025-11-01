/* Sprite.h
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

#include <cstdint>
#include <string>

class ImageBuffer;



// Class representing a drawable sprite. A sprite can have multiple frames, for
// animation. Each frame is stored in a separate OpenGL texture object. This may
// not be as efficient as sprite sheets, but with modern graphics cards it will
// not matter much and it makes working with the graphics a lot simpler.
class Sprite {
public:
	explicit Sprite(const std::string &name = "");

	const std::string &Name() const;

	// Add the given frames, optionally uploading them. The given buffer will be cleared afterwards.
	void AddFrames(ImageBuffer &buffer, bool is2x, bool noReduction);
	void AddSwizzleMaskFrames(ImageBuffer &buffer, bool is2x, bool noReduction);
	// Free up all textures loaded for this sprite.
	void Unload();

	// Image dimensions, in pixels.
	float Width() const;
	float Height() const;
	// Number of frames in the animation. If high DPI frames exist, the code has
	// ensured that they have the same number of frames.
	int Frames() const;

	// Get the offset of the center from the top left corner; this is for easy
	// shifting of corner to center coordinates.
	Point Center() const;

	// Get the texture index, either looking it up based on the Screen's HighDPI
	// setting or specifying it manually.
	uint32_t Texture() const;
	uint32_t Texture(bool isHighDPI) const;

	uint32_t SwizzleMask() const;
	uint32_t SwizzleMask(bool isHighDPI) const;

private:
	std::string name;

	uint32_t texture[2] = {0, 0};
	uint32_t swizzleMask[2] = {0, 0};

	float width = 0.f;
	float height = 0.f;
	int frames = 0;
};
