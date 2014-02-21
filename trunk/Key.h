/* Key.h
Michael Zahniser, 20 Feb 2014

Class holding key values and preferences.
*/

#ifndef KEY_H_INCLUDED
#define KEY_H_INCLUDED

#include <string>
#include <vector>



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
		ATTACK,
		DEFEND,
		MAP,
		INFO,
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
	
	// Get a string describing the given command.
	static const std::string &Description(Command command);
	
	
private:
	std::string filePath;
	std::vector<int> keys;
};



#endif
