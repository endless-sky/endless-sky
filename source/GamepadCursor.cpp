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
#include "Animate.h"
#include "Color.h"
#include "DelaunayTriangulation.h"
#include "GameData.h"
#include "shader/PointerShader.h"

namespace
{
	Angle g_cursor_angle(0.0);
	const Angle ANGLE_SPEED(.2);
}



Animate<Point> GamepadCursor::s_position;
bool GamepadCursor::s_enabled = false;



void GamepadCursor::SetPosition(const Point& pos, bool enable)
{
	if(!s_enabled)
	{
		s_position.Set(pos, 0);
		s_position.EndAnimation();
		s_enabled = enable;
	}
	else
	{
		if(s_position.Value().DistanceSquared(pos) > 300*300)
			s_position.Set(pos, 15);
		else
			s_position.Set(pos, 7);
	}

}



void GamepadCursor::SetEnabled(bool enabled)
{
	s_enabled = enabled;
	s_position.EndAnimation();
}



// Draw the cursor at the given position if enabled
void GamepadCursor::Draw()
{
	if(s_enabled)
	{
		// animate the cursor moving in the direction of the new position
		s_position.Step();

		// For now, just drawing a rotating set of four pointers.
		g_cursor_angle += Angle(.2);
		const Color* color = GameData::Colors().Get("medium");
		PointerShader::Bind();
		PointerShader::Add(s_position.AnimatedValue(), g_cursor_angle.Unit(), 12, 20, -20, *color);
		PointerShader::Add(s_position.AnimatedValue(), (g_cursor_angle + Angle(90.)).Unit(), 12, 20, -20, *color);
		PointerShader::Add(s_position.AnimatedValue(), (g_cursor_angle + Angle(180.)).Unit(), 12, 20, -20, *color);
		PointerShader::Add(s_position.AnimatedValue(), (g_cursor_angle + Angle(270.)).Unit(), 12, 20, -20, *color);
		PointerShader::Unbind();
	}
}



int GamepadCursor::MoveDir(const Point& dir, const std::vector<Point>& options)
{
	// Use a Delaunay Triangulation to create a reasonable graph spanning all
	// of the points, and then find the edge that closely matches the given
	// direction. This is probably overkill, but it gives reasonable results.

	// Also... snap the cursor to the nearest option, under the assumption that
	// the options are created and destroyed or moved occasionally
	DelaunayTriangulation dt;
	Point best_cursor(-100000.0, -100000.0);
	double best_distance = 1000000000000.0;
	for(const Point& p: options)
	{
		dt.AddPoint(p);

		double distance = Position().DistanceSquared(p);
		if(distance < best_distance)
		{
			best_distance = distance;
			best_cursor = p;
		}
	}

	// TODO: scrolls are usually around 50 pixels, more than this, so
	//       we keep hitting the else case.
	if(best_distance < 24*24) // most buttons are 20x100, with 5px spacing
		SetPosition(best_cursor);
	else // too far away. probably a missing button, not a scroll event
		dt.AddPoint(Position());

	// We want to be within 45 degrees of any vector, which means
	// the match has to be better than sqrt(2)/2. (a value of 1 means
	// a perfect match)
	float best_result = .70710678;
	size_t best_idx = options.size();
	const Point direction = dir.Unit();
	for(const auto &edge: dt.Edges())
	{
		size_t other = options.size();
		// testing for floating point equality is ok here, since we
		// didn't do any math on these points, just assignments.
		// If we added the current position due to it not matching any option,
		// then its index will be one higher than the maximum. Check for it
		// first.
		if(edge.first == options.size())
			other = edge.second;
		else if(edge.second == options.size())
			other = edge.first;
		else if(options[edge.first].X() == GamepadCursor::Position().X() &&
			options[edge.first].Y() == GamepadCursor::Position().Y())
			other = edge.second;
		else if(options[edge.second].X() == GamepadCursor::Position().X() &&
			options[edge.second].Y() == GamepadCursor::Position().Y())
			other = edge.first;

		if(other < options.size())
		{
			// this edge leads away from the currently selected zone.
			// compare it to the joystick angle, and pick the closest
			// one.
			Point edge_direction = (options[other] - GamepadCursor::Position()).Unit();
			// dot product gets closer to 1 the more it matches
			double dot = direction.Dot(edge_direction);
			if(dot > best_result)
			{
				best_result = dot;
				best_idx = other;
			}
		}
	}

	if(best_idx < options.size())
	{
		SetPosition(options[best_idx]);
		return best_idx;
	}
	return -1;
}