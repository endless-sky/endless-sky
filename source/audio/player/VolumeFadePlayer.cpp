/* VolumeFadePlayer.cpp
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

#include "VolumeFadePlayer.h"



VolumeFadePlayer::VolumeFadePlayer(SoundCategory category, std::unique_ptr<AudioSupplier> audioSupplier)
	: AudioPlayer(category, std::move(audioSupplier))
{
}



void VolumeFadePlayer::Update()
{
	if(isFading && !IsFinished())
	{
		double newVolume = Volume() - VOLUME_DECREASE;
		if(newVolume >= 0.)
			SetVolume(newVolume);
		else
		{
			SetVolume(0.);
			Stop();
		}
	}

	AudioPlayer::Update();
}



void VolumeFadePlayer::FadeOut()
{
	isFading = true;
}




