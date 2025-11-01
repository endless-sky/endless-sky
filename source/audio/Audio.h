/* Audio.h
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

#include "SoundCategory.h"

#include <filesystem>
#include <string>
#include <vector>

class Point;
class Sound;



// This class is a collection of global functions for handling audio. A sound
// can be played from any point in the code, and from any thread, just by
// specifying the name of the sound to play. Most sounds will come from a
// "source" at a certain position, and their volume and left / right balance is
// adjusted based on how far they are from the observer. Sounds that are not
// marked as looping will play once, then stop; looping sounds continue until
// their source stops calling the "play" function for them.
class Audio {
public:
	// Begin loading sounds (in a separate thread).
	static void Init(const std::vector<std::filesystem::path> &sources);
	static void LoadSounds(const std::vector<std::filesystem::path> &sources);
	static void CheckReferences(bool parseOnly = false);

	// Report the progress of loading sounds.
	static double GetProgress();

	// Get or set the volume (between 0 and 1).
	static double Volume(SoundCategory category);
	static void SetVolume(double level, SoundCategory category);

	// Get a pointer to the named sound. The name is the path relative to the
	// "sound/" folder, and without ~ if it's on the end, or the extension.
	// Do not call this function until Progress() is 100%.
	static const Sound *Get(const std::string &name);

	// Set the listener's position, and also update any sounds that have been
	// added but deferred because they were added from a thread other than the
	// main one (the one that called Init()).
	static void Update(const Point &listenerPosition);

	// Play the given sound, at full volume.
	static void Play(const Sound *sound, SoundCategory category);

	// Play the given sound, as if it is at the given distance from the
	// "listener". This will make it softer and change the left / right balance.
	static void Play(const Sound *sound, const Point &position, SoundCategory category);

	// Play the given music. An empty string means to play nothing.
	static void PlayMusic(const std::string &name);

	// Pause all active sound sources. Doesn't cause new streams to be paused, and doesn't pause the music source.
	// Has no effect following a call to "Audio::BlockPausing" until "Audio::UnblockPausing" is called.
	static void Pause();
	// Resumes all paused sound sources. If Pause() was called multiple times,
	// you have to call Resume() the same number of times to resume the sound sources.
	// Has no effect following a call to "Audio::BlockPausing" until "Audio::UnblockPausing" is called.
	static void Resume();
	// While pausing is blocked, "Audio::Pause" and "Audio::resume" have no effect.
	static void BlockPausing();
	static void UnblockPausing();

	/// Begin playing all the sounds that have been added since the last time
	/// this function was called.
	/// If the game is in fast forward mode, the fast version of sounds is played.
	static void Step(bool isFastForward);

	// Shut down the audio system (because we're about to quit).
	static void Quit();
};
