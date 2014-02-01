/* FrameTimer.cpp
Michael Zahnsier, 28 Oct 2013

Function definitions for the FrameTimer class.
*/

#include "FrameTimer.h"

using namespace std;



// Create a timer that is just responsible for measuring the time that
// elapses until Time() is called.
FrameTimer::FrameTimer()
{
	clock_gettime(CLOCK_MONOTONIC, &next);
}



// Create a frame timer that will space frames out at exactly the given FPS,
// _unless_ a frame takes too long by at least the given lag, in which case
// the next frame happens immediately but no "catch-up" is done.
FrameTimer::FrameTimer(int fps, int maxLagMsec)
	: step(1000000000 / fps), maxLag(maxLagMsec * 1000000)
{
	// TODO: Once I am using a compiler with full c++11 library support, I can
	// use chrono instead of linux-specific functions.
	clock_gettime(CLOCK_MONOTONIC, &next);
	Step();
}



// Wait until the next frame should begin.
void FrameTimer::Wait()
{
	// Note: in theory this could get interrupted by a signal handler, although
	// it's unlikely the program will receive any signals that do not terminate
	// it and that it does not ignore. But, the worst that would happen in that
	// case is that this particular frame will end too quickly, and then it will
	// go back to normal for the next one.
	clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, nullptr);
	
	timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);
	
	// Note: 2 seconds in nsec is nearly long enough to overflow a 32-bit signed
	// int. So, do all this using a long long. The value of "lag" is how many
	// nsec later we ended this frame than we wanted to:
	long long lag = (now.tv_sec - next.tv_sec);
	lag *= 1000000000ll;
	lag += (now.tv_nsec - next.tv_nsec);
	
	// If the lag is too high, don't try to do catch-up.
	if(lag > maxLag)
		next = now;
	
	Step();
}



// Find out how long it has been since this timer was created, in seconds.
double FrameTimer::Time() const
{
	timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);
	
	return (now.tv_sec - next.tv_sec) + (now.tv_nsec - next.tv_nsec) / 1000000000.;
}



// Change the frame rate (for viewing in slow motion).
void FrameTimer::SetFrameRate(int fps)
{
	step = (1000000000 / fps);
}



// Calculate when the next frame should begin.
void FrameTimer::Step()
{
	next.tv_nsec += step;
	if(next.tv_nsec > 1000000000)
	{
		++next.tv_sec;
		next.tv_nsec -= 1000000000;
	}
}
