/* Version.cpp
Copyright (c) 2022 by warp-core

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "Version.h"

using namespace std;

namespace {
	const string GAME_VERSION = "0.9.15-alpha";
	const string COMMIT_ID = "";
}



string Version::GetString()
{
	if(COMMIT_ID.empty())
		return GAME_VERSION;
	return GAME_VERSION + " " + COMMIT_ID;
}



const string &Version::GetVersion()
{
	return GAME_VERSION;
}



const string &Version::GetCommit()
{
	return COMMIT_ID;
}
