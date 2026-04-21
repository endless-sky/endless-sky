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



namespace {
	// In units of frames at 60fps, so half a second.
	uint32_t BLINK_INTERVAL = 30;
}



void Caret::Draw()
{
	++blinkCounter;

	if(blinkCounter < BLINK_INTERVAL)
	{
		Point p2 = pos;
		p2.Y() += height;
		p2.X() += 1;
		FillShader::Fill(Rectangle::WithCorners(pos, p2), color);
	}
	else if(blinkCounter >= BLINK_INTERVAL * 2)
	{
		blinkCounter = 0;
	}
}



void Caret::BlinkOn()
{
	blinkCounter = 0;
}
