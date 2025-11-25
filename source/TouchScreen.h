/* TouchScreen.h
Copyright (c) 2023 by Rian Shelley

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/
#ifndef TOUCHSCREEN_H_INCLUDED
#define TOUCHSCREEN_H_INCLUDED

#include <SDL2/SDL_events.h>

#include <vector>

#include "Point.h"


// Tracks touchscreen events for objects that need to poll for touch positions
class TouchScreen
{
public:
   static void Handle(const SDL_Event& event);
   static void CancelGesture();

   static std::vector<Point> Points();
};



#endif


