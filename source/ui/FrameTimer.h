/* FrameTimer.h
Copyright (c) 2014 by Michael Zahniser

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

#include <chrono>



// Class to be used for enforcing a certain frame rate. This ensures that the
// frames are not drawn faster than a certain speed, but if the calculations or
// the graphics cannot keep up it will allow things to go slower for a few frames
// without trying to "catch up" by making the subsequent frame faster.
class FrameTimer {
public:
	// Create a timer that is just responsible for measuring the time that
	// elapses until Time() is called.
	FrameTimer();
	// Create a frame timer that will space frames out at exactly the given FPS,
	// _unless_ a frame takes too long by at least the given lag, in which case
	// the next frame happens immediately but no "catch-up" is done.
	explicit FrameTimer(int fps, int maxLagMsec = 5);

	// Wait until the next frame should begin.
	void Wait();
	// Find out how long it has been since this timer was created, in seconds.
	double Time() const;

	// Change the frame rate (for viewing in slow motion).
	void SetFrameRate(int fps);


private:
	// Calculate when the next frame should begin.
	void Step();


private:
	std::chrono::steady_clock::time_point next;
	std::chrono::steady_clock::duration step;
	std::chrono::steady_clock::duration maxLag;
};
