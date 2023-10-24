/* Lua.h
Copyright (c) 2023 by Daniel Yoon

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef ES_LUA_H_
#define ES_LUA_H_

#include <bitset>
#include <optional>
#include <string>

extern "C"
{
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

namespace Lua
{
	bool init();
	void close();

	bool loadSource(const std::string &path);
	void runDailyScripts();
	void runInitScripts();

	lua_State *get();
	void dumpstack();
}

#endif
