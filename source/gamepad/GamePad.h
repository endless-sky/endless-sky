/* GamePad.h
Copyright (c) 2022 by Kari Pahula
Copyright (c) 2023 by thewierdnut

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef GAMEPAD_H_
#define GAMEPAD_H_

#include "../Point.h"

#include <cstdint>
#include <string>
#include <vector>

// GamePad state. It gets updated via SDL events and all users of it access this
// class to get it. No direct queries to SDL for getting it.
class GamePad {
public:
	// These are duplicates of some constants in SDL_gamecontroller.h, but
	// without a dependency on sdl to mess up unit test compilation.
	static constexpr uint8_t BUTTON_MAX = 21;
	static constexpr uint8_t BUTTON_INVALID = -1;
	static constexpr uint8_t AXIS_MAX = 6;
	static constexpr uint8_t AXIS_INVALID = -1;
   typedef bool Buttons[BUTTON_MAX];
   typedef int16_t Axes[AXIS_MAX];

	static void Init();
	static void SaveMapping();
	static void SaveConfig();
	static void Handle(const union SDL_Event &event);

	static int DeadZone();
	static void SetDeadZone(int dz);
	static int AxisIsButtonPressThreshold();
	static void SetAxisIsButtonPressThreshold(int t);

   static const Buttons &Held();
   static const Axes &Positions();

	// Axis state
	static Point LeftStick();
	static Point RightStick();
	static bool Trigger(uint8_t axis, bool positive);

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
	// Clear all of the Joystick -> Gamepad mappings for the current gamepad
	static void ClearMappings();
	// Reset Joystick -> Gamepad mappings to the default
	static void ResetMappings();
	// Capture and save the next Joystick input. This is to facilitate
	// remapping.
	static void CaptureNextJoystickInput();
	// Return the next joystick input, if it has been entered, or return an
	// empty string if the user hasn't pushed a button yet.
	static const std::string& GetNextJoystickInput();
	// Set a joystick to controller button mapping.
	static void SetControllerButtonMapping(const std::string& controllerButton, const std::string& joystickButton);

	// Calibrate the joystick axes
	static void BeginAxisCalibration();
	// Done calibrating joystick axes
	static void EndAxisCalibration();

	static const char *AxisDescription(uint8_t axis);
	static const char *ButtonDescription(uint8_t button);

	// typedef const char *DebugStrings[10];
	// static void DebugEvents(DebugStrings v);
};



#endif
