/* Caret.cpp
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

#include "Caret.h"

#include "shader/FillShader.h"

#include <SDL2/SDL.h>

void Caret::Draw()
{
   auto now = SDL_GetTicks();
   if(now > next_blink)
   {
      blink_on = !blink_on;
      next_blink = next_blink ? next_blink + interval : now + interval;
   }

   if(blink_on)
   {
      Point p2 = pos;
      p2.Y() += height;
      p2.X() += 1;
      FillShader::Fill(Rectangle::WithCorners(pos, p2), color);
   }
}

void Caret::BlinkOn()
{
   blink_on = true;
   next_blink = SDL_GetTicks() + interval;
}
