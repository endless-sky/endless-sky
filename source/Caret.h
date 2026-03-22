/* Caret.h
Copyright (c) 2026 by thewierdnut

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef CARET_H
#define CARET_H

#include "Color.h"
#include "Rectangle.h"

#include <cstdint>

// Draws a caret at the given position
class Caret final
{
public:
   void Draw();
   void BlinkOn();

   void SetColor(const Color &c) { color = c; }

   void SetHeight(int h) { height = h; }
   void SetX(int x) { pos.X() = x; }
   void SetY(int y) { pos.Y() = y; }

private:
   uint32_t interval = 500;
   uint32_t next_blink = 0;
   Color color{1.0, 1.0, 1.0, .75};
   bool blink_on = true;

   int height = 14;
   Point pos{0, 0};
};


#endif // CARET_H
