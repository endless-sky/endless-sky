/* Command.cpp
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

#include "Command.h"

#include "DataFile.h"
#include "DataNode.h"
#include "DataWriter.h"
#include "GamePad.h"
#include "text/Format.h"

#include <SDL2/SDL.h>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <map>

using namespace std;

namespace {
	// These lookup tables make it possible to map a command to its description,
	// the name of the key it is mapped to, or the SDL keycode it is mapped to.
	map<Command, string> description;
	map<Command, string> keyName;
	map<Command, string> iconName;
	map<int, Command> commandForKeycode;
	map<Gesture::GestureEnum, Command> commandForGesture;
	map<uint8_t, Command> commandForControllerButton;
	map<std::pair<uint8_t, bool>, Command> commandForControllerTrigger;
	map<Command, int> keycodeForCommand;
	// Keep track of any keycodes that are mapped to multiple commands, in order
	// to display a warning to the player.
	map<int, int> keycodeCount;
	// Need a uint64_t 1 to generate Commands.
	const uint64_t ONE = 1;
}



// Command enumeration, including the descriptive strings that are used for the
// commands both in the preferences panel and in the saved key settings.
const Command Command::NONE(0, "");
const Command Command::MENU(ONE << 0, "Show main menu", "ui/icon_exit");
const Command Command::FORWARD(ONE << 1, "Forward thrust");
const Command Command::LEFT(ONE << 2, "Turn left");
const Command Command::RIGHT(ONE << 3, "Turn right");
const Command Command::BACK(ONE << 4, "Reverse");
const Command Command::MOUSE_TURNING_HOLD(ONE << 5, "Mouse turning (hold)");
const Command Command::PRIMARY(ONE << 6, "Fire primary weapon", "ui/icon_fire");
const Command Command::TURRET_TRACKING(ONE << 7, "Toggle turret tracking");
const Command Command::SECONDARY(ONE << 8, "Fire secondary weapon", "ui/icon_secondary");
const Command Command::SELECT(ONE << 9, "Select secondary weapon");
const Command Command::LAND(ONE << 10, "Land on planet / station", "ui/icon_land");
const Command Command::BOARD(ONE << 11, "Board selected ship", "ui/icon_board");
const Command Command::HAIL(ONE << 12, "Talk to selected ship", "ui/icon_talk");
const Command Command::SCAN(ONE << 13, "Scan selected ship", "ui/icon_scan");
const Command Command::JUMP(ONE << 14, "Initiate hyperspace jump", "ui/icon_jump");
const Command Command::FLEET_JUMP(ONE << 15, "");
const Command Command::TARGET(ONE << 16, "Select next ship");
const Command Command::NEAREST(ONE << 17, "Select nearest hostile ship");
const Command Command::NEAREST_ASTEROID(ONE << 18, "Select nearest asteroid");
const Command Command::DEPLOY(ONE << 19, "Deploy / recall fighters", "ui/icon_deploy");
const Command Command::AFTERBURNER(ONE << 20, "Fire afterburner");
const Command Command::CLOAK(ONE << 21, "Toggle cloaking device", "ui/icon_cloak");
const Command Command::MAP(ONE << 22, "View star map", "ui/icon_map");
const Command Command::INFO(ONE << 23, "View player info", "ui/icon_info");
const Command Command::MESSAGE_LOG(ONE << 24, "View message log");
const Command Command::FULLSCREEN(ONE << 25, "Toggle fullscreen");
const Command Command::FASTFORWARD(ONE << 26, "Toggle fast-forward", "ui/icon_fast_forward");
const Command Command::HELP(ONE << 27, "Show help");
const Command Command::FIGHT(ONE << 28, "Fleet: Fight my target", "ui/icon_fleet_fight");
const Command Command::GATHER(ONE << 29, "Fleet: Gather around me", "ui/icon_fleet_gather");
const Command Command::HOLD(ONE << 30, "Fleet: Hold position", "ui/icon_fleet_stop");
const Command Command::HARVEST(ONE << 31, "Fleet: Harvest flotsam", "ui/icon_fleet_harvest");
const Command Command::AMMO(ONE << 32, "Fleet: Toggle ammo usage");
const Command Command::AUTOSTEER(ONE << 33, "Auto steer");
const Command Command::WAIT(ONE << 34, "");
const Command Command::STOP(ONE << 35, "Stop", "ui/icon_fleet_stop");
const Command Command::SHIFT(ONE << 36, "");

std::atomic<uint64_t> Command::simulated_command{};
std::atomic<uint64_t> Command::simulated_command_once{};
bool Command::simulated_command_skip = false;


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



// Create a command representing whatever is mapped to the given key code.
Command::Command(const SDL_Event &event)
{
	if(event.type == EventID())
	{
		state = reinterpret_cast<const CommandEvent&>(event).state;
	}
}



// Create a command representing whatever is mapped to the given gesture
Command::Command(Gesture::GestureEnum gesture)
{
	auto it = commandForGesture.find(gesture);
	if(it != commandForGesture.end())
		*this = it->second;
}



// Create a command representing the given axis trigger
Command Command::FromTrigger(uint8_t axis, bool positive)
{
	auto it = commandForControllerTrigger.find(std::make_pair(axis, positive));
	if(it != commandForControllerTrigger.end())
		return it->second;
	return Command();
}


// Create a command representing the given controller button
Command Command::FromButton(uint8_t button)
{
	auto it = commandForControllerButton.find(button);
	if(it != commandForControllerButton.end())
		return it->second;
	return Command();
}



// Read the current keyboard state.
void Command::ReadKeyboard()
{
	Clear();

	// inject simulated commands
	state = simulated_command.load(std::memory_order_relaxed);
	// inject simulated once commands
	if(simulated_command_skip)
	{
		// we want to skip the first Read, and inject on the next one
		simulated_command_skip = false;
	}
	else
		state |= simulated_command_once.exchange(0);

	const Uint8 *keyDown = SDL_GetKeyboardState(nullptr);

	// Read commands from the game controller
	const GamePad::Buttons& gamepadButtons = GamePad::Held();
	for(auto &kv : commandForControllerButton)
	{
		if(static_cast<size_t>(kv.first) < sizeof(gamepadButtons) / sizeof(*gamepadButtons))
		{
			if(gamepadButtons[kv.first])
				*this |= kv.second;
		}
	}
	// Read Trigger values (this may include joystick axes as well)
	for(auto &kv : commandForControllerTrigger)
	{
		if(kv.first.first < SDL_CONTROLLER_AXIS_MAX)
		{
			if(GamePad::Trigger(kv.first.first, kv.first.second))
				*this |= kv.second;
		}
	}


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

	bool file_has_buttons = false;
	bool file_has_triggers = false;
	bool file_has_gestures = false;

	// Each command can only have one keycode, one keycode can be assigned
	// to multiple commands.
	for(const DataNode &node : file)
	{
		auto it = commands.find(node.Token(0));
		if(it != commands.end() && node.Size() >= 2)
		{
			Command command = it->second;
			if(node.Token(1) == "gesture" && node.Size() >= 3)
			{
				if(!file_has_gestures)
				{
					commandForGesture.clear();
					file_has_gestures = true;
				}
				SetGesture(command, static_cast<Gesture::GestureEnum>(node.Value(2)));
			}
			else if(node.Token(1) == "controller_button" && node.Size() >= 3)
			{
				if(!file_has_buttons)
				{
					commandForControllerButton.clear();
					file_has_buttons = true;
				}
				auto button = static_cast<SDL_GameControllerButton>(node.Value(2));
				if(button >= 0 && button < SDL_CONTROLLER_BUTTON_MAX)
				{
					commandForControllerButton[button] = command;
				}
			}
			else if(node.Token(1) == "controller_trigger" && node.Size() >= 4)
			{
				if(!file_has_triggers)
				{
					commandForControllerTrigger.clear();
					file_has_triggers = true;
				}
				auto axis = static_cast<SDL_GameControllerAxis>(node.Value(2));
				bool positive = node.BoolValue(3);
				if(axis >= 0 && axis < SDL_CONTROLLER_AXIS_MAX)
				{
					commandForControllerTrigger[std::make_pair(axis, positive)] = command;
				}
			}
			else
			{
				int keycode = node.Value(1);
				keycodeForCommand[command] = keycode;
				keyName[command] = SDL_GetKeyName(keycode);
			}
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

	for(const auto& kv : commandForGesture)
	{
		auto dit = description.find(kv.second);
		if(dit != description.end())
			out.Write(dit->second, "gesture", static_cast<int>(kv.first));
	}
	for(const auto& kv : commandForControllerButton)
	{
		auto dit = description.find(kv.second);
		if(dit != description.end())
			out.Write(dit->second, "controller_button", static_cast<int>(kv.first));
	}
	for(const auto& kv : commandForControllerTrigger)
	{
		auto dit = description.find(kv.second);
		if(dit != description.end())
			out.Write(dit->second, "controller_trigger", static_cast<int>(kv.first.first), kv.first.second ? "1": "0");
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



// Set the gesture that is mapped to the given command
void Command::SetGesture(Command command, Gesture::GestureEnum gesture)
{
	for(auto it = commandForGesture.begin(); it != commandForGesture.end();)
	{
		if(it->second == command)
		{
			it = commandForGesture.erase(it);
		}
		else
		{
			++it;
		}
	}

	if(gesture != Gesture::NONE)
	{
		commandForGesture[gesture] = command;
	}
}



// Set the gesture that is mapped to the given command
void Command::SetControllerButton(Command command, uint8_t button)
{
	// Erase any buttons or triggers for this command
	for(auto it = commandForControllerButton.begin(); it != commandForControllerButton.end();)
	{
		if(it->second == command)
		{
			it = commandForControllerButton.erase(it);
		}
		else
		{
			++it;
		}
	}
	for(auto it = commandForControllerTrigger.begin(); it != commandForControllerTrigger.end();)
	{
		if(it->second == command)
		{
			it = commandForControllerTrigger.erase(it);
		}
		else
		{
			++it;
		}
	}

	if(button < GamePad::BUTTON_MAX && button != GamePad::BUTTON_INVALID)
	{
		commandForControllerButton[button] = command;
	}
}



// Set the gesture that is mapped to the given command
void Command::SetControllerTrigger(Command command, uint8_t axis, bool positive)
{
	// Erase any buttons or triggers for this command
	for(auto it = commandForControllerButton.begin(); it != commandForControllerButton.end();)
	{
		if(it->second == command)
		{
			it = commandForControllerButton.erase(it);
		}
		else
		{
			++it;
		}
	}
	for(auto it = commandForControllerTrigger.begin(); it != commandForControllerTrigger.end();)
	{
		if(it->second == command)
		{
			it = commandForControllerTrigger.erase(it);
		}
		else
		{
			++it;
		}
	}

	if(axis < GamePad::AXIS_MAX && axis != GamePad::AXIS_INVALID)
	{
		commandForControllerTrigger[std::make_pair(axis, positive)] = command;
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
	static const string empty = "(Not Set)";
	auto it = keyName.find(*this);
	if(it != keyName.end())
		return it->second;
	return empty;
}



const std::string &Command::GestureName() const
{
	static const string empty = "(Not Set)";
	
	for(auto& kv: commandForGesture)
	{
		if(kv.second == *this)
		{
			return Gesture::Description(kv.first);
		}
	}
	return empty;
}



const std::string Command::ButtonName() const
{
	// Only one button or trigger should be set
	for(auto& kv: commandForControllerButton)
	{
		if(kv.second == *this)
		{
			const char* description = GamePad::ButtonDescription(kv.first);
			return description ? description : "(unknown)";

		}
	}

	for(auto& kv: commandForControllerTrigger)
	{
		if(kv.second == *this)
		{
			const char* description = GamePad::ButtonDescription(kv.first.first);
			if(description)
				return std::string(description ? description : "(unknown)") + (kv.first.second ? " +" : " -");
		}
	}
	return "(Not Set)";
}



// Retrieve the icon associated with this command (if any)
const std::string& Command::Icon() const
{
	static string EMPTY;
	auto it = iconName.find(*this);
	return it == iconName.end() ? EMPTY : it->second;
}



// Check if the key has no binding.
bool Command::HasBinding() const
{
	auto it = keyName.find(*this);

	for(auto& kv: commandForGesture)
	{
		if(kv.second == *this)
		{
			return true;
		}
	}

	if(it == keyName.end())
		return false;
	if(it->second.empty())
		return false;
	return true;
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
			{"none", Command::NONE},
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
			{"mouseturninghold", Command::MOUSE_TURNING_HOLD},
			{"fleet jump", Command::FLEET_JUMP},
			{"target", Command::TARGET},
			{"nearest", Command::NEAREST},
			{"deploy", Command::DEPLOY},
			{"afterburner", Command::AFTERBURNER},
			{"cloak", Command::CLOAK},
			{"map", Command::MAP},
			{"info", Command::INFO},
			{"fullscreen", Command::FULLSCREEN},
			{"fastforward", Command::FASTFORWARD},
			{"fight", Command::FIGHT},
			{"gather", Command::GATHER},
			{"hold", Command::HOLD},
			{"ammo", Command::AMMO},
			{"nearest asteroid", Command::NEAREST_ASTEROID},
			{"wait", Command::WAIT},
			{"stop", Command::STOP},
			{"shift", Command::SHIFT}
		};

		auto it = lookup.find(node.Token(i));
		if(it != lookup.end())
			Set(it->second);
		else
			node.PrintTrace("Warning: Skipping unrecognized command \"" + node.Token(i) + "\":");
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
Command::Command(uint64_t state, const string &text, const string &icon)
	: state(state)
{
	if(!text.empty())
		description[*this] = text;

	if(!icon.empty())
		iconName[*this] = icon;
}



// Retrieve a command based on its description.
Command Command::Get(const std::string& command_description)
{
	for (auto& command: description)
	{
		if (command_description == command.second)
		{
			return command.first;
		}
	}
	return Command::NONE;
}



// Simulate a keyboard press for commands
void Command::InjectSet(const Command& command)
{
	simulated_command.fetch_or(command.state, std::memory_order_relaxed);
	SDL_Event event{};
	auto& cevent = reinterpret_cast<CommandEvent&>(event);
	cevent.type = EventID();
	cevent.state = command.state;
	cevent.pressed = SDL_PRESSED;
	SDL_PushEvent(&event);
}



// Simulate a keyboard press for commands
void Command::InjectOnce(const Command& command, bool next)
{
	simulated_command_once.fetch_or(command.state, std::memory_order_relaxed);
	simulated_command_skip = next;
	SDL_Event event{};
	auto& cevent = reinterpret_cast<CommandEvent&>(event);
	cevent.type = EventID();
	// command.state is 64 bits, but WindowID is 32 bits.
	cevent.state = command.state;
	cevent.pressed = SDL_PRESSED;
	SDL_PushEvent(&event);
	cevent.pressed = SDL_RELEASED;
	SDL_PushEvent(&event);
}



// Simulate key held, without pushing the associated event
void Command::InjectOnceNoEvent(const Command& command)
{
	simulated_command_once.fetch_or(command.state, std::memory_order_relaxed);
}



// Clear any set commands
void Command::InjectClear()
{
	SDL_Event event{};
	auto& cevent = reinterpret_cast<CommandEvent&>(event);
	cevent.type = EventID();
	cevent.state = simulated_command.exchange(0);
	cevent.pressed = SDL_RELEASED;
	SDL_PushEvent(&event);
}




// Simulate a keyboard release for commands
void Command::InjectUnset(const Command& command)
{
	simulated_command.fetch_and(~command.state, std::memory_order_relaxed);
	SDL_Event event{};
	event.type = EventID();
	event.key.windowID = command.state;
	event.key.state = SDL_RELEASED;
	SDL_PushEvent(&event);
}



// Register a set of events with SDL's event loop
uint32_t Command::EventID()
{
	static uint32_t command_event = SDL_RegisterEvents(1);
	return command_event;
}
