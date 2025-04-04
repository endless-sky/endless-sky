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
#include <AL/alc.h>

#include <cstdint>
#include <vector>



/// An audio supplier provides chunks of audio over time.
/// There are two main ways they operate: synchronously or asynchronously.
/// Synchronous suppliers provide audio samples on the fly, via AwaitNextChunk().
/// Asynchronous suppliers generate audio in the background, which can be requested via NextChunk().
/// Though both supplier types support both interfaces, the most efficient use is as described above.
/// Mixed usage may result in extra chunks of silence.
class AudioSupplier {
public:
	explicit AudioSupplier(bool is3x = false);
	virtual ~AudioSupplier() = default;

	AudioSupplier(const AudioSupplier&) = delete;
	AudioSupplier(AudioSupplier&&) = default;

	/// The estimated number of non-silent chunks that can be supplied by further NextChunk() or AwaitNextChunk() calls.
	/// Never less than AvailableChunks(), and is always zero when the supplier can't provide new chunks anymore.
	virtual ALsizei MaxChunkCount() const = 0;
	/// The number of chunks currently ready for access via NextChunk() or AwaitNextChunk().
	virtual int AvailableChunks() const = 0;

	/// Configures 3x audio playback. Some suppliers may choose to ignore this setting.
	virtual void Set3x(bool is3x);

	/// Whether the preferred chunk acquisition is AwaitNextChunk() or NextChunk().
	virtual bool IsSynchronous() const = 0;

	/// Puts the next queued audio chunk into the buffer, removing it from the supplier's queue.
	/// If there is no queued audio, the buffer is filled with silence.
	/// The buffer must be a buffer returned by the AwaitNextChunk() call of this supplier.
	/// A sync supplier without caching may return a silent buffer.
	/// This method should not be called if MaxChunkCount() is 0.
	/// Returns true if real sound was provided instead of silence.
	virtual bool NextChunk(ALuint buffer) = 0;
	/// Returns an audio chunk in a buffer. For an async supplier, a silent buffer may be returned.
	/// This method should not be called if MaxChunkCount() is 0.
	virtual ALuint AwaitNextChunk() = 0;
	/// Returns a buffer to the supplier, for reuse in AwaitNextChunk().
	/// The caller may not use this buffer in NextChunk() afterwards.
	virtual void ReturnBuffer(ALuint buffer) = 0;


protected:
	/// Sets the buffer to silence for the given number of frames.
	void SetSilence(ALuint buffer, size_t frames);


protected:
	static constexpr int SAMPLE_RATE = 44100;
	static constexpr ALenum FORMAT = AL_FORMAT_STEREO16;

	bool is3x = false;
};
