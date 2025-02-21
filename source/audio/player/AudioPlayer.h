/* AudioPlayer.h
Copyright (c) 2025 by tibetiroka

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

#include "../supplier/AudioSupplier.h"
#include "../SoundCategory.h"

#include <AL/al.h>
#include <AL/alc.h>

#include <memory>
#include <vector>


/// Audio players can play audio from a single supplier at a varying volume, supporting pause/resume functionality.
/// This class contains a base implementation suitable for playback of sound effects;
/// it is meant to be inherited by specialized classes for other purposes.
class AudioPlayer {
public:
	/// Creates a new audio player with the given audio.
	/// Please note that the audio isn't loaded from the supplier until the Play() call.
	AudioPlayer(SoundCategory category, std::unique_ptr<AudioSupplier> audioSupplier);
	virtual ~AudioPlayer();

	/// Updates the queued music of the player. This step may also mark the player as finished.
	virtual void Update();

	/// Checks whether the player is finished. Finished players will not be able to play audio again,
	/// and should not be stored.
	bool IsFinished() const;

	/// The volume of the playback, or 0 if the player has not started yet.
	double Volume() const;
	///
	void SetVolume(double level) const;

	virtual void Move(double x, double y, double z) const;

	SoundCategory Category() const;

	virtual void Pause() const;
	virtual void Play() const;

	/// Acquires the source for this player, and loads the initial buffers. Does not begin playback.
	virtual void Init();
	/// Instructs the player to stop. No new buffers will be queued, but queued buffers will finish playback.
	/// Until the player is marked finished, calling this function with 'false' can undo its effect.
	void Stop(bool stop = true);

	AudioSupplier *Supplier();


protected:
	bool ClaimSource();
	void ReleaseSource();

	virtual void ConfigureSource();


protected:
	static constexpr int MAX_INITIAL_BUFFERS = 3;

	SoundCategory category;

	/// The configured source, or 0
	ALuint alSource = 0;
	std::unique_ptr<AudioSupplier> audioSupplier;

	bool done = false;
	bool shouldStop = false;


private:
	static std::vector<ALuint> availableSources;
};
