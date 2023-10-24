/* LuaImpl.cpp
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

#include "LuaImpl.h"

#include "ConditionsStore.h"
#include "GameData.h"
#include "Logger.h"
#include "Lua.h"
#include "Messages.h"

#include <string>

using namespace std;

extern "C" int printMsg(lua_State *L)
{
	const char *message = luaL_checkstring(L, 1);
	int priority = (lua_isinteger(Lua::get(), 2)) ? luaL_checkinteger(L, 2) : 3;
	if(priority <= static_cast<uint_fast8_t>(Messages::Importance::Low) && priority >= 0)
		Messages::Add(message, static_cast<Messages::Importance>(priority));
	else
		Logger::LogError("Lua Message Add Importance was invalid: " + to_string(priority));
	return 0;
}

extern "C" int debug(lua_State *L)
{
	const char *message = luaL_checkstring(L, 1);
	Logger::LogError(string("[Lua]: ") + message);
	return 0;
}

void LuaImpl::registerFunction(lua_CFunction func, const char *name)
{
	lua_register(Lua::get(), name, func);
}

void LuaImpl::registerAll()
{
	registerFunction(printMsg, "es_addMsg");
	registerFunction(debug, "es_debug");
}
