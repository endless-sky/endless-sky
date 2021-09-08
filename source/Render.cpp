/* Render.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "Render.h"

#include "Audio.h"
#include "BatchShader.h"
#include "Color.h"
#include "Command.h"
#include "Conversation.h"
#include "DataFile.h"
#include "DataNode.h"
#include "DataWriter.h"
#include "Effect.h"
#include "Files.h"
#include "FillShader.h"
#include "Fleet.h"
#include "FogShader.h"
#include "text/FontSet.h"
#include "Galaxy.h"
#include "GameData.h"
#include "GameEvent.h"
#include "GameObjects.h"
#include "Government.h"
#include "Hazard.h"
#include "ImageSet.h"
#include "Interface.h"
#include "LineShader.h"
#include "Minable.h"
#include "Mission.h"
#include "Music.h"
#include "News.h"
#include "Outfit.h"
#include "OutlineShader.h"
#include "Person.h"
#include "Phrase.h"
#include "Planet.h"
#include "PointerShader.h"
#include "Politics.h"
#include "Random.h"
#include "RingShader.h"
#include "Ship.h"
#include "Sprite.h"
#include "SpriteQueue.h"
#include "SpriteSet.h"
#include "SpriteShader.h"
#include "StarField.h"
#include "StartConditions.h"
#include "System.h"
#include "Test.h"
#include "TestData.h"

#include <algorithm>
#include <iostream>
#include <map>
#include <set>
#include <utility>
#include <vector>

class Sprite;

using namespace std;

namespace {
	StarField background;
	SpriteQueue spriteQueue;

	// Whether sprites and audio have finished loading at game startup.
	bool initiallyLoaded = false;

	map<const Sprite *, shared_ptr<ImageSet>> deferred;
	map<const Sprite *, int> preloaded;


	map<string, shared_ptr<ImageSet>> FindImages()
	{
		map<string, shared_ptr<ImageSet>> images;
		for(const string &source : GameData::Sources())
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



void Render::Load()
{
	// Now, read all the images in all the path directories. For each unique
	// name, only remember one instance, letting things on the higher priority
	// paths override the default images.
	map<string, shared_ptr<ImageSet>> images = FindImages();

	// From the name, strip out any frame number, plus the extension.
	for(const auto &it : images)
	{
		// This should never happen, but just in case:
		if(!it.second)
			continue;

		// Reduce the set of images to those that are valid.
		it.second->ValidateFrames();
		// For landscapes, remember all the source files but don't load them yet.
		if(ImageSet::IsDeferred(it.first))
			deferred[SpriteSet::Get(it.first)] = it.second;
		else
			spriteQueue.Add(it.second);
	}

	// Generate a catalog of music files.
	Music::Init(GameData::Sources());
}



void Render::LoadShaders(bool useShaderSwizzle)
{
	FontSet::Add(Files::Images() + "font/ubuntu14r.png", 14);
	FontSet::Add(Files::Images() + "font/ubuntu18r.png", 18);

	// Load the key settings.
	Command::LoadSettings(Files::Resources() + "keys.txt");
	Command::LoadSettings(Files::Config() + "keys.txt");

	FillShader::Init();
	FogShader::Init();
	LineShader::Init();
	OutlineShader::Init();
	PointerShader::Init();
	RingShader::Init();
	SpriteShader::Init(useShaderSwizzle);
	BatchShader::Init();

	background.Init(16384, 4096);
}



double Render::Progress()
{
	auto progress = min(spriteQueue.Progress(), Audio::GetProgress());
	if(progress == 1.)
	{
		if(!initiallyLoaded)
		{
			// Now that we have finished loading all the basic sprites and sounds, we can look for invalid file paths,
			// e.g. due to capitalization errors or other typos.
			SpriteSet::CheckReferences();
			Audio::CheckReferences();
			initiallyLoaded = true;
		}
	}
	return progress;
}



bool Render::IsLoaded()
{
	return initiallyLoaded;
}



// Begin loading a sprite that was previously deferred. Currently this is
// done with all landscapes to speed up the program's startup.
void Render::Preload(const Sprite *sprite)
{
	// Make sure this sprite actually is one that uses deferred loading.
	auto dit = deferred.find(sprite);
	if(!sprite || dit == deferred.end())
		return;

	// If this sprite is one of the currently loaded ones, there is no need to
	// load it again. But, make note of the fact that it is the most recently
	// asked-for sprite.
	map<const Sprite *, int>::iterator pit = preloaded.find(sprite);
	if(pit != preloaded.end())
	{
		for(pair<const Sprite * const, int> &it : preloaded)
			if(it.second < pit->second)
				++it.second;

		pit->second = 0;
		return;
	}

	// This sprite is not currently preloaded. Check to see whether we already
	// have the maximum number of sprites loaded, in which case the oldest one
	// must be unloaded to make room for this one.
	const string &name = sprite->Name();
	pit = preloaded.begin();
	while(pit != preloaded.end())
	{
		++pit->second;
		if(pit->second >= 20)
		{
			spriteQueue.Unload(name);
			pit = preloaded.erase(pit);
		}
		else
			++pit;
	}

	// Now, load all the files for this sprite.
	preloaded[sprite] = 0;
	spriteQueue.Add(dit->second);
}



void Render::FinishLoading()
{
	spriteQueue.Finish();
}



const StarField &Render::Background()
{
	return background;
}



void Render::SetHaze(const Sprite *sprite, bool allowAnimation)
{
	background.SetHaze(sprite, allowAnimation);
}



void Render::LoadPlugin(const string &path, const string &name)
{
	// Create an image set for the plugin icon.
	auto icon = make_shared<ImageSet>(name);

	// Try adding all the possible icon variants.
	if(Files::Exists(path + "icon.png"))
		icon->Add(path + "icon.png");
	else if(Files::Exists(path + "icon.jpg"))
		icon->Add(path + "icon.jpg");

	if(Files::Exists(path + "icon@2x.png"))
		icon->Add(path + "icon@2x.png");
	else if(Files::Exists(path + "icon@2x.jpg"))
		icon->Add(path + "icon@2x.jpg");

	icon->ValidateFrames();
	spriteQueue.Add(icon);
}
