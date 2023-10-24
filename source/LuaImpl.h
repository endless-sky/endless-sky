#ifndef ES_LUA_IMPL_H_
#define ES_LUA_IMPL_H_

#include "Lua.h"

namespace LuaImpl
{
	void registerAll();

	void registerFunction(lua_CFunction func, const char *name);
}

#endif
