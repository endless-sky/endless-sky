/* GamepadCursor.cpp
Copyright (c) 2023 by Rian Shelley

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "GamepadCursor.h"

#include "Angle.h"
#include "Color.h"
#include "GameData.h"
#include "PointerShader.h"

namespace
{
	Angle g_cursor_angle(0.0);
	const Angle ANGLE_SPEED(.2);

	Point g_drawn_position(0, 0);
	Point g_move_direction(0, 0);
	size_t g_move_steps = 0;
}



Point GamepadCursor::s_position{};
bool GamepadCursor::s_enabled = false;



void GamepadCursor::SetPosition(const Point& pos, bool enable)
{
	if(!s_enabled)
	{
		g_drawn_position = pos;
		g_move_steps = 0;
		s_enabled = enable;
	}
	else
	{
		g_move_direction = (pos - g_drawn_position);
		if(g_move_direction.LengthSquared() > 300*300)
			g_move_steps = 15;
		else
			g_move_steps = 7;
		g_move_direction /= g_move_steps;
	}

	s_position = pos;
}



void GamepadCursor::SetEnabled(bool enabled)
{
	s_enabled = enabled;
	g_drawn_position = s_position;
	g_move_steps = 0;
}



// Draw the cursor at the given position if enabled
void GamepadCursor::Draw()
{
	if(s_enabled)
	{
		if(g_move_steps != 0)
		{
			// animate the cursor moving in the direction of the new position
			g_drawn_position += g_move_direction;
			--g_move_steps;
			if(g_move_steps == 0)
				g_drawn_position = s_position;
		}

		// For now, just drawing a rotating set of four pointers.
		g_cursor_angle += Angle(.2);
		const Color* color = GameData::Colors().Get("medium");
		PointerShader::Bind();
		PointerShader::Add(g_drawn_position, g_cursor_angle.Unit(), 12, 20, -20, *color);
		PointerShader::Add(g_drawn_position, (g_cursor_angle + Angle(90.)).Unit(), 12, 20, -20, *color);
		PointerShader::Add(g_drawn_position, (g_cursor_angle + Angle(180.)).Unit(), 12, 20, -20, *color);
		PointerShader::Add(g_drawn_position, (g_cursor_angle + Angle(270.)).Unit(), 12, 20, -20, *color);
		PointerShader::Unbind();
	}
}