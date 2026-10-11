/* Endpoint.h
Copyright (c) 2025 by Amazinite

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include <string>

class DataWriter;



// Actions that a Conversation or Dialog can end with.
class Endpoint {
public:
	static const int ACCEPT = -1;
	static const int DECLINE = -2;
	static const int DEFER = -3;
	// These 3 options force the player to TakeOff (if landed), or cause
	// the boarded NPCs to explode, in addition to respectively duplicating
	// the above mission outcomes.
	static const int LAUNCH = -4;
	static const int FLEE = -5;
	static const int DEPART = -6;
	// The player may simply die (if landed on a planet or captured while
	// in space), or the flagship might also explode.
	static const int DIE = -7;
	static const int EXPLODE = -8;

	// Check whether the given outcome is one that forces the
	// player to immediately depart.
	static bool RequiresLaunch(int outcome);

	// Get the index of the given special string. 0 means it is "goto", a number
	// less than 0 means it is an outcome, and 1 means no match.
	static int TokenIndex(const std::string &token);

	// Map an index back to a string, for saving the conversation to a file.
	static std::string TokenName(int index);

	// Write a "goto" or endpoint.
	static void WriteToken(int index, DataWriter &out);
};
