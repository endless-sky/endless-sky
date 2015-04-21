/* Command.cpp
Copyright (c) 2015 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "Command.h"

#include "DataFile.h"
#include "DataNode.h"

#include <SDL2/SDL.h>

#include <algorithm>
#include <fstream>
#include <map>

using namespace std;

namespace {
	map<Command, string> description;
	map<Command, string> keyName;
	map<int, Command> commandForKeycode;
	map<Command, int> keycodeForCommand;
	map<int, int> keycodeCount;
}

const Command Command::NONE(0, "");
const Command Command::MENU(1uL << 0, "Show main menu");
const Command Command::FORWARD(1uL << 1, "Forward thrust");
const Command Command::LEFT(1uL << 2, "Turn left");
const Command Command::RIGHT(1uL << 3, "Turn right");
const Command Command::BACK(1uL << 4, "Reverse");
const Command Command::PRIMARY(1uL << 5, "Fire primary weapon");
const Command Command::SECONDARY(1uL << 6, "Fire secondary weapon");
const Command Command::SELECT(1uL << 7, "Select secondary weapon");
const Command Command::LAND(1uL << 8, "Land on planet / station");
const Command Command::BOARD(1uL << 9, "Board selected ship");
const Command Command::HAIL(1uL << 10, "Talk to selected ship");
const Command Command::SCAN(1uL << 11, "Scan selected ship");
const Command Command::JUMP(1uL << 12, "Initiate hyperspace jump");
const Command Command::TARGET(1uL << 13, "Select next ship");
const Command Command::NEAREST(1uL << 14, "Select nearest hostile ship");
const Command Command::DEPLOY(1uL << 15, "Deploy / recall fighters");
const Command Command::AFTERBURNER(1uL << 16, "Fire afterburner");
const Command Command::CLOAK(1uL << 17, "Toggle cloaking device");
const Command Command::MAP(1uL << 18, "View star map");
const Command Command::INFO(1uL << 19, "View player info");
const Command Command::FULLSCREEN(1uL << 20, "Toggle fullscreen");
const Command Command::FIGHT(1uL << 21, "Fleet: Fight my target");
const Command Command::GATHER(1uL << 22, "Fleet: Gather around me");
const Command Command::HOLD(1uL << 23, "Fleet: Hold position");
const Command Command::WAIT(1uL << 24, "");



// Assume SDL_Keycode is a signed int, so I don't need to include SDL.h.
Command::Command(int keycode)
{
	auto it = commandForKeycode.find(keycode);
	if(it != commandForKeycode.end())
		*this = it->second;
}



// Read the current keyboard state.
void Command::ReadKeyboard()
{
	Clear();
	const Uint8 *keyDown = SDL_GetKeyboardState(nullptr);
	
	// Each command can only have one keycode, but misconfigured settings can
	// temporarily cause one keycode to be used for two commands.
	for(const auto &it : keycodeForCommand)
		if(keyDown[SDL_GetScancodeFromKey(it.second)])
			*this |= it.first;
}



// Load or save the keyboard preferences.
void Command::LoadSettings(const string &path)
{
	DataFile file(path);
	
	map<string, Command> commands;
	for(const auto &it : description)
		commands[it.second] = it.first;
	
	// Each command can only have one keycode, but misconfigured settings can
	// temporarily cause one keycode to be used for two commands.
	for(const DataNode &node : file)
	{
		auto it = commands.find(node.Token(0));
		if(it != commands.end() && node.Size() >= 2)
		{
			Command command = it->second;
			int keycode = node.Value(1);
			keycodeForCommand[command] = keycode;
			keyName[command] = SDL_GetKeyName(keycode);
		}
	}
	
	commandForKeycode.clear();
	keycodeCount.clear();
	for(const auto &it : keycodeForCommand)
	{
		commandForKeycode[it.second] = it.first;
		++keycodeCount[it.second];
	}
}



void Command::SaveSettings(const string &path)
{
	ofstream out(path);
	
	for(const auto &it : commandForKeycode)
	{
		auto dit = description.find(it.second);
		if(dit != description.end())
			out << '"' << dit->second << '"' << ' ' << it.first << '\n';
	}
}



void Command::SetKey(Command command, int keycode)
{
	// Always reset *all* the mappings when one is set. That way, if two commands
	// are mapped to the same key and you change one of them, the other stays mapped.
	keycodeForCommand[command] = keycode;
	keyName[command] = SDL_GetKeyName(keycode);
	
	commandForKeycode.clear();
	keycodeCount.clear();
	
	for(const auto &it : keycodeForCommand)
	{
		commandForKeycode[it.second] = it.first;
		++keycodeCount[it.second];
	}
}



// Get the description or keycode name for this command. If this command is
// a combination of more than one command, an empty string is returned.
const string &Command::Description() const
{
	static const string empty = "";
	auto it = description.find(*this);
	return (it == description.end() ? empty : it->second);
}



const string &Command::KeyName() const
{
	static const string empty = "";
	auto it = keyName.find(*this);
	return (it == keyName.end() ? empty : it->second);
}



bool Command::HasConflict() const
{
	auto it = keycodeForCommand.find(*this);
	if(it == keycodeForCommand.end())
		return false;
	
	auto cit = keycodeCount.find(it->second);
	return (cit != keycodeCount.end() && cit->second > 1);
}



// Clear all commands (i.e. set to the default state).
void Command::Clear()
{
	*this = Command();
}



// Clear any commands that are set in the given command.
void Command::Clear(Command command)
{
	state &= ~command.state;
}



// Set any commands that are set in the given command.
void Command::Set(Command command)
{
	state |= command.state;
}



// Check if any of the given command's bits that are set, are also set here.
bool Command::Has(Command command) const
{
	return (state & command.state);
}



// Get the commands that are set in this and not in the given command.
Command Command::AndNot(Command command) const
{
	return (state & ~command.state);
}



// Get or set the turn amount.
void Command::SetTurn(double amount)
{
	turn = max(-1., min(1., amount));
}



double Command::Turn() const
{
	return turn;
}



// Get or set the fire commands.
bool Command::HasFire(int index)
{
	return state & ((1ull << 32) << index);
}



void Command::SetFire(int index)
{
	state |= ((1ull << 32) << index);
}



// Check if any bits are set in this command (including a nonzero turn).
Command::operator bool() const
{
	return !!*this;
}



bool Command::operator!() const
{
	return !state && !turn;
}



// For sorting commands:
bool Command::operator<(const Command &command) const
{
	return (state < command.state);
}



// Get the commands that are set in either of these commands.
Command Command::operator|(const Command &command) const
{
	Command result = *this;
	result |= command;
	return result;
}



Command &Command::operator|=(const Command &command)
{
	state |= command.state;
	if(command.turn)
		turn = command.turn;
	return *this;
}



Command::Command(uint64_t state)
	: state(state)
{
}



Command::Command(uint64_t state, const string &text)
	: state(state)
{
	if(!text.empty())
		description[*this] = text;
}
