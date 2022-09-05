/* Version.h
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

#ifndef VERSION_H_
#define VERSION_H_

#include <string>

// A class to get a string with the game version and
// the latest commit hash for non-release version.
class Version {
public:
	static std::string GetString();
	static const std::string &GetVersion();
	static const std::string &GetCommit();
};

#endif
