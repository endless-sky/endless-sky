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



void Fade::AddSource(unique_ptr<AudioSupplier> source, size_t fadePerFrame)
{
	// Don't allow a slower fade than the default.
	if(fadePerFrame < 1)
		fadePerFrame = 1;

	if(primarySource)
		fadeProgress.emplace_back(std::move(primarySource), MAX_FADE, fadePerFrame);
	primarySource = std::move(source);
}



void Fade::Set3x(bool is3x)
{
	AudioSupplier::Set3x(is3x);
	if(primarySource)
		primarySource->Set3x(is3x);
	for(const auto &[source, fade, fadePerFrame] : fadeProgress)
		source->Set3x(is3x);
}



size_t Fade::MaxChunks() const
{
	size_t count = primarySource ? primarySource->MaxChunks() : 0;
	for(const auto &[supplier, fade, fadePerFrame] : fadeProgress)
		count = max(supplier->MaxChunks(), count);

	return count;
}



size_t Fade::AvailableChunks() const
{
	size_t count = primarySource ? primarySource->AvailableChunks() : MaxChunks();
	for(const auto &[supplier, fade, fadePerFrame] : fadeProgress)
		count = min(supplier->AvailableChunks(), count);

	return count;
}



vector<AudioSupplier::sample_t> Fade::NextDataChunk()
{
	vector<sample_t> result;
	if(!primarySource && fadeProgress.empty())
		// With no input sources, output silence.
		result = vector<sample_t>(OUTPUT_CHUNK);
	else if(primarySource && fadeProgress.empty())
		// With only primary input (nothing to blend with), output primary.
		result = primarySource->NextDataChunk();
	else // fade sources
	{
		// Generate the faded background.
		vector<sample_t> faded = std::get<0>(fadeProgress[0])->NextDataChunk();
		for(size_t i = 1; i < fadeProgress.size(); ++i)
		{
			vector<sample_t> other = std::get<0>(fadeProgress[i])->NextDataChunk();
			auto &[source, fade, fadePerFrame] = fadeProgress[i - 1];
			CrossFade(faded, other, fade, fadePerFrame);
			faded = std::move(other);
		}

		// Get the foreground data.
		if(primarySource)
			result = primarySource->NextDataChunk();
		else
			result.resize(OUTPUT_CHUNK); // silence

		// The final blend.
		auto &[source, fade, fadePerFrame] = fadeProgress.back();
		CrossFade(faded, result, fade, fadePerFrame);
	}

	// Clean up the finished sources.
	if(primarySource && !primarySource->MaxChunks())
		primarySource.reset();
	erase_if(fadeProgress, [](const auto &faded){ return !std::get<1>(faded) || !std::get<0>(faded)->MaxChunks(); });

	return result;
}



void Fade::CrossFade(const vector<sample_t> &fadeOut, vector<sample_t> &fadeIn, size_t &fade, size_t fadePerFrame)
{
	for(size_t i = 0; i < fadeIn.size() && fade; ++i)
	{
		fadeIn[i] = (fadeOut[i] * fade + (fadeIn[i] * (MAX_FADE - fade))) / MAX_FADE;
		if(fade > fadePerFrame)
			fade -= fadePerFrame;
		else
			fade = 0;
	}
}
