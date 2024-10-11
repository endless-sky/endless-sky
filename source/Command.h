/* Command.h
Copyright (c) 2015 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include <SDL2/SDL_events.h>

#include <cstdint>
#include <string>
#include <atomic>

#include "Gesture.h"

class DataNode;

struct CommandEvent
{
	uint32_t type;
	uint32_t timestamp;
	uint32_t windowID;
	uint64_t state;
	uint8_t  pressed;
};
static_assert(sizeof(CommandEvent) <= sizeof(SDL_Event), "The CommandEvent should be smaller");

// Class mapping key presses to specific commands / actions. The player can
// change the mappings for most of these keys in the preferences panel.
// A single Command object can represent multiple individual commands, e.g.
// everything the AI wants a ship to do, or all keys the player is holding down.
class Command {
public:
	// Empty command:
	static const Command NONE;
	// Main menu:
	static const Command MENU;
	// Ship controls:
	static const Command FORWARD;
	static const Command LEFT;
	static const Command RIGHT;
	static const Command AUTOSTEER;
	static const Command BACK;
	static const Command MOUSE_TURNING_HOLD;
	static const Command PRIMARY;
	static const Command TURRET_TRACKING;
	static const Command SECONDARY;
	static const Command SELECT;
	static const Command LAND;
	static const Command BOARD;
	static const Command HAIL;
	static const Command SCAN;
	static const Command JUMP;
	static const Command FLEET_JUMP;
	static const Command TARGET;
	static const Command NEAREST;
	static const Command NEAREST_ASTEROID;
	static const Command DEPLOY;
	static const Command AFTERBURNER;
	static const Command CLOAK;
	// UI controls:
	static const Command MAP;
	static const Command INFO;
	static const Command MESSAGE_LOG;
	static const Command FULLSCREEN;
	static const Command FASTFORWARD;
	static const Command HELP;
	// Escort commands:
	static const Command FIGHT;
	static const Command GATHER;
	static const Command HOLD_FIRE;
	static const Command HOLD_POSITION;
	static const Command AMMO;
	static const Command HARVEST;
	// This command is given in combination with JUMP or LAND and tells a ship
	// not to jump or land yet even if it is in position to do so. It can be
	// given from the AI when a ship is waiting for its parent. It can also be
	// given from the player/input engine when the player is preparing his/her
	// fleet for jumping or to indicate that the player is switching landing
	// targets. (There is no explicit key mapped to this command.)
	static const Command WAIT;
	// This command from the AI tells a ship that if possible, it should apply
	// less than its full thrust in order to come to a complete stop.
	static const Command STOP;
	// Modifier command, usually triggered by shift-key. Changes behavior of
	// other commands like NEAREST, TARGET, HAIL and BOARD.
	static const Command SHIFT;

	// Mobile specific
	static const Command FLEET_FORMATION;

public:
	// In the given text, replace any instances of command names (in angle
	// brackets) with key names (in quotes).
	static std::string ReplaceNamesWithKeys(const std::string &text);

public:
	Command() = default;
	// Create a command representing whatever command is mapped to the given
	// keycode (if any).
	explicit Command(int keycode);
	// Create a command from an sdl command event
	explicit Command(const union SDL_Event &event);
	// Create a command representing whatever is mapped to the given gesture
	explicit Command(Gesture::GestureEnum gesture);
	// Create a command representing a game controller axis trigger
	static Command FromTrigger(uint8_t axis, bool positive);
	// Create a command representing a game controller button press
	static Command FromButton(uint8_t button);

	// Read the current keyboard state and set this object to reflect it.
	void ReadKeyboard();

	// Load or save the keyboard preferences.
	static void LoadSettings(const std::string &path);
	static void SaveSettings(const std::string &path);
	static void SetKey(Command command, int keycode);
	static void SetGesture(Command command, Gesture::GestureEnum gesture);
	static void SetControllerButton(Command command, uint8_t button);
	static void SetControllerTrigger(Command command, uint8_t trigger, bool positive);

	// Get the description or keycode name for this command. If this command is
	// a combination of more than one command, an empty string is returned.
	const std::string &Description() const;
	const std::string &KeyName() const;
	const std::string &GestureName() const;
	const std::string ButtonName() const;
	const std::string &Icon() const;
	bool HasBinding() const;
	bool HasConflict() const;

	// Load this command from an input file (for testing or scripted missions).
	void Load(const DataNode &node);

	// Reset this to an empty command.
	void Clear();
	// Clear, set, or check the given bits. This ignores the turn field.
	void Clear(Command command);
	void Set(Command command);
	bool Has(Command command) const;
	// Get the commands that are set in this and in the given command.
	Command And(Command command) const;
	// Get the commands that are set in this and not in the given command.
	Command AndNot(Command command) const;

	// Get or set the turn amount. The amount must be between -1 and 1, but it
	// can be a fractional value to allow finer control.
	void SetTurn(double amount);
	double Turn() const;

	// Check if any bits are set in this command (including a nonzero turn).
	explicit operator bool() const;
	bool operator!() const;
	// This operator is just provided to allow commands to be used in a map.
	bool operator<(const Command &command) const;

	// Get the commands that are set in either of these commands.
	Command operator|(const Command &command) const;
	Command &operator|=(const Command &command);
	bool operator==(const Command &command) const { return command.state == state && command.turn == turn; }

	// Allow UI's to simulate keyboard input
	static void InjectOnce(const Command& command, bool next = false);
	static void InjectOnceNoEvent(const Command& command);
	static void InjectSet(const Command& command);
	static void InjectClear();
	static void InjectUnset(const Command& command);
	static Command Get(const std::string& description);
	// Register an event, and return its value. This event gets triggered
	// whenever we call InjectSet/InjetUnset
	static uint32_t EventID();

	static void DebugDumpGestures();

private:
	explicit Command(uint64_t state);
	Command(uint64_t state, const std::string &text, const std::string &icon = "");


private:
	// The key commands are stored in a single bitmask with
	// 64 bits for key commands.
	uint64_t state = 0;
	// Turning amount is stored as a separate double to allow fractional values.
	double turn = 0.;

	// If we want to simulate input from the ui, place it here to be read later
	static std::atomic<uint64_t> simulated_command;
	static std::atomic<uint64_t> simulated_command_once;
	static bool simulated_command_skip;
};
