/* FrameTimer.h
Michael Zahniser, 28 Oct 2013

Class to be used for enforcing a certain frame rate.
*/

#ifndef FRAME_TIMER_H_INCLUDED
#define FRAME_TIMER_H_INCLUDED

#include <ctime>



class FrameTimer {
public:
	// Create a timer that is just responsible for measuring the time that
	// elapses until Time() is called.
	FrameTimer();
	// Create a frame timer that will space frames out at exactly the given FPS,
	// _unless_ a frame takes too long by at least the given lag, in which case
	// the next frame happens immediately but no "catch-up" is done.
	FrameTimer(int fps, int maxLagMsec = 5);
	
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
	timespec next;
	long step;
	long maxLag;
};



#endif
