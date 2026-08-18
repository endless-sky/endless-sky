/* Logger.h
Copyright (c) 2022 by Peter van der Meer

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

#include <functional>
#include <string>



// Default static logging facility, different programs might have different
// conventions and requirements on how they handle logging, so the running
// program should register its preferred logging facility when starting up.
class Logger {
public:
	enum class Level : char {
		// Messages that don't indicate any failure, for example standard
		// logger session information.
		INFO = 'I',
		// Messages about problems that might affect the game in a non-critical way,
		// for example data errors.
		WARNING = 'W',
		// Messages about most important problems, including game startup errors.
		ERROR = 'E'
	};

	// Print additional control messages when a session begins or ends.
	class Session {
	public:
		Session(bool quiet);
		~Session();


	private:
		bool quiet;
	};


public:
	static void SetLogCallback(std::function<void(const std::string &message, Level)> callback);
	static void Log(const std::string &message, Level level);
};
