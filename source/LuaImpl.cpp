#include "LuaImpl.h"

#include "ConditionsStore.h"
#include "GameData.h"
#include "Messages.h"
#include "Logger.h"
#include "Lua.h"

#include <string>

using namespace std;

extern "C" int printMsg(lua_State *L)
{
	const char *message = luaL_checkstring(L, 1);
	int priority;
	if (lua_isinteger(Lua::get(), 2))
		priority = luaL_checkinteger(L, 2);
	else
		priority = 3;
	if (priority <= static_cast<uint_fast8_t>(Messages::Importance::Low) && priority >= 0)
	{
		Messages::Add(message, static_cast<Messages::Importance>(priority));
	}
	else
	{
		Logger::LogError("Lua Message Add Importance was invalid: " + to_string(priority));
	}
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
