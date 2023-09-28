/* Debug.h
Copyright (c) 2023 by Daniel Yoon

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef DEBUG_H_
#define DEBUG_H_

#include <imgui.h>

class SDL_Window;
union SDL_Event;



class Debug
{
public:
	static bool Init(SDL_Window *window, void *glContext);

	static bool Process(SDL_Event *event);
	static void StartCapture();
	static void Render();

	static void Shutdown();

	// Add more overloads as needed
	static bool ScalarSlider(const char *label, double *value, double min, double max);

	static void SetDebugMode(bool mode);
	static bool GetDebugMode();
};



#endif
