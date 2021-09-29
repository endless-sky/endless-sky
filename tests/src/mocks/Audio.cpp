/* Audio.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "../../../source/Audio.h"

#include "../../../source/Sound.h"

#include <map>
#include <string>
#include <vector>

using namespace std;

namespace {
	// Sound resources that have been loaded from files.
	map<string, Sound> sounds;
}



// Begin loading sounds (in a separate thread).
void Audio::Init(const vector<string> &sources)
{
}



void Audio::CheckReferences()
{
}



// Report the progress of loading sounds.
double Audio::GetProgress()
{
	return 1.;
}



// Get the volume.
double Audio::Volume()
{
	return 1.;
}



// Set the volume (to a value between 0 and 1).
void Audio::SetVolume(double level)
{
}



// Get a pointer to the named sound. The name is the path relative to the
// "sound/" folder, and without ~ if it's on the end, or the extension.
const Sound *Audio::Get(const string &name)
{
	return &sounds[name];
}



// Set the listener's position, and also update any sounds that have been
// added but deferred because they were added from a thread other than the
// main one (the one that called Init()).
void Audio::Update(const Point &listenerPosition)
{
}



// Play the given sound, at full volume.
void Audio::Play(const Sound *sound)
{
}



// Play the given sound, as if it is at the given distance from the
// "listener". This will make it softer and change the left / right balance.
void Audio::Play(const Sound *sound, const Point &position)
{
}



// Play the given music. An empty string means to play nothing.
void Audio::PlayMusic(const string &name)
{
}



// Begin playing all the sounds that have been added since the last time
// this function was called.
void Audio::Step()
{
}



// Shut down the audio system (because we're about to quit).
void Audio::Quit()
{
}
