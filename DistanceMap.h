/* DistanceMap.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef DISTANCE_MAP_H_
#define DISTANCE_MAP_H_

#include <map>

class System;
class PlayerInfo;



// This is a map of how many hyperspace jumps it takes to get to other systems
// from the given "center" system.
class DistanceMap {
public:
	// If a player is given, the map will only use hyperspace paths known to the
	// player; that is, one end of the path has been visited. Also, if the
	// player's flagship has a jump drive, the jumps will be make use of it.
	DistanceMap(const System *center);
	DistanceMap(const PlayerInfo &player);
	
	// Find out if the given system is reachable.
	bool HasRoute(const System *system) const;
	// Find out how many jumps away the given system is.
	int Distance(const System *system) const;
	// If I am in the given system, going to the player's system, what system
	// should I jump to next?
	const System *Route(const System *system) const;
	
	
private:
	void Init();
	void InitHyper(const PlayerInfo &player);
	void InitJump(const PlayerInfo &player);
	
	
private:
	std::map<const System *, int> distance;
	std::map<const System *, const System *> route;
	bool hasJump = false;
};



#endif
