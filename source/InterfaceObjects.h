/* InterfaceObjects.h
Copyright (c) 2022 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef INTERFACE_OBJECTS_H_
#define INTERFACE_OBJECTS_H_

#include "Color.h"
#include "Interface.h"
#include "Set.h"

#include <mutex>
#include <string>


class DataNode;
class Panel;


// This class contains the interface definitions for the game. Those should
// typically be loaded once and then used as long as the game is running.
class InterfaceObjects {
public:
	bool LoadNode(const DataNode &node);
	const Set<Color> &Colors();
	const Set<Interface> &Interfaces();

	const std::string &Tooltip(const std::string &label);
	std::string HelpMessage(const std::string &name);
	const std::map<std::string, std::string> &HelpTemplates();

	// Draws the current menu background. Unlike accessing the menu background
	// through GameData, this function is thread-safe.
	void DrawMenuBackground(Panel *panel) const;


private:
	Set<Color> colors;
	Set<Interface> interfaces;

	std::map<std::string, std::string> tooltips;
	std::map<std::string, std::string> helpMessages;
	
	// A local cache of the menu background interface for thread-safe access.
	mutable std::mutex menuBackgroundMutex;
	Interface menuBackgroundCache;
};

#endif
