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



/// Default static logging facility, different programs might have different
/// conventions and requirements on how they handle logging, so the running
/// program should register its preferred logging facility when starting up.
class Logger {
public:
	static void SetLogErrorCallback(std::function<void(const std::string &message)> callback);
	static void LogError(const std::string &message);
};
