/* WavSupplier.cpp
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

#include "WavSupplier.h"

#include "../Sound.h"

#include <algorithm>
#include <cmath>

using namespace std;



WavSupplier::WavSupplier(const Sound &sound, bool is3x, bool looping)
	: AudioSupplier(is3x, looping), sound(sound), wasStarted(false)
{
}



size_t WavSupplier::MaxChunks() const
{
	if(isLooping)
		return 2;
	else if(wasStarted && !currentSample)
		return 0;
	else
		return ceil(((is3x ? sound.Buffer3x() : sound.Buffer()).size() - currentSample) / static_cast<float>(OUTPUT_CHUNK));
}



size_t WavSupplier::AvailableChunks() const
{
	return MaxChunks();
}



vector<AudioSupplier::sample_t> WavSupplier::NextDataChunk()
{
	vector<sample_t> samples(OUTPUT_CHUNK);
	// If we are at the beginning of the buffer and it was already played, this is a loop.
	if(!currentSample && wasStarted && !isLooping)
		return samples;

	size_t currentSampleCount = 0;
	do {
		// If restarting the buffer, check 3x status.
		if(!currentSample)
		{
			is3x = nextPlaybackIs3x;
			wasStarted = true;
		}
		const vector<sample_t> &input = is3x ? sound.Buffer3x() : sound.Buffer();
		size_t readChunk = min(input.size() - currentSample, samples.size() - currentSampleCount);
		std::copy_n(input.begin() + currentSample, readChunk, samples.begin() + currentSampleCount);
		currentSampleCount += readChunk;
		currentSample = (currentSample + readChunk) % input.size();
	} while(currentSampleCount < samples.size() && isLooping);
	return samples;
}

