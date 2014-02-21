/* Key.cpp
Michael Zahniser, 20 Feb 2014

Function definitions for the Key class.
*/

#include "Key.h"

#include "DataFile.h"

#include <SDL/SDL.h>

#include <fstream>
#include <map>

using namespace std;

namespace {
	static const string DESCRIPTION[] = {
		"Show main menu",
		"Forward thrust",
		"Turn left",
		"Turn right",
		"Reverse",
		"Fire primary weapon",
		"Fire secondary weapon",
		"Select secondary weapon",
		"Land on planet / station",
		"Board selected ship",
		"Talk to selected ship",
		"Scan selected ship",
		"Initiate hyperspace jump",
		"Select next ship",
		"Select nearest hostile ship",
		"Deploy / recall fighters",
		"Order escorts to attack target",
		"Order escorts to defend you",
		"View star map",
		"View player info"
	};
}



// Default constructor.
Key::Key()
{
	// Resize the key mapping vector to the proper size. It will not be cleared
	// or reset when loading, so that you can load defaults from one file and
	// local modifications from another.
	keys.resize(END - MENU, 0);
}



// Get the current key state, as a bitmask.
int Key::State() const
{
	int count = 0;
	const Uint8 *state = SDL_GetKeyState(&count);
	
	int result = 0;
	int bit = 1;
	for(int key : keys)
	{
		if(state[key])
			result |= bit;
		bit <<= 1;
	}
	return result;
}



// Load the key preferences from a file.
void Key::Load(const string &path)
{
	filePath = path;
	DataFile file(path);
	
	map<string, int> commands;
	for(int i = MENU; i != END; ++i)
		commands[DESCRIPTION[i]] = i;
	
	for(const DataFile::Node &node : file)
	{
		auto it = commands.find(node.Token(0));
		if(it != commands.end() && node.Size() >= 2)
			keys[it->second] = node.Value(1);
	}
}



// Write the key preferences to the file they were loaded from.
void Key::Save() const
{
	if(!filePath.empty())
		Save(filePath);
}



// Write the key preferences to a different file.
void Key::Save(const string &path) const
{
	ofstream out(path);
	
	for(unsigned i = 0; i < keys.size(); ++i)
		out << '"' << DESCRIPTION[i] << '"' << ' ' << keys[i] << '\n';
}



// Set which key maps to which command.
void Key::Set(Command command, int key)
{
	keys[command] = key;
}



// Get the key that triggers the given command.
int Key::Get(Command command) const
{
	return keys[command];
}



// Get a string describing the given command.
const string &Key::Description(Command command)
{
	return DESCRIPTION[command];
}
