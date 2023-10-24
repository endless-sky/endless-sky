#include "Lua.h"

#include <string>

class LuaPlugin
{
public:
	LuaPlugin();

	void runDaily();
	void runInit();

private:
	int daily = LUA_NOREF;
	int init = LUA_NOREF;
};
