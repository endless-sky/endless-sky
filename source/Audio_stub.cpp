/* Audio.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "Audio.h"

static double g_volume = 0;

// all stubs. I will figure out audio later. 

// Begin loading sounds (in a separate thread).
void Audio::Init(const std::vector<std::string> &) {}
void Audio::CheckReferences() {}

// Report the progress of loading sounds.
double Audio::GetProgress() { return 1.0; }

// Get or set the volume (between 0 and 1).
double Audio::Volume() { return g_volume; }
void Audio::SetVolume(double level) { g_volume = level; }

// Get a pointer to the named sound. The name is the path relative to the
// "sound/" folder, and without ~ if it's on the end, or the extension.
// Do not call this function until Progress() is 100%.
const Sound* Audio::Get(const std::string &name) { return nullptr; } // may need to return static uninit instance

// Set the listener's position, and also update any sounds that have been
// added but deferred because they were added from a thread other than the
// main one (the one that called Init()).
void Audio::Update(const Point &listenerPosition) {}

// Play the given sound, at full volume.
void Audio::Play(const Sound *sound) {}

// Play the given sound, as if it is at the given distance from the
// "listener". This will make it softer and change the left / right balance.
void Audio::Play(const Sound *sound, const Point &position) {}

// Play the given music. An empty string means to play nothing.
void Audio::PlayMusic(const std::string &name) {}

// Begin playing all the sounds that have been added since the last time
// this function was called.
void Audio::Step() {}

// Shut down the audio system (because we're about to quit).
void Audio::Quit() {}
