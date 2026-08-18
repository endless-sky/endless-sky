/* TimerResolutionGuard.cpp
Copyright (c) 2025 by TomGoodIdea

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "TimerResolutionGuard.h"

#include <windows.h>

#include <timeapi.h>



TimerResolutionGuard::TimerResolutionGuard()
{
	// Make sure that the sleep timer has at least 1 ms resolution
	// to avoid irregular frame rates.
	timeBeginPeriod(1);
}



TimerResolutionGuard::~TimerResolutionGuard()
{
	// Reset the timer resolution so that it doesn't affect performance of the whole OS.
	timeEndPeriod(1);
}
