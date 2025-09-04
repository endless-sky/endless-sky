/* TrackSuppler.h
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

#include "../supplier/AsyncAudioSupplier.h"
#include "Track.h"

#include <mutex>



/// Handles track switching logic, and supplies audio from the current track.
class TrackSupplier : public AsyncAudioSupplier
{
public:
	enum class SwitchPriority {
		IMMEDIATE, PREFERRED, END_OF_TRACK
	};


public:
	TrackSupplier();

	const Track *GetCurrentTrack() const;
	const Track *GetNextTrack() const;
	const SwitchPriority GetNextTrackPriority() const;
	/// Configures what track to play after the current one.
	/// If forced, the supplier switches to the new track as soon as
	/// the cached buffers of the old track run out. Otherwise, it waits for
	/// the current track to finish.
	void SetNextTrack(const Track *track, SwitchPriority priority = SwitchPriority::END_OF_TRACK, bool loop = false,
		bool sync = false);

	// Inherited pure virtual methods
	void Decode() override;


private:
	friend class Audio;

	const Track *current = nullptr;
	const Track *next = nullptr;
	SwitchPriority nextPriority = SwitchPriority::END_OF_TRACK;
	bool nextIsLooping = false;
	bool nextIsSynced = false;
	mutable std::mutex lock;
};