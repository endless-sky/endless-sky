/* Command.h
Copyright (c) 2015 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef COMMAND_H_
#define COMMAND_H_

#include <cstdint>
#include <string>



// Class mapping key presses to specific commands / actions. The player can
// change the mappings for most of these keys in the preferences panel.
class Command {
public:
	static const Command NONE;
	static const Command MENU;
	static const Command FORWARD;
	static const Command LEFT;
	static const Command RIGHT;
	static const Command BACK;
	static const Command PRIMARY;
	static const Command SECONDARY;
	static const Command SELECT;
	static const Command LAND;
	static const Command BOARD;
	static const Command HAIL;
	static const Command SCAN;
	static const Command JUMP;
	static const Command TARGET;
	static const Command NEAREST;
	static const Command DEPLOY;
	static const Command AFTERBURNER;
	static const Command CLOAK;
	static const Command MAP;
	static const Command INFO;
	static const Command FULLSCREEN;
	static const Command FIGHT;
	static const Command GATHER;
	static const Command HOLD;
	static const Command WAIT;
	static const Command STOP;
	
public:
	Command() = default;
	// Assume SDL_Keycode is a signed int, so I don't need to include SDL.h.
	explicit Command(int keycode);
	
	// Read the current keyboard state.
	void ReadKeyboard();
	
	// Load or save the keyboard preferences.
	static void LoadSettings(const std::string &path);
	static void SaveSettings(const std::string &path);
	static void SetKey(Command command, int keycode);
	
	// Get the description or keycode name for this command. If this command is
	// a combination of more than one command, an empty string is returned.
	const std::string &Description() const;
	const std::string &KeyName() const;
	bool HasConflict() const;
	
	// Clear all commands (i.e. set to the default state).
	void Clear();
	// Clear, set, or check the given bits. This ignores the turn field.
	void Clear(Command command);
	void Set(Command command);
	bool Has(Command command) const;
	// Get the commands that are set in this and not in the given command.
	Command AndNot(Command command) const;
	
	// Get or set the turn amount.
	void SetTurn(double amount);
	double Turn() const;
	// Get or set the fire commands.
	bool HasFire(int index) const;
	void SetFire(int index);
	// Check if any weapons are firing.
	bool IsFiring() const;
	
	// Check if any bits are set in this command (including a nonzero turn).
	explicit operator bool() const;
	bool operator!() const;
	// For sorting commands:
	bool operator<(const Command &command) const;
	
	// Get the commands that are set in either of these commands.
	Command operator|(const Command &command) const;
	Command &operator|=(const Command &command);
	
	
private:
	Command(uint64_t state);
	Command(uint64_t state, const std::string &text);
	
	
private:
	uint64_t state = 0;
	double turn = 0.;
};



#endif
