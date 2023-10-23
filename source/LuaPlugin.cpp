#include "LuaPlugin.h"

#include "Logger.h"

using namespace std;

namespace {
	int lua_fn_field_ref(lua_State *L, const char *fieldname) {
		const auto field_ty = lua_getfield(L, 1, fieldname);
		if (field_ty == LUA_TFUNCTION) {
			return luaL_ref(L, LUA_REGISTRYINDEX);
		} else if (field_ty != LUA_TNIL) {
			Logger::LogError(string("Expected lua fn in field ") + fieldname + ", got: " + lua_typename(L, field_ty));
		}
		return LUA_NOREF;
	}
}

LuaPlugin::LuaPlugin() {
	auto L = Lua::get();

	this->daily = lua_fn_field_ref(L, "es_daily");
	this->init = lua_fn_field_ref(L, "es_init");
}

void LuaPlugin::runDaily() {
	if(daily != LUA_NOREF) {
		lua_rawgeti(Lua::get(), LUA_REGISTRYINDEX, daily);
		lua_call(Lua::get(), 0, 0);
	}
}

void LuaPlugin::runInit() {
	if(init != LUA_NOREF) {
		lua_rawgeti(Lua::get(), LUA_REGISTRYINDEX, init);
		lua_call(Lua::get(), 0, 0);
	}
}
