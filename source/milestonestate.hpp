/* milestonestate.hpp
Copyright (c) 2022 by warp-core

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef MILESTONE_STATE_HPP_
#define MILESTONE_STATE_HPP_

#include <cstdint>
#include <map>

enum class MilestoneState : int_fast8_t {
	DEFAULT,
	HIDDEN,
	LOCKED,
	UNLOCKED,
	COMPLETE,
};

MilestoneState MilestoneState::FromString(std::string input)
{
	static std::map<std::string, MilestoneState> stringToMilestoneState = {
		{"hidden", HIDDEN},
		{"locked", LOCKED},
		{"unlocked", UNLOCKED},
		{"completed", COMPLETE}
	};
	return stringToMilestoneState.find(input)->second;
}

#endif
