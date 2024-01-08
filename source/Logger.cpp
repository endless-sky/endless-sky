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

#include <iostream>
#include <mutex>

using namespace std;

namespace {
	function<void(const string &message)> logErrorCallback = nullptr;
	mutex logErrorMutex;
}



void Logger::SetLogErrorCallback(function<void(const string &message)> callback)
{
	logErrorCallback = std::move(callback);
}



void Logger::LogError(const string &message)
{
	lock_guard<mutex> lock(logErrorMutex);
	// Log by default to stderr.
	cerr << message << endl;
	// Perform additional logging through callback if any is registered.
	if(logErrorCallback)
		logErrorCallback(message);
}
