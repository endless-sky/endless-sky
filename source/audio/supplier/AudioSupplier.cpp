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

#include <cstring>


AudioSupplier::AudioSupplier(bool is3x)
	: is3x(is3x)
{
}



void AudioSupplier::Set3x(bool is3x)
{
	this->is3x = is3x;
}


void AudioSupplier::SetSilence(ALuint buffer, size_t frames)
{
	std::vector<int16_t> data(frames);
	alBufferData(buffer, AL_FORMAT_STEREO16, data.data(), 2 * frames, 44100);
}
