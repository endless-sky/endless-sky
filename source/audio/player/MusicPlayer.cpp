/* MusicPlayer.cpp
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

#include "MusicPlayer.h"

using namespace std;



MusicPlayer::MusicPlayer(unique_ptr<AudioSupplier> audioSupplier)
	: AudioPlayer(SoundCategory::MUSIC, std::move(audioSupplier), false)
{
}



void MusicPlayer::Move(double x, double y, double z) const
{
}



void MusicPlayer::Pause() const
{
}



void MusicPlayer::ConfigureSource()
{
	if(!alSource)
		return;

	alSourcef(alSource, AL_PITCH, 1.);
	alSourcef(alSource, AL_REFERENCE_DISTANCE, 1.);
	alSourcef(alSource, AL_ROLLOFF_FACTOR, 1.);
	alSourcef(alSource, AL_MAX_DISTANCE, 100.);
	AudioPlayer::Move(0, 0, 0);
}




