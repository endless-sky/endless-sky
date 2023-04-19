/* Gesture.h
Copyright (c) 2023 by Rian Shelley

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef GESTURE_H_
#define GESTURE_H_

#include <vector>
#include <cstdint>
#include <array>
#include <string>

/**
 * Handle gesture recognition and events. Intentionally ignoring the
 * SDL_gesture api, as it is unmaintained and removed in SDL3
 */
class Gesture
{
public:
   enum GestureEnum {NONE, X, CIRCLE, CARET_UP, CARET_LEFT, CARET_RIGHT, CARET_DOWN};

   void Start(float x, float y, int finger_id);
   GestureEnum Add(float x, float y, int finger_id);
   GestureEnum End();

   struct Point { float x; float y; };

   static uint32_t EventID();
   static const std::string& Description(GestureEnum gesture);


private:
   std::vector<Point> m_path;

   float m_ymin;
   float m_ymax;
   float m_xmin;
   float m_xmax;

   uint64_t m_tick_start = 0;

   bool m_valid = false;
};

#endif