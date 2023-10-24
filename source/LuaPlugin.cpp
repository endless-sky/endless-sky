/* LuaPlugin.cpp
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

#include "LuaPlugin.h"

#include "Logger.h"

using namespace std;

namespace {
	int lua_fn_field_ref(lua_State *L, const char *fieldname)
	{
		const auto field_ty = lua_getfield(L, 1, fieldname);
		if(field_ty == LUA_TFUNCTION)
			return luaL_ref(L, LUA_REGISTRYINDEX);
		else if(field_ty != LUA_TNIL)
			Logger::LogError(string("Expected lua fn in field ") + fieldname + ", got: " + lua_typename(L, field_ty));
		return LUA_NOREF;
	}

	void runRawChecked(lua_State *L, int raw) {
		if(raw != LUA_NOREF)
		{
			lua_rawgeti(Lua::get(), LUA_REGISTRYINDEX, raw);
			lua_call(Lua::get(), 0, 0);
		}
	}
}

LuaPlugin::LuaPlugin()
{
	auto L = Lua::get();

	this->daily = lua_fn_field_ref(L, "es_daily");
	this->init = lua_fn_field_ref(L, "es_init");
}

void LuaPlugin::runDaily()
{
	runRawChecked(Lua::get(), daily);
}

void LuaPlugin::runInit()
{
	runRawChecked(Lua::get(), init);
}
