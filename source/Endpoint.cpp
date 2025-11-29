/* Endpoint.cpp
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

#include "Endpoint.h"

#include "DataWriter.h"

#include <map>

using namespace std;

namespace {
	// Lookup table for matching special tokens to enumeration values.
	map<string, int> TOKEN_INDEX = {
		{"accept", Endpoint::ACCEPT},
		{"decline", Endpoint::DECLINE},
		{"defer", Endpoint::DEFER},
		{"launch", Endpoint::LAUNCH},
		{"flee", Endpoint::FLEE},
		{"depart", Endpoint::DEPART},
		{"die", Endpoint::DIE},
		{"explode", Endpoint::EXPLODE}
	};
}

const int Endpoint::ACCEPT;
const int Endpoint::DECLINE;
const int Endpoint::DEFER;
const int Endpoint::LAUNCH;
const int Endpoint::FLEE;
const int Endpoint::DEPART;
const int Endpoint::DIE;
const int Endpoint::EXPLODE;



bool Endpoint::RequiresLaunch(int outcome)
{
	return outcome == LAUNCH || outcome == FLEE || outcome == DEPART;
}



int Endpoint::TokenIndex(const string &token)
{
	auto it = TOKEN_INDEX.find(token);
	return (it == TOKEN_INDEX.end() ? 0 : it->second);
}



string Endpoint::TokenName(int index)
{
	for(const auto &it : TOKEN_INDEX)
		if(it.second == index)
			return it.first;

	return to_string(index);
}



void Endpoint::WriteToken(int index, DataWriter &out)
{
	out.BeginChild();
	{
		if(index >= 0)
			out.Write("goto", index);
		else
			out.Write(TokenName(index));
	}
	out.EndChild();
}
