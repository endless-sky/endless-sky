/* GameAssets.cpp
Copyright (c) 2022 by quyykk

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "GameAssets.h"

#include "Files.h"
#include "ImageSet.h"
#include "MaskManager.h"
#include "Sprite.h"

#include <map>
#include <memory>
#include <utility>

using namespace std;



namespace {
	map<string, shared_ptr<ImageSet>> FindImages(const vector<string> &sources)
	{
		auto images = map<string, shared_ptr<ImageSet>>{};
		for(const string &source : sources)
		{
			// All names will only include the portion of the path that comes after
			// this directory prefix.
			string directoryPath = source + "images/";
			size_t start = directoryPath.size();

			vector<string> imageFiles = Files::RecursiveList(directoryPath);
			for(string &path : imageFiles)
				if(ImageSet::IsImage(path))
				{
					string name = ImageSet::Name(path.substr(start));

					shared_ptr<ImageSet> &imageSet = images[name];
					if(!imageSet)
						imageSet.reset(new ImageSet(name));
					imageSet->Add(std::move(path));
				}
		}
		return images;
	}
}



GameAssets::GameAssets() noexcept
	: soundQueue(sounds), spriteQueue(sprites)
{
}



future<void> GameAssets::Load(const vector<string> &sources, int options)
{
	// Start loading the data files.
	auto wait = objects.Load(sources, options & Debug);

	if(!(options & OnlyData))
	{
		LoadImages(sources);
		LoadSounds(sources);
	}

	return wait;
}



double GameAssets::GetProgress() const
{
	// Cache progress completion seen, so clients are
	// isolated from the loading implementation details.
	static bool initiallyLoaded = false;
	if(initiallyLoaded)
		return 1.;

	double val = min(min(spriteQueue.GetProgress(), soundQueue.GetProgress()), objects.GetProgress());
	if(val >= 1.)
		initiallyLoaded = true;
	return val;
}



void GameAssets::LoadImages(const std::vector<std::string> &sources)
{
	// Now, read all the images in all the path directories. For each unique
	// name, only remember one instance, letting things on the higher priority
	// paths override the default images.
	// From the name, strip out any frame number, plus the extension.
	for(const auto &it : FindImages(sources))
	{
		// This should never happen, but just in case:
		if(!it.second)
			continue;

		// Reduce the set of images to those that are valid.
		it.second->ValidateFrames();
		spriteQueue.Add(it.second);
	}
}



void GameAssets::LoadSounds(const std::vector<std::string> &sources)
{
	for(const string &source : sources)
	{
		string root = source + "sounds/";
		vector<string> files = Files::RecursiveList(root);

		for(const string &path : files)
		{
			// Sanity check on the path length.
			if(path.length() < root.length() + 4)
				continue;
			string ext = path.substr(path.length() - 4);

			// Music sound files are loaded when needed.
			if(ext == ".mp3" || ext == ".MP3")
			{
				string name = path.substr(root.length(), path.length() - root.length() - 4);
				music[name] = path;
			}
			// Regular sound files are loaded into memory for faster access.
			else if(ext == ".wav" || ext == ".WAV")
			{
				// The "name" of the sound is its full path within the "sounds/"
				// folder, without the ".wav" or "~.wav" suffix.
				size_t end = path.length() - 4;
				if(path[end - 1] == '~')
					--end;
				soundQueue.Add({path, path.substr(root.length(), end - root.length())});
			}
		}
	}
}
