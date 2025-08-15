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

#include <memory>



/// Audio players can play audio from a single supplier at a varying volume, supporting pause/resume functionality.
/// This class contains a base implementation suitable for playback of sound effects;
/// it is meant to be inherited by specialized classes for other purposes.
class AudioPlayer {
public:
	/// Creates a new audio player with the given audio.
	/// Please note that the audio isn't loaded from the supplier until the Play() call.
	AudioPlayer(SoundCategory category, std::unique_ptr<AudioSupplier> audioSupplier, bool spatial = true);
	virtual ~AudioPlayer();

	/// Updates the queued music of the player. This step may also mark the player as finished.
	virtual void Update();

	/// Checks whether the player is finished. Finished players will not be able to play audio again,
	/// and should not be stored.
	bool IsFinished() const;

	/// The volume of the playback, or 0 if the player is not initialized.
	double Volume() const;
	/// Sets the volume of the player, if it is initialized.
	void SetVolume(double level) const;

	/// Moves the sound to the specified point in 3D. Ignored if the player is not initialized.
	virtual void Move(double x, double y, double z) const;

	/// The category of the sound being played.
	SoundCategory Category() const;

	virtual void Pause() const;
	virtual void Play() const;

	/// Acquires the source for this player, and loads the initial buffers. Does not begin playback.
	virtual void Init();
	/// Instructs the player to stop. No new buffers will be queued, but queued buffers will finish playback.
	/// Until the player is marked finished, calling this function with 'false' can undo its effect.
	void Stop(bool stop = true);

	/// The supplier of the player. Never null.
	AudioSupplier *Supplier();


protected:
	/// Acquires a new source, if there isn't one already.
	bool ClaimSource();
	/// Releases the current source, making it available for other audio players.
	void ReleaseSource();

	/// Configures a source for the first time after being claimed.
	virtual void ConfigureSource();


protected:
	/// The maximum number of buffers to queue up synchronously when the player is initialized.
	static constexpr size_t MAX_INITIAL_BUFFERS = 3;

	SoundCategory category;
	bool spatial;

	/// The configured source, or 0.
	ALuint alSource = 0;
	/// The data supplier; never null.
	std::unique_ptr<AudioSupplier> audioSupplier;

	/// Whether the player has terminated.
	bool done = false;
	/// Whether the player should stop queueing up more buffers (and terminate, once they all run out).
	bool shouldStop = false;
};
