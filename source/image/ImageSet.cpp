/* ImageSet.cpp
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

#include "ImageSet.h"

#include "../text/Format.h"
#include "../GameData.h"
#include "ImageBuffer.h"
#include "../Logger.h"
#include "Mask.h"
#include "MaskManager.h"
#include "Sprite.h"

#include <algorithm>
#include <cassert>

using namespace std;

namespace {
	// Determine whether the given path or name is to a sprite for which a
	// collision mask ought to be generated.
	bool IsMasked(const filesystem::path &path)
	{
		if(path.empty())
			return false;
		filesystem::path directory = *path.begin();
		return directory == "ship" || directory == "asteroid";
	}

	// Add consecutive frames from the given map to the given vector. Issue warnings for missing or mislabeled frames.
	void AddValid(const map<size_t, filesystem::path> &frameData, vector<filesystem::path> &sequence,
		const string &prefix, bool is2x, bool isSwizzleMask) noexcept(false)
	{
		if(frameData.empty())
			return;
		// Valid animations (or stills) begin with frame 0.
		if(frameData.begin()->first != 0)
		{
			Logger::LogError(prefix + "ignored " + (isSwizzleMask ? "mask " : "") + (is2x ? "@2x " : "")
				+ "frame " + to_string(frameData.begin()->first) + " (" + to_string(frameData.size())
				+ " ignored in total). Animations must start at frame 0.");
			return;
		}

		// Find the first frame that is not a single increment over the previous frame.
		auto it = frameData.begin();
		auto next = it;
		auto end = frameData.end();
		while(++next != end && next->first == it->first + 1)
			it = next;
		// Copy the sorted, valid paths from the map to the frame sequence vector.
		size_t count = distance(frameData.begin(), next);
		sequence.resize(count);
		transform(frameData.begin(), next, sequence.begin(),
			[](const pair<size_t, filesystem::path> &p) -> filesystem::path { return p.second; });

		// If `next` is not the end, then there was at least one discontinuous frame.
		if(next != frameData.end())
		{
			size_t ignored = distance(next, frameData.end());
			Logger::LogError(prefix + "missing " + (isSwizzleMask ? "mask " : "") + (is2x ? "@2x " : "") + "frame "
				+ to_string(it->first + 1) + " (" + to_string(ignored)
				+ (ignored > 1 ? " frames" : " frame") + " ignored in total).");
		}
	}
}



// Check if the given path is to an image of a valid file type.
bool ImageSet::IsImage(const filesystem::path &path)
{
	filesystem::path ext = path.extension();
	return ImageBuffer::ImageExtensions().contains(Format::LowerCase(ext.string()));
}



// Determine whether the given path or name is for a sprite whose loading
// should be deferred until needed.
bool ImageSet::IsDeferred(const filesystem::path &path)
{
	if(path.empty())
		return false;
	return *path.begin() == "land";
}



ImageSet::ImageSet(string name)
	: name(std::move(name))
{
}



// Get the name of the sprite for this image set.
const string &ImageSet::Name() const
{
	return name;
}



// Whether this image set is empty, i.e. has no images.
bool ImageSet::IsEmpty() const
{
	return framePaths[0].empty() && framePaths[1].empty();
}



// Add a single image to this set. Assume the name of the image has already
// been checked to make sure it belongs in this set.
void ImageSet::Add(ImageFileData data)
{
	// Determine which frame of the sprite this image will be.
	// Store the requested path.
	framePaths[data.is2x + (2 * data.isSwizzleMask)][data.frameNumber].swap(data.path);
}



// Reduce all given paths to frame images into a sequence of consecutive frames.
void ImageSet::ValidateFrames() noexcept(false)
{
	string prefix = "Sprite \"" + name + "\": ";
	AddValid(framePaths[0], paths[0], prefix, false, false);
	AddValid(framePaths[1], paths[1], prefix, true, false);
	AddValid(framePaths[2], paths[2], prefix, false, true);
	AddValid(framePaths[3], paths[3], prefix, true, true);
	framePaths[0].clear();
	framePaths[1].clear();
	framePaths[2].clear();
	framePaths[3].clear();

	auto DropPaths = [&](vector<filesystem::path> &toResize, const string &specifier) {
		if(toResize.size() > paths[0].size())
		{
			Logger::LogError(prefix + to_string(toResize.size() - paths[0].size())
				+ " extra frames for the " + specifier + " sprite will be ignored.");
			toResize.resize(paths[0].size());
		}
	};

	// Drop any @2x and mask paths that will not be used.
	DropPaths(paths[1], "@2x");
	DropPaths(paths[2], "mask");
	DropPaths(paths[3], "@2x mask");
}



// Load all the frames. This should be called in one of the image-loading
// worker threads. This also generates collision masks if needed.
void ImageSet::Load() noexcept(false)
{
	assert(framePaths[0].empty() && "should call ValidateFrames before calling Load");

	// Determine how many frames there will be, total. The image buffers will
	// not actually be allocated until the first image is loaded (at which point
	// the sprite's dimensions will be known).
	size_t frames = paths[0].size();
	buffer[0].Clear(frames);
	buffer[1].Clear(frames);
	buffer[2].Clear(frames);
	buffer[3].Clear(frames);

	// Check whether we need to generate collision masks.
	bool makeMasks = IsMasked(name);
	if(makeMasks)
		masks.resize(frames);

	// Load the 1x sprites first, then the 2x sprites, because they are likely
	// to be in separate locations on the disk. Create masks if needed.
	for(size_t i = 0; i < frames; ++i)
	{
		if(!buffer[0].Read(paths[0][i], i))
			Logger::LogError("Failed to read image data for \"" + name + "\" frame #" + to_string(i));
		else if(makeMasks)
		{
			masks[i].Create(buffer[0], i);
			if(!masks[i].IsLoaded())
				Logger::LogError("Failed to create collision mask for \"" + name + "\" frame #" + to_string(i));
		}
	}

	auto FillSwizzleMasks = [&](vector<filesystem::path> &toFill, unsigned int intendedSize) {
		if(toFill.size() == 1 && intendedSize > 1)
			for(unsigned int i = toFill.size(); i < intendedSize; i++)
				toFill.emplace_back(toFill.back());
	};
	// If there is only a swizzle-mask defined for the first frame fill up the swizzle-masks
	// with this mask.
	FillSwizzleMasks(paths[2], paths[0].size());
	FillSwizzleMasks(paths[3], paths[0].size());


	auto LoadSprites = [&](vector<filesystem::path> &toLoad, ImageBuffer &buffer, const string &specifier) {
		for(size_t i = 0; i < frames && i < toLoad.size(); ++i)
			if(!buffer.Read(toLoad[i], i))
			{
				Logger::LogError("Removing " + specifier + " frames for \"" + name + "\" due to read error");
				buffer.Clear();
				break;
			}
	};
	// Now, load the mask and 2x sprites, if they exist. Because the number of 1x frames
	// is definitive, don't load any frames beyond the size of the 1x list.
	LoadSprites(paths[1], buffer[1], "@2x");
	LoadSprites(paths[2], buffer[2], "mask");
	LoadSprites(paths[3], buffer[3], "@2x mask");

	// Warn about a "high-profile" image that will be blurry due to rendering at 50% scale.
	bool willBlur = (buffer[0].Width() & 1) || (buffer[0].Height() & 1);
	if(willBlur && (
			(name.length() > 5 && !name.compare(0, 5, "ship/"))
			|| (name.length() > 7 && !name.compare(0, 7, "outfit/"))
			|| (name.length() > 10 && !name.compare(0, 10, "thumbnail/"))
	))
		Logger::LogError("Warning: image \"" + name + "\" will be blurry since width and/or height are not even ("
			+ to_string(buffer[0].Width()) + "x" + to_string(buffer[0].Height()) + ").");
}



// Create the sprite and optionally upload the image data to the GPU. After this is
// called, the internal image buffers and mask vector will be cleared, but
// the paths are saved in case the sprite needs to be loaded again.
void ImageSet::Upload(Sprite *sprite, bool enableUpload)
{
	// Clear all the buffers if we are not uploading the image data.
	if(!enableUpload)
		for(ImageBuffer &it : buffer)
			it.Clear();

	// Load the frames (this will clear the buffers).
	sprite->AddFrames(buffer[0], false);
	sprite->AddFrames(buffer[1], true);
	sprite->AddSwizzleMaskFrames(buffer[2], false);
	sprite->AddSwizzleMaskFrames(buffer[3], true);

	GameData::GetMaskManager().SetMasks(sprite, std::move(masks));
	masks.clear();
}
