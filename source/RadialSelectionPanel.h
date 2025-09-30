/* RadialSelectionPanel.h
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

#ifndef RADIAL_SELECTION_PANEL_H_
#define RADIAL_SELECTION_PANEL_H_

#include "Panel.h"
#include "pi.h"

#include <SDL2/SDL.h>

class RadialSelectionPanel: public Panel
{
public:
	RadialSelectionPanel() { UpdateLabelPosition(); }

	// we need to know how we were triggered, so we know what release event
	// destroys this panel
	void ReleaseWithMouseUp(const Point& position, MouseButton button);
	void ReleaseWithFingerUp(const Point& position, int fid);
	void ReleaseWithButtonUp(SDL_GameControllerButton button);
	void ReleaseWithAxisZero(SDL_GameControllerAxis axis);

	void AddOption(const std::string& icon, const std::string& description, std::function<void()> callback);
	void AddOption(Command command);

	// Adjustable properties
	// Where the panel is centered
	void SetPosition(const Point& position) { m_position = position; }
	// Set the starting angle in radians for the options. Defaults to 0.
	void SetStartAngle(float a);
	// Set the ending angle in radians the options. Defaults to 2*pi radians.
	void SetStopAngle(float a);
	// Set the radius from the center to where the options are drawn
	void SetRadius(float r);

protected:
	virtual bool Hover(int x, int y) override;
	virtual bool Drag(double dx, double dy) override;
	virtual bool Release(int x, int y, MouseButton button) override;
	virtual bool FingerMove(int x, int y, int fid) override;
	virtual bool FingerUp(int x, int y, int fid) override;
	virtual bool ControllerButtonUp(SDL_GameControllerButton button) override;
	virtual bool ControllerAxis(SDL_GameControllerAxis axis, int position) override;
	virtual bool ControllerTriggerReleased(SDL_GameControllerAxis axis, bool positive) override;

	// Set cursor position, in screen coordinates
	void MoveCursor(const Point& p);
	// Activate the currently selected option
	void ActivateOption();
	// Update the label position
	void UpdateLabelPosition();

	virtual void Draw() override;

private:
	struct Option
	{
		std::string icon;
		std::string description;
		std::function<void()> callback;

		Point position;
	};

	std::vector<Option> m_options;

	Point m_position;
	float m_radius = 150;
	float m_startAngle = 0;
	float m_stopAngle = 2 * PI;
	float m_angleDelta = 0;
	int m_selected_idx = -1;
	float m_zoom = 0;

	Point m_labelPos;

	// this control is meant to be triggered using a button press. We need to
	// track which button it is, so that we can remove ourselves on the button
	// release.
	SDL_GameControllerAxis m_triggered_axis = SDL_CONTROLLER_AXIS_INVALID;
	SDL_GameControllerButton m_triggered_button = SDL_CONTROLLER_BUTTON_INVALID;
	SDL_Keycode m_triggered_key = SDLK_UNKNOWN;
	int m_triggered_finger_id = -1;
	int m_triggered_mouse_button = -1;
	Point m_mouse_pos;
	Point m_cursor_pos;
};


#endif