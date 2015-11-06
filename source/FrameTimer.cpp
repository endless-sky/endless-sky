/* FrameTimer.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "FrameTimer.h"

#include <thread>

using namespace std;



// Create a timer that is just responsible for measuring the time that
// elapses until Time() is called.
FrameTimer::FrameTimer()
{
	next = chrono::high_resolution_clock::now();
}



// Create a frame timer that will space frames out at exactly the given FPS,
// _unless_ a frame takes too long by at least the given lag, in which case
// the next frame happens immediately but no "catch-up" is done.
FrameTimer::FrameTimer(int fps, int maxLagMsec)
	: step(chrono::nanoseconds(1000000000 / fps)),
	maxLag(chrono::milliseconds(maxLagMsec))
{
	next = chrono::high_resolution_clock::now();
	Step();
}



// Wait until the next frame should begin.
void FrameTimer::Wait()
{
	chrono::high_resolution_clock::time_point now = chrono::high_resolution_clock::now();
	if(now < next)
	{
		// If the clock somehow got confused, don't allow it to sleep for longer
		// than the step interval.
		this_thread::sleep_for(min(next - now, step));
		now = chrono::high_resolution_clock::now();
	}
	
	// If the lag is too high, don't try to do catch-up.
	if(now - next > maxLag)
		next = now;
	
	Step();
}



// Find out how long it has been since this timer was created, in seconds.
double FrameTimer::Time() const
{
	chrono::high_resolution_clock::time_point now = chrono::high_resolution_clock::now();
	return chrono::duration_cast<chrono::nanoseconds>(now - next).count() * .000000001;
}



// Change the frame rate (for viewing in slow motion).
void FrameTimer::SetFrameRate(int fps)
{
	step = chrono::nanoseconds(1000000000 / fps);
}



// Calculate when the next frame should begin.
void FrameTimer::Step()
{
	next += step;
}
