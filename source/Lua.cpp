/* Lua.cpp
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

#include "Lua.h"

#include "ConditionsStore.h"
#include "Files.h"
#include "GameData.h"
#include "Logger.h"
#include "LuaImpl.h"
#include "LuaPlugin.h"

#include <iostream>
#include <vector>

using namespace std;

namespace {
	lua_State *L;

	vector<LuaPlugin> plugins;
}

void Lua::dumpstack()
{
	printf("Dumping Stack:\n");
	int top = lua_gettop(L);
	for(int i = 1; i <= top; i++)
	{
		printf("%d\t%s\t", i, luaL_typename(L, i));
		switch(lua_type(L, i))
		{
		case LUA_TNUMBER:
			printf("%g\n", lua_tonumber(L, i));
			break;
		case LUA_TSTRING:
			printf("%s\n", lua_tostring(L, i));
			break;
		case LUA_TBOOLEAN:
			printf("%s\n", (lua_toboolean(L, i) ? "true" : "false"));
			break;
		case LUA_TNIL:
			printf("%s\n", "nil");
			break;
		default:
			printf("%p\n", lua_topointer(L, i));
			break;
		}
	}
}

lua_State *Lua::get()
{
	return L;
}

bool Lua::init()
{
	L = luaL_newstate();
	luaL_openlibs(L);

	LuaImpl::registerAll();

	return true;
}

void Lua::close()
{
	lua_close(L);
}

bool Lua::loadSource(const string &path)
{
	const auto truePath = Files::Data() + path;
	const auto loaded = Files::Read(truePath);
	luaL_loadstring(L, loaded.c_str());
	lua_call(L, 0, 1);

	plugins.emplace_back();

	return true;
}

void Lua::runDailyScripts()
{
	for(auto &plugin : plugins)
	{
		plugin.runDaily();
	}
}

void Lua::runInitScripts()
{
	for(auto &plugin : plugins)
	{
		plugin.runInit();
	}
}
