/* RadialSelectionPanel.cpp
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
#include "RadialSelectionPanel.h"
#include "Color.h"
#include "FillShader.h"
#include "GameData.h"
#include "GamePad.h"
#include "LineShader.h"
#include "OutlineShader.h"
#include "RingShader.h"
#include "Set.h"
#include "image/Sprite.h"
#include "image/SpriteSet.h"
#include "SpriteShader.h"
#include "UI.h"
#include "pi.h"
#include "text/Font.h"
#include "text/FontSet.h"
#include <SDL2/SDL.h>


namespace
{
	constexpr float DEAD_ZONE = .75f;

	// Defining some functions here to handle circular math.

	// restrict to positive range from 0 to 2 PI
	float AngleClamp(float a)
	{
		a = fmod(a, 2 * PI);
		if(a < 0)
			a += 2 * PI;
		return a;
	}

	// get measurement between two angles
	float AngleDelta(float a, float b)
	{
		return b > a ? b - a : b + 2 * PI - a;
	}

	// is an angle within two other angles?
	bool AngleWithin(float a, float left, float right)
	{
		if(right < left)
		{
			right += 2 * PI;
		}
		if(a < left)
		{
			a += 2 * PI;
		}
		return left <= a && a < right;
	}
}



void RadialSelectionPanel::ReleaseWithMouseUp(const Point& position, int button)
{
	// Default the position to the mouse position, and also cache the mouse
	// position for the selection logic
	m_position = position;
	m_mouse_pos = position;
	m_triggered_mouse_button = button;
	MoveCursor(position);
}



void RadialSelectionPanel::ReleaseWithFingerUp(const Point& position, int fid)
{
	m_position = position;
	m_triggered_finger_id = fid;
	MoveCursor(position);
}



void RadialSelectionPanel::ReleaseWithButtonUp(SDL_GameControllerButton button)
{
	// position defaults to the center of the screen (0, 0)
	m_triggered_button = button;
	MoveCursor({0, 0});
}



void RadialSelectionPanel::ReleaseWithAxisZero(SDL_GameControllerAxis axis)
{
	// position defaults to the center of the screen (0, 0)
	m_triggered_axis = axis;
	MoveCursor({0, 0});
}



void RadialSelectionPanel::AddOption(const std::string& icon, const std::string& description, std::function<void()> callback)
{
	m_options.emplace_back(Option {
		icon,
		description,
		callback
	});
	

	float range = AngleDelta(m_startAngle, m_stopAngle);

	m_angleDelta = range / (m_options.size() - 1);

	// if we do something stupid like use 355 degrees of range, then the
	// endpoints will only be 5 degrees apart. Detect this and increase the
	// number of splits if needed.
	if(2 * PI - range < m_angleDelta)
		m_angleDelta = range / m_options.size();

	float angle = m_startAngle;
	for(auto& option: m_options)
	{
		// unit vector points up
		option.position = Point(sin(angle), -cos(angle)) * m_radius;
		angle += m_angleDelta;
	}
}


void RadialSelectionPanel::AddOption(Command command)
{
	// Add the command. Set the next flag in the lambda, so that it applies the
	// command after this panel goes away (the engine will discard any keyboard
	// state on its first startup)
	AddOption(
		command.Icon(),
		command.Description(),
		[command]() { Command::InjectOnce(command, true); }
	);
}



void RadialSelectionPanel::SetStartAngle(float a)
{
	m_startAngle = AngleClamp(a);
	UpdateLabelPosition();
}



void RadialSelectionPanel::SetStopAngle(float a)
{
	m_stopAngle = AngleClamp(a);
	UpdateLabelPosition();
}



void RadialSelectionPanel::SetRadius(float r)
{
	m_radius = r;
	UpdateLabelPosition();
}



bool RadialSelectionPanel::Hover(int x, int y)
{
	if(m_triggered_mouse_button != -1)
	{
		MoveCursor(Point(x, y));
		return true;
	}
	return false;
}



bool RadialSelectionPanel::Drag(double dx, double dy)
{
	if(m_triggered_mouse_button != -1)
	{
		m_mouse_pos += Point(dx, dy);
		MoveCursor(m_mouse_pos);
		return true;
	}
	return false;
}



bool RadialSelectionPanel::Release(int x, int y)
{
	if(m_triggered_mouse_button != -1)
	{
		m_mouse_pos = Point(x, y);
		MoveCursor(m_mouse_pos);
		ActivateOption();
		return true;
	}
	return false;
}



bool RadialSelectionPanel::FingerMove(int x, int y, int fid)
{
	if(m_triggered_finger_id == fid)
	{
		MoveCursor(Point(x, y));
		return true;
	}
	return false;
}



bool RadialSelectionPanel::FingerUp(int x, int y, int fid)
{
	if(m_triggered_finger_id == fid)
	{
		MoveCursor(Point(x, y));
		ActivateOption();
		return true;
	}
	return false;
}



bool RadialSelectionPanel::ControllerButtonUp(SDL_GameControllerButton button)
{
	if(m_triggered_button == button)
	{
		ActivateOption();
		return true;
	}
	return false;
}



bool RadialSelectionPanel::ControllerAxis(SDL_GameControllerAxis axis, int position)
{
	if(m_triggered_axis == axis)
	{
		if(SDL_abs(position) < GamePad::DeadZone())
		{
			ActivateOption();
			return true;
		}
	}
	else if (axis == SDL_CONTROLLER_AXIS_LEFTX ||
	         axis == SDL_CONTROLLER_AXIS_LEFTY)
	{
		MoveCursor(m_position + GamePad::LeftStick() * m_radius / 32767);
		return true;
	}
	else if (axis == SDL_CONTROLLER_AXIS_RIGHTX ||
	         axis == SDL_CONTROLLER_AXIS_RIGHTY)
	{
		MoveCursor(m_position + GamePad::RightStick() * m_radius / 32767);
		return true;
	}
	return false;
}



bool RadialSelectionPanel::ControllerTriggerReleased(SDL_GameControllerAxis axis, bool positive)
{
	if(m_triggered_axis == axis)
	{
		ActivateOption();
		return true;
	}
	return false;
}



void RadialSelectionPanel::MoveCursor(const Point& p)
{
	m_cursor_pos = p;
	Point relativePosition = p - m_position;
	if(relativePosition.LengthSquared() < m_radius * m_radius * (DEAD_ZONE * DEAD_ZONE))
	{
		// not close enough to the edge. We don't know what they are pointing at
		m_selected_idx = -1;
	}
	else
	{
		// what angle are we pointing at?
		relativePosition = relativePosition.Unit();
		// 0 degrees is up, so args are rotated counter-clockwise 90 degrees
		float pointAngle = AngleClamp(SDL_atan2f(relativePosition.X(), -relativePosition.Y()));
		float testAngle = AngleClamp(m_startAngle - m_angleDelta / 2);
		for(m_selected_idx = 0; m_selected_idx < static_cast<int>(m_options.size()); ++m_selected_idx)
		{
			float nextAngle = AngleClamp(testAngle + m_angleDelta);
			if(AngleWithin(pointAngle, testAngle, nextAngle))
			{
				break;
			}
			testAngle = nextAngle;
		}
	}
}



void RadialSelectionPanel::ActivateOption()
{
	GetUI()->Pop(this); 	// quit the dialog
	if(m_selected_idx >= 0 && m_selected_idx < static_cast<int>(m_options.size()))
	{
		m_options[m_selected_idx].callback();
	}
}



void RadialSelectionPanel::UpdateLabelPosition()
{
	float midAngle = AngleClamp(
		m_startAngle < m_stopAngle
			? (m_startAngle + m_stopAngle) / 2
			: (m_startAngle + m_stopAngle + 2 * PI) / 2
	);

	// unit vector points up
	m_labelPos = Point(sin(midAngle), -cos(midAngle)) * (m_radius / 2);
}



void RadialSelectionPanel::Draw()
{
	DrawBackdrop();
	if(m_zoom < 1)
		m_zoom = std::min(1.0, m_zoom + 8.0/60);
	else
		m_zoom = 1.0;
	
	const Color* color = GameData::Colors().Get("medium");
	const Color* colorBright = GameData::Colors().Get("bright");

	if(m_selected_idx >= 0 && m_selected_idx < static_cast<int>(m_options.size()))
		LineShader::Draw(m_position, m_position + m_options[m_selected_idx].position * m_zoom, 3, *color);
	else
		LineShader::Draw(m_position, m_cursor_pos, 1, *color);

	const Font &font = FontSet::Get(18);
	std::set<std::string> used_chars;

	for(int i = 0; i < static_cast<int>(m_options.size()); ++i)
	{
		Point draw_position = m_position + m_options[i].position * m_zoom;

		const Sprite *sprite = nullptr;
		if(!m_options[i].icon.empty() && (sprite = SpriteSet::Get(m_options[i].icon)))
		{
			if(i == m_selected_idx)
				SpriteShader::Draw(sprite, draw_position);
			else
				OutlineShader::Draw(sprite, draw_position, {sprite->Width(), sprite->Height()}, *color);
		}
		else
		{
			// no icon. just draw a circle with the first letter of the command in it.
			if(i == m_selected_idx)
			{
				RingShader::Draw(draw_position, 32, 28, *colorBright);
			}
			else
			{
				RingShader::Draw(draw_position, 32, 30, *color);
			}
			
			std::string icon_label = "?";

			// trim "Fleet: " off the front if it is present
			std::string description = m_options[i].description;
			if(description.substr(0, 7) == "Fleet: ")
				description = description.substr(7);

			for(char c: description)
			{
				auto result = used_chars.insert(std::string(1, c));
				if(result.second)
				{
					icon_label = *result.first;
					break;
				}
			}

			float posx = draw_position.X() - font.Width(icon_label) / 2.0;
			float posy = draw_position.Y() - font.Height() / 2.0;
			font.DrawAliased(icon_label, posx, posy, i == m_selected_idx ? *colorBright : *color);
		}
	}


	if(m_selected_idx >= 0 && m_selected_idx < static_cast<int>(m_options.size()))
	{
		float width = font.Width(m_options[m_selected_idx].description);
		float height = font.Height();
		float posx = m_position.X() + (m_labelPos.X() - width / 2.0);
		float posy = m_position.Y() + (m_labelPos.Y() - height/ 2.0);
		FillShader::Fill(m_labelPos, {width, height}, Color(0, .5));
		font.DrawAliased(m_options[m_selected_idx].description, posx, posy, *colorBright);
	}
	
}