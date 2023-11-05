/* TouchScreen.cpp
Copyright (c) 2023 by Rian Shelley

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "TouchScreen.h"

#include "Screen.h"
#include "Gesture.h"

#include <map>
#include <mutex>

namespace
{
	std::map<SDL_FingerID, Point> g_fingers;
	std::vector<Point> g_finger_points;
	std::mutex g_finger_points_mutex;
	Gesture g_gesture;


	Point ToScreenCoordinates(const SDL_TouchFingerEvent& f)
	{
		return Point(
			(f.x - .5) * Screen::Width(),
			(f.y - .5) * Screen::Height()
		);
	}
}



void TouchScreen::Handle(const SDL_Event &event)
{
	Point pos;
	switch(event.type)
	{
	case SDL_FINGERDOWN:
		pos = ToScreenCoordinates(event.tfinger);
		g_fingers[event.tfinger.fingerId] = pos;
		g_gesture.Start(pos.X(), pos.Y(), event.tfinger.fingerId);
		break;
	case SDL_FINGERMOTION:
		pos = ToScreenCoordinates(event.tfinger);
		g_fingers[event.tfinger.fingerId] = pos;
		g_gesture.Add(pos.X(), pos.Y(), event.tfinger.fingerId);
		break;
	case SDL_FINGERUP:
		pos = ToScreenCoordinates(event.tfinger);
		g_fingers.erase(event.tfinger.fingerId);
		g_gesture.Add(pos.X(), pos.Y(), event.tfinger.fingerId);
		g_gesture.End(); // will trigger gesture event if needed
		break;
	default:
		return;
	}
	static std::vector<Point> s_points;
	s_points.clear();
	for(auto& kv: g_fingers)
		s_points.push_back(kv.second);

	std::lock_guard<std::mutex> lock(g_finger_points_mutex);
	g_finger_points.swap(s_points);
}



void TouchScreen::CancelGesture()
{
	g_gesture.Cancel();
}



// Return a set of all points currently being touched on the screen.
// No attempt is made to track which finger is which. If you need that info,
// use the events.
std::vector<Point> TouchScreen::Points()
{
	// Could use SDL_GetTouchFinger(), but a first cut didn't work right, and
	// its hard to troubleshoot on a desktop with no touchscreen.
	std::lock_guard<std::mutex> lock(g_finger_points_mutex);
	return g_finger_points;

	//std::vector<Point> ret;
	//for(int d = 0; d < SDL_GetNumTouchDevices(); ++d)
	//{
	//	for(int i = 0; i < SDL_GetNumTouchFingers(d); ++i)
	//	{
	//		const SDL_Finger* f = SDL_GetTouchFinger(d, i);
	//		if(f)
	//		{
	//			// finger events are floats from 0 to 1. Normalize to game screen
	//			// coordinates
	//			// finger coordinates are 0 to 1, normalize to screen coordinates
	//			int x = (f->x - .5) * Screen::Width();
	//			int y = (f->y - .5) * Screen::Height();
	//			ret.emplace_back(x, y);
	//		}
	//	}
	//}
	//return ret;
}