/* AudioSupplier.h
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

#include <AL/al.h>

#include <cstddef>
#include <cstdint>
#include <vector>



/// An audio supplier provides chunks of audio over time,
/// which can be requested via NextChunk() or NextDataChunk().
class AudioSupplier {
public:
	using sample_t = int16_t;

	static ALuint CreateBuffer();
	static void DestroyBuffer(ALuint buffer);


public:
	explicit AudioSupplier(bool is3x = false, bool isLooping = false);
	virtual ~AudioSupplier() = default;

	AudioSupplier(const AudioSupplier &) = delete;
	AudioSupplier(AudioSupplier &&) = default;

	/// The estimated number of non-silent chunks that can be supplied by further NextChunk()/NextDataChunk() calls.
	/// Never less than AvailableChunks(), and is always zero when the supplier can't provide new chunks anymore.
	virtual size_t MaxChunks() const = 0;
	/// The number of chunks currently ready for access via NextChunk().
	virtual size_t AvailableChunks() const = 0;
	/// Gets the next, fixed-size chunk of audio samples. If there is no available chunk, a silence chunk is returned.
	/// This returns the raw samples that would be put into an OpenAL buffer via a NextChunk() call.
	virtual std::vector<sample_t> NextDataChunk() = 0;

	/// Configures 3x audio playback.
	virtual void Set3x(bool is3x);

	/// Puts the next queued audio chunk into the buffer, removing it from the supplier's queue.
	/// If there is no queued audio, the buffer is filled with silence.
	void NextChunk(ALuint buffer, bool spatial);


protected:
	/// Sets the buffer to silence for the given number of samples.
	static void SetSilence(ALuint buffer, size_t samples);


public:
	static constexpr int SAMPLE_RATE = 44100;


protected:
	static constexpr ALenum FORMAT = AL_FORMAT_STEREO16;
	static constexpr ALenum FORMAT_SPATIAL = AL_FORMAT_MONO16;
	/// How many samples to put in each output chunk. Because the output is in
	/// stereo, the duration of the sample is half this amount, divided by the sample rate.
	/// This chunk size provides 5 in-game frames' worth of audio.
	static constexpr size_t OUTPUT_CHUNK = 1. / 60. * SAMPLE_RATE * 2 * 5;
	/// How many bytes to read from a file at a time
	static constexpr size_t INPUT_CHUNK = sizeof(sample_t) * 65536;

	/// Whether the current playback is using the 3x samples (if available).
	bool is3x = false;
	/// 3x status can only really change when the file is played from the beginning.
	/// This caches the status it should have after the next restart.
	bool nextPlaybackIs3x = false;
	/// A looping player will stream data forever.
	bool isLooping = false;

	/// The index of the first sample to be processed
	size_t currentSample = 0;
};
