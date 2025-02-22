/* AudioDataSupplier.h
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

#include <cstdint>
#include <vector>



/// An audio data supplier can provide the raw samples as well as the OpenAL buffers.
/// All data suppliers use the same chunk size for easier post-processing.
class AudioDataSupplier : public AudioSupplier {
public:
	/// Gets the next, fixed-size chunk of audio samples.
	virtual std::vector<int16_t> NextDataChunk() = 0;


protected:
	/// How many samples to put in each output chunk. Because the output is in
	/// stereo, the duration of the sample is half this amount, divided by the sample rate.
	static constexpr size_t OUTPUT_CHUNK = 32768;
	/// How many bytes to read from a file at a time
	static constexpr size_t INPUT_CHUNK = sizeof(int16_t) * 65536;
};
