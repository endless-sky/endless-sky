/* WavSupplier.h
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

#include "AudioSupplier.h"

class Sound;



/// A sync buffered supplier for waveform files. The audio is supplied in a single chunk.
class WavSupplier : public AudioSupplier {
public:
	WavSupplier(const Sound& sound, bool is3x, bool looping = false);

	// Inherited pure virtual methods
	ALsizei MaxChunkCount() const override;
	int AvailableChunks() const override;
	bool IsSynchronous() const override;
	bool NextChunk(ALuint buffer) override;
	ALuint AwaitNextChunk() override;
	void ReturnBuffer(ALuint buffer) override;

private:
	const Sound& sound;
	const bool looping;
	bool wasBufferGiven;
};
