/* GamePad.h
Copyright (c) 2022 by Kari Pahula

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef GAMEPAD_H_
#define GAMEPAD_H_

#include "Command.h"
#include "Panel.h"
#include "Point.h"

#include <chrono>
#include <map>
#include <set>

#include <SDL2/SDL.h>

// GamePad state. It gets updated via SDL events and all users of it access this
// class to get it. No direct queries to SDL for getting it.
class GamePad {
public:
	static constexpr int LONG_PRESS_MILLISECONDS = 250;
	static constexpr double SCROLL_THRESHOLD = 0.5;
	static constexpr double STICK_MOUSE_MULT = 2.5;
	static constexpr double VECTOR_TURN_THRESHOLD = 0.05;

	static GamePad &Singleton();

	void Handle(const SDL_Event &event);

	// Read held buttons and how long they have been held.
	std::map<Uint8, std::chrono::milliseconds> HeldButtons() const;
	// Read held buttons and when they were first pressed.
	std::map<Uint8, std::chrono::time_point<std::chrono::steady_clock>> HeldButtonsSince() const;
	// Read released buttons and how long they have been held.
	std::map<Uint8, std::chrono::milliseconds> ReleasedButtons() const;

	// Read single button held state.
	bool Held(Uint8) const;

	// Clear button state. It won't show up in held or released buttons until there's
	// another down or up event, respectively.
	void Clear();
	void Clear(const std::set<Uint8> &);
	void Clear(Uint8);

	// Simple list of pressed buttons.
	std::set<Uint8> ReadHeld(const std::set<Uint8> &);

	// Axis state
	Point LeftStick() const;
	Point RightStick() const;
	double LeftStickX() const;
	double LeftStickY() const;
	double RightStickX() const;
	double RightStickY() const;
	double LeftTrigger() const;
	double RightTrigger() const;

	bool RepeatAxis(Uint8 axis);
	bool RepeatAxisNeg(Uint8 axis);
	bool RepeatButton(Uint8 button);

	bool HavePads() const;

	// Read controller state into a Command, for Engine.  May clear state held
	// by GamePad to avoid repeating them.
	Command ToCommand();

	// For getting interactions like Command::HAIL which are handled by UI
	// instead of Engine. May clear state.
	Command ToPanelCommand();

	// The angle of right stick is used for fleet commands.
	Command RightStickCommand() const;


private:
	bool Repeat(Uint8 which, double threshold, Uint8 opposite = 255);


private:
	std::map<Uint8, std::chrono::time_point<std::chrono::steady_clock>> held;
	std::map<Uint8, std::chrono::time_point<std::chrono::steady_clock>> released;
	std::set<Uint8> unreportedReleases;

	double axis[SDL_CONTROLLER_AXIS_MAX] = {0};
	int activePads = 0;

	std::map<Uint8, std::chrono::time_point<std::chrono::steady_clock>> repeatTimer;
	Uint8 repeatWhich;
};



#endif
