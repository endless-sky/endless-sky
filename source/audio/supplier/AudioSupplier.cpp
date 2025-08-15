/* AudioSupplier.cpp
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

#include "AudioSupplier.h"

using namespace std;



AudioSupplier::AudioSupplier(bool is3x, bool isLooping)
	: is3x(is3x), nextPlaybackIs3x(is3x), isLooping(isLooping)
{
}



void AudioSupplier::Set3x(bool is3x)
{
	nextPlaybackIs3x = is3x;
}



void AudioSupplier::NextChunk(ALuint buffer)
{
	if(AvailableChunks())
	{
		vector<sample_t> samples = NextDataChunk();
		alBufferData(buffer, FORMAT, samples.data(), sizeof(sample_t) * OUTPUT_CHUNK, SAMPLE_RATE);
	}
	else
		SetSilence(buffer, OUTPUT_CHUNK);
}



ALuint AudioSupplier::CreateBuffer()
{
	ALuint buffer;
	alGenBuffers(1, &buffer);
	return buffer;
}



void AudioSupplier::DestroyBuffer(ALuint buffer)
{
	alDeleteBuffers(1, &buffer);
}



void AudioSupplier::SetSilence(ALuint buffer, size_t frames)
{
	vector<sample_t> data(frames);
	alBufferData(buffer, FORMAT, data.data(), sizeof(sample_t) * frames, SAMPLE_RATE);
}
