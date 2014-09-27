/* Key.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef KEY_H_
#define KEY_H_

#include <string>
#include <vector>



// Class mapping key presses to specific commands / actions.
class Key {
public:
	enum Command {
		MENU = 0,
		FORWARD,
		LEFT,
		RIGHT,
		BACK,
		PRIMARY,
		SECONDARY,
		SELECT,
		LAND,
		BOARD,
		HAIL,
		SCAN,
		JUMP,
		TARGET,
		NEAREST,
		DEPLOY,
		AFTERBURNER,
		CLOAK,
		MAP,
		INFO,
		FULLSCREEN,
		END
	};
	
	
public:
	// Default constructor.
	Key();
	
	// Get the current key state, as a bitmask.
	int State() const;
	// Get the bit associated with the given command.
	static int Bit(Command command);
	
	// Load the key preferences from a file.
	void Load(const std::string &path);
	// Write the key preferences to the file they were loaded from.
	void Save() const;
	// Write the key preferences to a different file.
	void Save(const std::string &path) const;
	
	// Set which key maps to which command.
	void Set(Command command, int key);
	// Get the key that triggers the given command.
	int Get(Command command) const;
	// Get the name of the key that triggers the given command.
	std::string Name(Command command) const;
	
	// Get a string describing the given command.
	static const std::string &Description(Command command);
	
	
private:
	std::string filePath;
	std::vector<int> keys;
};



#endif
