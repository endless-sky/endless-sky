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
#include "DataWriter.h"
#include "text/Format.h"

#include <SDL2/SDL.h>

#include <algorithm>
#include <cmath>
#include <map>

using namespace std;

namespace {
	// These lookup tables make it possible to map a command to its description,
	// the name of the key it is mapped to, or the SDL keycode it is mapped to.
	map<Command, string> description;
	map<Command, string> keyName;
	map<int, Command> commandForKeycode;
	map<Command, int> keycodeForCommand;
	// Keep track of any keycodes that are mapped to multiple commands, in order
	// to display a warning to the player.
	map<int, int> keycodeCount;
}

// Command enumeration, including the descriptive strings that are used for the
// commands both in the preferences panel and in the saved key settings.
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
const Command Command::FASTFORWARD(1uL << 21, "Toggle fast-forward");
const Command Command::FIGHT(1uL << 22, "Fleet: Fight my target");
const Command Command::GATHER(1uL << 23, "Fleet: Gather around me");
const Command Command::HOLD(1uL << 24, "Fleet: Hold position");
const Command Command::AMMO(1uL << 25, "Fleet: Toggle ammo usage");
const Command Command::WAIT(1uL << 26, "");
const Command Command::STOP(1ul << 27, "");
const Command Command::SHIFT(1uL << 28, "");



// In the given text, replace any instances of command names (in angle brackets)
// with key names (in quotes).
string Command::ReplaceNamesWithKeys(const string &text)
{
	map<string, string> subs;
	for(const auto &it : description)
		subs['<' + it.second + '>'] = '"' + keyName[it.first] + '"';
	
	return Format::Replace(text, subs);
}



// Create a command representing whatever is mapped to the given key code.
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
	// temporarily cause one keycode to be used for two commands. Also, more
	// than one key can be held down at once.
	for(const auto &it : keycodeForCommand)
		if(keyDown[SDL_GetScancodeFromKey(it.second)])
			*this |= it.first;
	
	// Check whether the `Shift` modifier key was pressed for this step.
	if(SDL_GetModState() & KMOD_SHIFT)
		*this |= SHIFT;
}



// Load the keyboard preferences.
void Command::LoadSettings(const string &path)
{
	DataFile file(path);
	
	// Create a map of command names to Command objects in the enumeration above.
	map<string, Command> commands;
	for(const auto &it : description)
		commands[it.second] = it.first;
	
	// Each command can only have one keycode, one keycode can be assigned
	// to multiple commands.
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
	
	// Regenerate the lookup tables.
	commandForKeycode.clear();
	keycodeCount.clear();
	for(const auto &it : keycodeForCommand)
	{
		commandForKeycode[it.second] = it.first;
		++keycodeCount[it.second];
	}
}



// Save the keyboard preferences.
void Command::SaveSettings(const string &path)
{
	DataWriter out(path);
	
	for(const auto &it : keycodeForCommand)
	{
		auto dit = description.find(it.first);
		if(dit != description.end())
			out.Write(dit->second, it.second);
	}
}



// Set the key that is mapped to the given command.
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



// Get the description of this command. If this command is a combination of more
// than one command, an empty string is returned.
const string &Command::Description() const
{
	static const string empty;
	auto it = description.find(*this);
	return (it == description.end() ? empty : it->second);
}



// Get the name of the key that is mapped to this command. If this command is
// a combination of more than one command, an empty string is returned.
const string &Command::KeyName() const
{
	static const string empty;
	auto it = keyName.find(*this);
	return (it == keyName.end() ? empty : it->second);
}



// Check whether this is the only command mapped to the key it is mapped to.
bool Command::HasConflict() const
{
	auto it = keycodeForCommand.find(*this);
	if(it == keycodeForCommand.end())
		return false;
	
	auto cit = keycodeCount.find(it->second);
	return (cit != keycodeCount.end() && cit->second > 1);
}



// Load this command from an input file (for testing or scripted missions).
void Command::Load(const DataNode &node)
{
	for(int i = 1; i < node.Size(); ++i)
	{
		static const map<string, Command> lookup = {
			{"menu", Command::MENU},
			{"forward", Command::FORWARD},
			{"left", Command::LEFT},
			{"right", Command::RIGHT},
			{"back", Command::BACK},
			{"primary", Command::PRIMARY},
			{"secondary", Command::SECONDARY},
			{"select", Command::SELECT},
			{"land", Command::LAND},
			{"board", Command::BOARD},
			{"hail", Command::HAIL},
			{"scan", Command::SCAN},
			{"jump", Command::JUMP},
			{"target", Command::TARGET},
			{"nearest", Command::NEAREST},
			{"deploy", Command::DEPLOY},
			{"afterburner", Command::AFTERBURNER},
			{"cloak", Command::CLOAK},
			{"map", Command::MAP},
			{"info", Command::INFO},
			{"fight", Command::FIGHT},
			{"gather", Command::GATHER},
			{"hold", Command::HOLD},
			{"ammo", Command::AMMO}
		};
		
		auto it = lookup.find(node.Token(i));
		if(it != lookup.end())
			Set(it->second);
		else
			node.PrintTrace("Skipping unrecognized command \"" + node.Token(i) + "\":");
	}
}



// Reset this to an empty command.
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



// Get the commands that are set in this and in the given command.
Command Command::And(Command command) const
{
	return Command(state & command.state);
}



// Get the commands that are set in this and not in the given command.
Command Command::AndNot(Command command) const
{
	return Command(state & ~command.state);
}



// Set the turn direction and amount to a value between -1 and 1.
void Command::SetTurn(double amount)
{
	turn = max(-1., min(1., amount));
}



// Get the turn amount.
double Command::Turn() const
{
	return turn;
}



// Check if this command includes a command to fire the given weapon.
bool Command::HasFire(int index) const
{
	if(index < 0 || index >= 32)
		return false;
	
	return state & ((1ull << 32) << index);
}



// Add to this set of commands a command to fire the given weapon.
void Command::SetFire(int index)
{
	if(index < 0 || index >= 32)
		return;
	
	state |= ((1ull << 32) << index);
}



// Check if any weapons are firing.
bool Command::IsFiring() const
{
	return (state & 0xFFFFFFFF00000000ull);
}



// Set the turn rate of the turret with the given weapon index. A value of
// -1 or 1 means to turn at the full speed the turret is capable of.
double Command::Aim(int index) const
{
	if(index < 0 || index >= 32)
		return 0;
	
	return aim[index] / 127.;
}



void Command::SetAim(int index, double amount)
{
	if(index < 0 || index >= 32)
		return;
	
	aim[index] = round(127. * max(-1., min(1., amount)));
}



// Check if any bits are set in this command (including a nonzero turn).
Command::operator bool() const
{
	return !!*this;
}



// Check whether this command is entirely empty.
bool Command::operator!() const
{
	return !state && !turn;
}



// For sorting commands (e.g. so a command can be the key in a map):
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



// Combine everything in the given command with this command. If the given
// command has a nonzero turn set, it overrides this command's turn value.
Command &Command::operator|=(const Command &command)
{
	state |= command.state;
	if(command.turn)
		turn = command.turn;
	return *this;
}



// Private constructor.
Command::Command(uint64_t state)
	: state(state)
{
}



// Private constructor that also stores the given description in the lookup
// table. (This is used for the enumeration at the top of this file.)
Command::Command(uint64_t state, const string &text)
	: state(state)
{
	if(!text.empty())
		description[*this] = text;
}
