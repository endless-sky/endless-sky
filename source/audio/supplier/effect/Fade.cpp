/* Fade.cpp
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

#include "Fade.h"

using namespace std;



void Fade::AddSource(unique_ptr<AudioDataSupplier> source, size_t fade)
{
	fade = min(fade, MAX_FADE); // don't allow a slower fade than the default

	if(primarySource && fade)
		fadeProgress.emplace_back(std::move(primarySource), fade);
	primarySource = std::move(source);
}


ALsizei Fade::MaxChunkCount() const
{
	int count = primarySource ? primarySource->MaxChunkCount() : 1;
	for(const auto &[supplier, fade] : fadeProgress)
		count = max(supplier->MaxChunkCount(), count);

	return max(1, count);
}



int Fade::AvailableChunks() const
{
	int count = primarySource ? primarySource->AvailableChunks() : MaxChunkCount();
	for(const auto &[supplier, fade] : fadeProgress)
		count = min(supplier->AvailableChunks(), count);

	return count;
}



bool Fade::IsSynchronous() const
{
	return false;
}



bool Fade::NextChunk(ALuint buffer)
{
	vector<int16_t> next = NextDataChunk();
	alBufferData(buffer, FORMAT, next.data(), OUTPUT_CHUNK, SAMPLE_RATE);
	return true;
}



vector<int16_t> Fade::NextDataChunk()
{
	vector<int16_t> result;
	if(!primarySource && fadeProgress.empty()) // no sources -> silence
		result = vector<int16_t>(OUTPUT_CHUNK);
	else if(primarySource && fadeProgress.empty()) // only primary -> primary
		result = primarySource->NextDataChunk();
	else
	{
		// Generate the faded background
		vector<int16_t> faded = fadeProgress[0].first->NextDataChunk();
		for(size_t i = 1; i < fadeProgress.size(); i++)
		{
			vector<int16_t> other = fadeProgress[i].first->NextDataChunk();
			CrossFade(faded, other, fadeProgress[i - 1].second);
			faded = other;
		}

		// Get the foreground data
		if(primarySource)
			result = primarySource->NextDataChunk();
		else
			result.resize(OUTPUT_CHUNK); // silence

		// final blend
		CrossFade(faded, result, fadeProgress.back().second);
	}

	// Clean up the finished sources.
	if(primarySource && !primarySource->MaxChunkCount())
		primarySource.reset();
	erase_if(fadeProgress, [](const auto &pair){return !pair.second || !pair.first->MaxChunkCount();});

	return result;
}



ALuint Fade::AwaitNextChunk()
{
	ALuint buffer;
	alGenBuffers(1, &buffer);

	NextChunk(buffer);
	return buffer;
}


void Fade::ReturnBuffer(ALuint buffer)
{
	alDeleteBuffers(1, &buffer);
}



void Fade::CrossFade(const vector<int16_t> &fadeOut, vector<int16_t> &fadeIn, size_t &fade)
{
	for(size_t i = 0; i < fadeIn.size() && fade; ++i)
	{
		fadeIn[i] = (fadeOut[i] * fade + (fadeIn[i] * (MAX_FADE - fade))) / fade;
		--fade;
	}
}
