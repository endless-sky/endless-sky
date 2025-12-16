/* PrintData.h
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

#pragma once

class PlayerInfo;



// A class containing methods used to print ship and outfit stats to the console.
class PrintData {
public:
	static bool IsPrintDataArgument(const char *const *argv);
	static void Print(const char *const *argv, PlayerInfo &player);
	static void Help();
};
