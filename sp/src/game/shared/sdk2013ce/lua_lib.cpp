#include "cbase.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#include "lua_lib.h"


int luaopen_overrides(lua_State* state)
{
	// Lower level stuff has to be
	// implemented in-engine (see lua_lib.h)
	// so first of all we add the base lib.
	// It gets to use the global namespace.
	lua_pushglobaltable(state);
	luaL_setfuncs(state, lib_base, 0);

	// set _G
	lua_pushvalue(state, -1);
	lua_setfield(state, -2, "_G");

	// set _VERSION
	lua_pushliteral(state, LUA_VERSION);
	lua_setfield(state, -2, "_VERSION");

	return 1;
}


LUA_FUNCTION(_print)
{
	LUA_ARGS(argc);

	for (int i = 1; i <= argc; i++) {
		ConColorMsg(COLOR_CYAN, luaL_tolstring(state, i, NULL));
	}
	Msg("\n");
	
	return 0;
}

LUA_FUNCTION(_error)
{
	// The error message is on top of the stack.
	// Fetch it, print it and then pop it off the stack.
	const char* message = luaL_tolstring(state, -1, NULL);

	//luaL_error(state, message);
	Warning("[Lua] Error: %s\n", message);

	lua_pop(state, 1);

	return 0;
}