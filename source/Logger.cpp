/* Logger.cpp
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

#include "Logger.h"

#include "text/Format.h"
#include "GameVersion.h"

#ifdef _WIN32
#include "windows/WinVersion.h"
#else
#include <sys/utsname.h>
#endif

#include <iostream>
#include <mutex>

using namespace std;

namespace {
	function<void(const string &message, Logger::Level)> logCallback = nullptr;
	mutex logMutex;
}



Logger::Session::Session(bool quiet)
	: quiet{quiet}
{
	if(quiet)
		return;

	string message = "Logger session beginning. Game version: " + GameVersion::Running().ToString()
		+ ". Detected operating system version: ";
#ifdef _WIN32
	message += WinVersion::ToString() + '.';
#else
	utsname uName;
	uname(&uName);
	message += string(uName.sysname) + ' ' + uName.release + ' ' + uName.version + '.';
#endif
	Log(message, Level::INFO);
}



Logger::Session::~Session()
{
	if(quiet)
		return;

	Log("Logger session end.", Level::INFO);
}



void Logger::SetLogCallback(function<void(const string &message, Level)> callback)
{
	logCallback = std::move(callback);
}



void Logger::Log(const string &message, Level level)
{
	lock_guard<mutex> lock(logMutex);
	string formatted = Format::TimestampString(chrono::system_clock::now(), true)
		+ " | " + static_cast<char>(level) + " | " + message;
	(level == Level::INFO ? cout : cerr) << formatted << endl;
	// Perform additional logging through callback if any is registered.
	if(logCallback)
		logCallback(formatted, level);
}
