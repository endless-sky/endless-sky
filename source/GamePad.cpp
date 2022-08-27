/* GamePad.cpp
Copyright (c) 2022 by Kari Pahula

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "GamePad.h"

#include "pi.h"
#include "Preferences.h"

#include <chrono>
#include <cmath>
#include <map>
#include <mutex>

#include <SDL2/SDL.h>

using namespace std;

namespace {
	// Gradient thrust is available but most of the time you'd just floor it.
	const double THRUST_MULTI = 2.;

	mutex gamePadMutex;

	GamePad singleton;
}



GamePad &GamePad::Singleton()
{
	return singleton;
}



// Handle an event. Will update GamePad state for further queries.
void GamePad::Handle(const SDL_Event &event)
{
	if(event.type == SDL_CONTROLLERDEVICEADDED)
	{
		// Needs to be called to receive axis and button events
		SDL_GameControllerOpen(event.cdevice.which);
		++activePads;
	}
	if(event.type == SDL_CONTROLLERDEVICEREMOVED)
	{
		--activePads;
	}
	if(event.type == SDL_CONTROLLERAXISMOTION)
	{
		double gradient = event.caxis.value / 32768.;
		axis[event.caxis.axis] = gradient;
	}
	if(event.type == SDL_CONTROLLERBUTTONDOWN)
	{
		lock_guard<mutex> lock(gamePadMutex);
		held[event.cbutton.button] = chrono::steady_clock::now();
		released.erase(event.cbutton.button);
	}
	if(event.type == SDL_CONTROLLERBUTTONUP)
	{
		lock_guard<mutex> lock(gamePadMutex);
		released[event.cbutton.button] = chrono::steady_clock::now();
		unreportedReleases.insert(event.cbutton.button);
		repeatTimer.erase(SDL_CONTROLLER_AXIS_MAX * 2 + event.cbutton.button);
	}
}



// Button counts as held if there's been a down event but no corresponding up event yet.
map<Uint8, chrono::milliseconds> GamePad::HeldButtons() const
{
	lock_guard<mutex> lock(gamePadMutex);
	chrono::time_point<std::chrono::steady_clock> now = chrono::steady_clock::now();
	std::map<Uint8, chrono::milliseconds> result;
	for(auto it = held.cbegin(); it != held.cend(); ++it)
	{
		if(released.count(it->first) == 0)
			result[it->first] = chrono::duration_cast<chrono::milliseconds>(now - it->second);
	}
	return result;
}



map<Uint8, std::chrono::time_point<std::chrono::steady_clock>> GamePad::HeldButtonsSince() const
{
	lock_guard<mutex> lock(gamePadMutex);
	map<Uint8, std::chrono::time_point<std::chrono::steady_clock>> result(held);
	for(auto it = result.begin(); it != result.cend();)
	{
		if(released.find(it->first) != released.cend())
			it = result.erase(it);
		else
			++it;
	}
	return result;
}



std::map<Uint8, chrono::milliseconds> GamePad::ReleasedButtons() const
{
	lock_guard<mutex> lock(gamePadMutex);
	std::map<Uint8, chrono::milliseconds> result;
	for(auto it = released.cbegin(); it != released.cend(); ++it)
	{
		if(unreportedReleases.find(it->first) != unreportedReleases.cend())
		{
			auto start = held.find(it->first);
			result[it->first] = start != held.cend() ?
				chrono::duration_cast<chrono::milliseconds>(it->second - start->second) :
					chrono::milliseconds::zero();
		}
	}
	return result;
}



bool GamePad::Held(Uint8 button) const
{
	lock_guard<mutex> lock(gamePadMutex);
	return held.find(button) != held.cend() && released.find(button) == released.cend();
}



void GamePad::Clear()
{
	lock_guard<mutex> lock(gamePadMutex);
	held.clear();
	released.clear();
	unreportedReleases.clear();
}



void GamePad::Clear(const set<Uint8> &toClear)
{
	lock_guard<mutex> lock(gamePadMutex);
	for(auto it = toClear.cbegin(); it != toClear.cend(); ++it)
	{
		held.erase(*it);
		released.erase(*it);
		unreportedReleases.erase(*it);
	}
}



void GamePad::Clear(Uint8 button)
{
	lock_guard<mutex> lock(gamePadMutex);
	held.erase(button);
	released.erase(button);
	unreportedReleases.erase(button);
}



bool GamePad::HavePads() const
{
	return activePads > 0;
}



std::set<Uint8> GamePad::ReadHeld(const std::set<Uint8> &interest)
{
	std::set<Uint8> result;
	lock_guard<mutex> lock(gamePadMutex);
	for(auto it = held.cbegin(); it != held.cend(); ++it)
		if(released.find(it->first) == released.cend() && interest.find(it->first) != interest.cend())
			result.insert(it->first);
	return result;
}



Point GamePad::LeftStick() const
{
	return Point(axis[SDL_CONTROLLER_AXIS_LEFTX], axis[SDL_CONTROLLER_AXIS_LEFTY]);
}



Point GamePad::RightStick() const
{
	return Point(axis[SDL_CONTROLLER_AXIS_RIGHTX], axis[SDL_CONTROLLER_AXIS_RIGHTY]);
}



double GamePad::LeftStickX() const
{
	return axis[SDL_CONTROLLER_AXIS_LEFTX];
}



double GamePad::LeftStickY() const
{
	return axis[SDL_CONTROLLER_AXIS_LEFTY];
}



double GamePad::RightStickX() const
{
	return axis[SDL_CONTROLLER_AXIS_RIGHTX];
}



double GamePad::RightStickY() const
{
	return axis[SDL_CONTROLLER_AXIS_RIGHTY];
}



double GamePad::LeftTrigger() const
{
	return axis[SDL_CONTROLLER_AXIS_TRIGGERLEFT];
}



double GamePad::RightTrigger() const
{
	return axis[SDL_CONTROLLER_AXIS_TRIGGERRIGHT];
}



bool GamePad::RepeatAxis(Uint8 which)
{
	double threshold;
	double gradient = axis[which];
	if(gradient > 0.5)
		threshold = 1500;
	else
		return false;
	return Repeat(which, threshold, which + SDL_CONTROLLER_AXIS_MAX);
}



bool GamePad::RepeatAxisNeg(Uint8 which)
{
	double threshold;
	double gradient = axis[which];
	if(gradient < -0.5)
		threshold = 1500;
	else
		return false;
	return Repeat(which + SDL_CONTROLLER_AXIS_MAX, threshold);
}



bool GamePad::RepeatButton(Uint8 button)
{
	if(held.find(button) == held.cend())
		return false;
	else if(released.find(button) != released.cend())
		return false;
	else
		return Repeat(button + SDL_CONTROLLER_AXIS_MAX * 2, 200.);
}



Command GamePad::ToCommand()
{
	Command command;
	std::map<Uint8, std::chrono::milliseconds> heldButtons = HeldButtons();
	std::map<Uint8, std::chrono::milliseconds> releasedButtons = ReleasedButtons();
	lock_guard<mutex> lock(gamePadMutex);
	// Overloaded buttons with short press commands
	for(auto it = releasedButtons.cbegin(); it != releasedButtons.cend(); ++it)
	{
		if(it->first == SDL_CONTROLLER_BUTTON_B
				&& it->second.count() < GamePad::LONG_PRESS_MILLISECONDS)
			command.Set(Command::LAND);
		else if(it->first == SDL_CONTROLLER_BUTTON_X
				&& it->second.count() < GamePad::LONG_PRESS_MILLISECONDS)
			command.Set(Command::CLOAK);
		else if(it->first == SDL_CONTROLLER_BUTTON_Y
				&& it->second.count() < GamePad::LONG_PRESS_MILLISECONDS)
		{
			command.Set(Command::BOARD);
		}
		else
			continue;
		held.erase(it->first);
		released.erase(it->first);
	}

	// Single click commands and held or long press triggered alternative commands
	for(auto it = heldButtons.cbegin(); it != heldButtons.cend(); ++it)
	{
		if(it->first == SDL_CONTROLLER_BUTTON_A)
			command.Set(Command::JUMP);
		else if(it->first == SDL_CONTROLLER_BUTTON_X
				&& it->second.count() >= GamePad::LONG_PRESS_MILLISECONDS)
			command.Set(Command::SCAN);
		else if(it->first == SDL_CONTROLLER_BUTTON_LEFTSHOULDER)
			command.Set(Command::SECONDARY);
		else if(it->first == SDL_CONTROLLER_BUTTON_RIGHTSHOULDER)
			command.Set(Command::PRIMARY);
		else if(it->first >= SDL_CONTROLLER_BUTTON_DPAD_UP
				&& it->first <= SDL_CONTROLLER_BUTTON_DPAD_RIGHT)
		{
			if(it->first == SDL_CONTROLLER_BUTTON_DPAD_UP)
				command.Set(Command::SELECT);
			else if(it->first == SDL_CONTROLLER_BUTTON_DPAD_DOWN)
				command.Set(Command::NEAREST);
			else if(it->first == SDL_CONTROLLER_BUTTON_DPAD_LEFT)
				command.Set(Command::TARGET);
			else if(it->first == SDL_CONTROLLER_BUTTON_DPAD_RIGHT)
				command.Set(Command::TARGET | Command::SHIFT);
			held.erase(it->first);
		}
		else if(it->first == SDL_CONTROLLER_BUTTON_RIGHTSTICK)
		{
			Command rightStickCommand = RightStickCommand();
			if(rightStickCommand && !rightStickCommand.Has(Command::AMMO))
			{
				command |= rightStickCommand;
				held.erase(it->first);
			}
		}
	}
	Point leftStick = LeftStick();
	if(leftStick.LengthSquared() > GamePad::VECTOR_TURN_THRESHOLD)
		command.SetTurnPoint(leftStick);
	if(axis[SDL_CONTROLLER_AXIS_TRIGGERRIGHT] > 0.1)
	{
		double thrust = axis[SDL_CONTROLLER_AXIS_TRIGGERRIGHT] * THRUST_MULTI;
		command.SetThrustGradient(thrust);
		if(thrust >= 1. && axis[SDL_CONTROLLER_AXIS_TRIGGERLEFT] > 0.5)
			command.Set(Command::AFTERBURNER);
	}
	else if(axis[SDL_CONTROLLER_AXIS_TRIGGERLEFT] > 0.1)
	{
		double thrust = -axis[SDL_CONTROLLER_AXIS_TRIGGERLEFT] * THRUST_MULTI;
		command.SetThrustGradient(thrust);
	}
	return command;
}



Command GamePad::ToPanelCommand()
{
	Command command;
	std::map<Uint8, std::chrono::milliseconds> heldButtons = HeldButtons();
	lock_guard<mutex> lock(gamePadMutex);
	for(std::map<Uint8, std::chrono::milliseconds>::const_iterator it = heldButtons.cbegin(); it != heldButtons.cend(); ++it)
	{
		if(it->first == SDL_CONTROLLER_BUTTON_B
			&& it->second.count() >= GamePad::LONG_PRESS_MILLISECONDS)
			command.Set(Command::SHIFT | Command::HAIL);
		else if(it->first == SDL_CONTROLLER_BUTTON_Y
				&& it->second.count() >= GamePad::LONG_PRESS_MILLISECONDS)
			command.Set(Command::HAIL);
		else if(it->first == SDL_CONTROLLER_BUTTON_BACK)
			command.Set(Command::INFO);
		else if(it->first == SDL_CONTROLLER_BUTTON_LEFTSTICK)
			command.Set(Command::MAP);
		else if(it->first == SDL_CONTROLLER_BUTTON_RIGHTSTICK && RightStickCommand().Has(Command::AMMO))
			command.Set(Command::AMMO);
		else
			continue;
		held.erase(it->first);
	}

	return command;
}



Command GamePad::RightStickCommand() const
{
	Command command;
	Point stick = RightStick();
	if(stick.LengthSquared() > GamePad::VECTOR_TURN_THRESHOLD)
	{
		double deg = std::atan2(stick.Y(), stick.X()) * TO_DEG;
		double degAbs = fabs(deg);
		if(degAbs >= 150.)
			command.Set(Command::DEPLOY);
		else if(degAbs <= 30.)
			command.Set(Command::BOARD | Command::SHIFT);
		else if(deg < 150. && deg >= 90.)
			command.Set(Command::HOLD);
		else if(deg < 90. && deg > 30.)
			command.Set(Command::GATHER);
		else if(deg < -30. && deg >= -90.)
			command.Set(Command::FIGHT);
		else
			command.Set(Command::AMMO);
	}
	return command;
}



bool GamePad::Repeat(Uint8 which, double threshold, Uint8 opposite)
{
	auto it = repeatTimer.find(which);
	chrono::time_point<std::chrono::steady_clock> now = chrono::steady_clock::now();
	if(it != repeatTimer.cend())
	{
		threshold *= Preferences::ScrollSpeed() / 60.;
		int diff = chrono::duration_cast<chrono::milliseconds>(now - it->second).count();
		if(diff > threshold)
			repeatTimer[which] = now;
		else
			return false;
	}
	else
		repeatTimer[which] = chrono::steady_clock::now();
	// Releasing a stick quickly may cause opposite spikes.
	if(opposite != 255)
	{
		chrono::time_point<std::chrono::steady_clock> adjust = chrono::steady_clock::now();
		now -= chrono::milliseconds(int(threshold) - 100);
		repeatTimer[opposite] = adjust;
	}
	return true;
}
