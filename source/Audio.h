/* Audio.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef AUDIO_H_
#define AUDIO_H_

#include <string>
#include <vector>

class Point;
class Sound;



// This class is a collection of global functions for handling audio. A sound
// can be played from any point in the code, and from any thread, just by
// specifying the name of the sound to play. Most sounds will come from a
// "source" at a certain position and velocity, and their volume is adjusted
// based on how far they are from the observer. Pitch is also adjusted (a
// Doppler shift) depending on whether they are moving toward the observer or
// away, and at what velocity. Sounds that are not marked as looping will play
// once, then stop; looping sounds continue until their source stops calling
// the "play" function for them.
class Audio {
public:
	// Begin loading sounds (in a separate thread).
	static void Init(const std::vector<std::string> &sources);
	
	// Check the progress of loading sounds.
	static double Progress();
	
	// Get or set the volume (between 0 and 1).
	static double Volume();
	static void SetVolume(double level);
	
	// Get a pointer to the named sound. The name is the path relative to the
	// "sound/" folder, and without ~ if it's on the end, or the extension.
	// Do not call this function until Progress() is 100%.
	static const Sound *Get(const std::string &name);
	
	// Set the listener's position, and also update any sounds that have been
	// added but deferred because they were added from a thread other than the
	// main one (the one that called Init()).
	static void Update(const Point &listenerPosition);
	
	// Play the given sound, at full volume.
	static void Play(const Sound *sound);
	
	// Play the given sound, as if it is at the given distance from the
	// "listener". This will make it softer and change the left / right balance.
	static void Play(const Sound *sound, const Point &position);
	
	// Begin playing all the sounds that have been added since the last time
	// this function was called.
	static void Step();
	
	// Shut down the audio system (because we're about to quit).
	static void Quit();
};



#endif
