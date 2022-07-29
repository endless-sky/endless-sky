/* Version.h
Copyright (c) 2022 by warp-core

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef VERSION_H_
#define VERSION_H_

namespace {
	const std::string GAME_VERSION = "0.9.15-alpha";
	const std::string COMMIT_ID = "";
}

class Version {
public:
	static std::string GetVersionString()
	{
		if(COMMIT_ID == "")
			return GAME_VERSION;
		return GAME_VERSION + " " + COMMIT_ID;
	}
};

#endif
