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

	// Record the height and width from the buffer, but don't upload the buffer into memory.
	// Used for sprites that use deferred loading to allow them to at least know their
	// size for certain panels that rely on knowing a sprite's size without needing the sprite
	// to be fully loaded.
	void LoadDimensions(const ImageBuffer &buffer);
	// Whether this sprite has any dimensions, guaranteeing that it exists even if it needs to be loaded.
	bool HasDimensions() const;

	// Add the given frames, optionally uploading them. The given buffers will be cleared afterwards.
	// Receive both the 1x and 2x buffers. If the 2x buffer is not empty, then it will be used.
	void AddFrames(ImageBuffer &buffer1x, ImageBuffer &buffer2x, bool noReduction);
	void AddSwizzleMaskFrames(ImageBuffer &buffer1x, ImageBuffer &buffer2x, bool noReduction);
	// Whether the textures for this sprite have been uploaded yet.
	bool IsLoaded() const;
	// Free up all textures loaded for this sprite.
	void Unload();

	// Image dimensions, in pixels.
	float Width() const;
	float Height() const;
	// Number of frames in the animation. If high DPI frames exist, the code has
	// ensured that they have the same number of frames.
	int Frames() const;
	// This will either be 0, 1, or the same as the number of normal frames.
	int SwizzleMaskFrames() const;

	// Get the offset of the center from the top left corner; this is for easy
	// shifting of corner to center coordinates.
	Point Center() const;

	// Get the texture index.
	uint32_t Texture() const;
	uint32_t SwizzleMask() const;


private:
	std::string name;

	uint32_t texture{};
	uint32_t swizzleMask{};
	bool isLoaded = false;

	float width = 0.f;
	float height = 0.f;
	int frames = 0;
	int swizzleMaskFrames = 0;
};
