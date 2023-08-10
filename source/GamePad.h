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
#include "Point.h"

#include <SDL_gamecontroller.h>
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

   typedef bool Buttons[SDL_CONTROLLER_BUTTON_MAX];
   typedef int16_t Axes[SDL_CONTROLLER_AXIS_MAX];

	static void Init();
	static void SaveMapping();
	static void Handle(const SDL_Event &event);

	// Read held buttons and how long they have been held.
	// std::map<Uint8, std::chrono::milliseconds> HeldButtons() const;
	// Read held buttons and when they were first pressed.
	// std::map<Uint8, std::chrono::time_point<std::chrono::steady_clock>> HeldButtonsSince() const;
	// Read released buttons and how long they have been held.
	// std::map<Uint8, std::chrono::milliseconds> ReleasedButtons() const;

	// Read single button held state.
	// bool Held(Uint8) const;

	// Clear button state. It won't show up in held or released buttons until there's
	// another down or up event, respectively.
	// void Clear();
	// void Clear(const std::set<Uint8> &);
	// void Clear(Uint8);

	// Simple list of pressed buttons.
	//std::set<Uint8> ReadHeld(const std::set<Uint8> &);
   
   static const Buttons& Held();
   static const Axes& Positions();
   //static const std::map<int, std::string>& Controllers();
   //static std::pair<int, std::string> SelectedController();

	// Axis state
	static Point LeftStick();
	static Point RightStick();

	// bool RepeatAxis(Uint8 axis);
	// bool RepeatAxisNeg(Uint8 axis);
	// bool RepeatButton(Uint8 button);

	// bool HavePads() const;

	// Read controller state into a Command, for Engine.  May clear state held
	// by GamePad to avoid repeating them.
	// Command ToCommand();

	// For getting interactions like Command::HAIL which are handled by UI
	// instead of Engine. May clear state.
	// Command ToPanelCommand();

	// The angle of right stick is used for fleet commands.
	// Command RightStickCommand() const;

	// Retrieve a list of all the controller button->joystick button mappings
	static std::vector<std::pair<std::string, std::string>> GetCurrentSdlMappings();
	// Retrieve an ordered list of all the controllers attached to the system
	static std::vector<std::string> GetControllerList();
	// Returns an index into the controller list indicating which one is active.
	// Do not cache this value, as it will change if the set of controllers
	// change.
	static int CurrentControllerIdx();
	// Change which controller is active.
	static void SetControllerIdx(int idx);
	// Clear all of the Gamepad -> Joystick mappings for the current gamepad
	static void ClearMappings();
	// Capture and save the next Joystick input. This is to facilitate
	// remapping.
	static void CaptureNextJoystickInput();
	// Return the next joystick input, if it has been entered, or return an
	// empty string if the user hasn't pushed a button yet.
	static const std::string& GetNextJoystickInput();
	// Set a joystick to controller button mapping.
	static void SetControllerButtonMapping(const std::string& controllerButton, const std::string& joystickButton);



private:
	// bool Repeat(Uint8 which, double threshold, Uint8 opposite = 255);



};



#endif