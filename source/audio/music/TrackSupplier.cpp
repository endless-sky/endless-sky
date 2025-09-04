/* TrackSupplier.h
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

#include "TrackSupplier.h"

#include "../supplier/effect/Fade.h"

#include <algorithm>
#include <cmath>
#include <limits>

using namespace std;

namespace
{
	size_t Available(const vector<unique_ptr<AudioSupplier>> &suppliers)
	{
		size_t available = suppliers.empty() ? 0 : suppliers.front()->AvailableChunks();
		for(const auto &supplier : suppliers)
			available = min(available, supplier->AvailableChunks());
		return available;
	}

	size_t Remaining(const vector<unique_ptr<AudioSupplier>> &suppliers)
	{
		size_t remaining = suppliers.empty() ? 0 : suppliers.front()->MaxChunks();
		for(const auto &supplier : suppliers)
			remaining = max(remaining, supplier->MaxChunks());
		return remaining;
	}
}


TrackSupplier::TrackSupplier() : AsyncAudioSupplier({})
{
}


const Track *TrackSupplier::GetCurrentTrack() const
{
	lock_guard guard(lock);
	return current;
}


const Track *TrackSupplier::GetNextTrack() const
{
	lock_guard guard(lock);
	return next;
}



const TrackSupplier::SwitchPriority TrackSupplier::GetNextTrackPriority() const
{
	lock_guard guard(lock);
	return nextPriority;
}



void TrackSupplier::SetNextTrack(const Track* track, SwitchPriority priority, bool loop, bool sync)
{
	lock_guard guard(lock);
	if(track == current)
	{
		// Special case: switching back to the current track.
		// Just clear any previous change requests.
		next = nullptr;
		nextPriority = SwitchPriority::END_OF_TRACK;
		nextIsLooping = false;
		nextIsSynced = false;
	}
	else
	{
		next = track;
		nextPriority = priority;
		nextIsLooping = loop;
		nextIsSynced = sync && track;
	}
}



void TrackSupplier::Decode()
{
	// The audio suppliers used for playback.
	// The first one is a Fade used for the background,
	// which is preserved when switching tracks.
	vector<unique_ptr<AudioSupplier>> suppliers;
	suppliers.emplace_back(make_unique<Fade>());
	const AudioSupplier *currentBackground = nullptr;
	// Preload the suppliers for the next track.
	vector<unique_ptr<AudioSupplier>> nextSuppliers;
	const Track* cachedNext = nullptr;
	// How many iterations the next track was scheduled for.
	int nextCounter = 0;
	// How many iterations to wait before switching to a "preferred" track. Chosen arbitrarily.
	constexpr int PREFERRED_COUNTER_LIMIT = 50;
	constexpr int SILENCE_PREFERRED_COUNTER_LIMIT = 10;

	while(!done)
	{
		AwaitBufferSpace();
		if(done)
		{
			PadBuffer();
			break;
		}

		// Validate the cached suppliers.
		{
			lock_guard guard(lock);
			if(cachedNext != next)
			{
				cachedNext = next;
				nextSuppliers.clear();
				nextCounter = 0;
				if(next)
				{
					auto layers = next->Layers();
					if(!layers.empty())
					{
						nextSuppliers.emplace_back(layers.front().get().CreateSupplier(looping));
						for(size_t i = 1; i < layers.size(); ++i)
							nextSuppliers.emplace_back(layers[i].get().CreateSupplier(nextIsLooping));
					}
				}
			}
			if(nextPriority == SwitchPriority::PREFERRED)
				++nextCounter;
			else
				nextCounter = 0;
		}

		// Switch to the next supplier, if this one is exhausted.
		// If the switch is forced, only switch once the cached chunks run out.
		// This helps to reduce accidental switching when travelling between systems.
		bool wantsToChange = false;
		{
			lock_guard guard(lock);
			bool shouldChange = Available(nextSuppliers) || !next;
			switch(nextPriority)
			{
				case SwitchPriority::IMMEDIATE:
					wantsToChange = true;
					break;
				case SwitchPriority::PREFERRED:
					wantsToChange = nextCounter >= ((next && current) ? PREFERRED_COUNTER_LIMIT : SILENCE_PREFERRED_COUNTER_LIMIT);
					break;
				case SwitchPriority::END_OF_TRACK:
					wantsToChange = !Remaining(suppliers);
					break;
			}
			shouldChange &= wantsToChange;
			if(nextIsSynced && currentBackground)
			{
				while(nextSuppliers.front()->ConsumedBuffers() < currentBackground->ConsumedBuffers() &&
						Available(nextSuppliers))
					nextSuppliers.front()->NextDataChunk();
				shouldChange &= nextSuppliers.front()->ConsumedBuffers() == currentBackground->ConsumedBuffers();
			}

			if(shouldChange)
			{
				current = next;
				next = nullptr;
				nextCounter = 0;
				nextPriority = SwitchPriority::END_OF_TRACK;
				cachedNext = nullptr;
				nextIsLooping = false;

				suppliers.resize(1);
				currentBackground = nextSuppliers.empty() ? nullptr : nextSuppliers.front().get();
				if(nextSuppliers.empty())
					reinterpret_cast<Fade*>(suppliers[0].get())->AddSource({});
				else
					reinterpret_cast<Fade*>(suppliers[0].get())->AddSource(std::move(nextSuppliers.front()));
				for(size_t i = 1; i < nextSuppliers.size(); ++i)
					suppliers.emplace_back(std::move(nextSuppliers[i]));
				nextSuppliers.clear();
			}
		}

		// Advance the foreground layers to the next file.
		if(current && currentBackground && currentBackground->MaxChunks() && !wantsToChange)
		{
			for(size_t i = 1; i < suppliers.size(); ++i)
				if(!suppliers[i]->MaxChunks())
					suppliers[i] = current->Layers()[i].get().CreateSupplier(false);
		}

		// Get the next chunk from each supplier, and blend them together.
		// As the audio samples represent the amplitudes of the sound waves, they can be combined simply by
		// adding them together. However, this can overflow the sample type when the waves amplify each other.
		// A good soft limiter is tanh(): it transforms all values into the [-1, 1] range while being almost linear
		// for small values (tanh(1) = 0.8). This also avoids lower harmonic distortion for a reasonable input volume
		// that is caused by all nonlinear limiters.
		if(Available(suppliers))
		{
			vector<float> mergedInputs(OUTPUT_CHUNK);
			for (const auto &supplier : suppliers)
			{
				vector<sample_t> samples = supplier->NextDataChunk();
				// Convert the samples to 32-bit float so we can add them together without overflow.
				for(size_t i = 0; i < OUTPUT_CHUNK; ++i)
					mergedInputs[i] += static_cast<float>(samples[i]) / numeric_limits<sample_t>::max();
			}
			// Blend the samples
			vector<sample_t> samples;
			samples.reserve(OUTPUT_CHUNK);
			for (float sample : mergedInputs)
			{
				samples.emplace_back(round(tanh(sample) * numeric_limits<sample_t>::max()));
			}
			AddBufferData(samples);
		}
		else if(!done)
		{
			// If we can't read data, wait for it to become available.
			// This would normally be managed in some I/O operation for async suppliers,
			// but we aren't reading from any file here.
			this_thread::sleep_for(50ms);
		}
	}
}
