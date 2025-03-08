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


WavSupplier::WavSupplier(const Sound &sound, bool is3x, bool looping)
		: AudioSupplier(is3x), sound(sound), looping(looping), wasBufferGiven(false)
{
}



ALsizei WavSupplier::MaxChunkCount() const
{
	if(looping)
		return 2;
	return !wasBufferGiven;
}



int WavSupplier::AvailableChunks() const
{
	return MaxChunkCount();
}



bool WavSupplier::IsSynchronous() const
{
	return true;
}



bool WavSupplier::NextChunk(ALuint buffer)
{
	// This function must receive the buffer from AwaitNextChunk(),
	// which returns the audio buffer backing this sound. No action needed.
	// Note: Unlike the sync use via AwaitNextChunk(),
	// this will not change 3x mode.
	return true;
}



ALuint WavSupplier::AwaitNextChunk()
{
	wasBufferGiven = true;
	if(is3x && sound.Buffer3x())
		return sound.Buffer3x();
	return sound.Buffer();
}



void WavSupplier::ReturnBuffer(ALuint buffer)
{
	// This is the audio buffer backing this sound. No action needed.
}
