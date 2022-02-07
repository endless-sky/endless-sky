/* SpriteQueue.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "SpriteQueue.h"

#include "ImageSet.h"
#include "Sprite.h"
#include "SpriteSet.h"

using namespace std;



SpriteQueue::~SpriteQueue()
{
	tasks.cancel();
	tasks.wait();
}



// Add a sprite to load.
void SpriteQueue::Add(const shared_ptr<ImageSet> &images)
{
	tasks.run([this, images]
		{
			images->Load();
			++added;

			toLoad.push(images);
		});
}



// Unload the texture for the given sprite (to free up memory).
void SpriteQueue::Unload(const string &name)
{
	toUnload.push(name);
}



// Determine the fraction of sprites uploaded to the GPU.
double SpriteQueue::GetProgress() const
{
	// Special cases: we're bailing out, or we are done.
	if(added <= 0 || added == completed)
		return 1.;
	return static_cast<double>(completed) / static_cast<double>(added);
}



void SpriteQueue::UploadSprites()
{
	while(!toUnload.empty())
	{
		string image;
		if(!toUnload.try_pop(image))
			break;

		SpriteSet::Modify(image)->Unload();
	}

	for(int i = 0; i < 100; ++i)
	{
		// Extract the one item we should work on uploading right now.
		shared_ptr<ImageSet> imageSet;
		if(!toLoad.try_pop(imageSet))
			break;

		imageSet->Upload(SpriteSet::Modify(imageSet->Name()));
		++completed;
	}
}



// Finish loading.
void SpriteQueue::Finish()
{
	tasks.wait();
}
