#include "cbase.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#include "lua_lib.h"

LUA_FUNCTION(_assert)
{
	LUA_ARGS_MIN(argc, 1);

	int v = lua_toboolean(state, 1);
	const char* message;
	if (argc > 1)
		message = lua_tostring(state, 1);
	else
		message = "assertion failed!";

	if (!v) {
		luaL_error(state, message);
	}

	lua_pushboolean(state, v);

	return 1;
}

LUA_FUNCTION(_tostring)
{
	LUA_ARGS_MIN(argc, 1);

	lua_pushstring(state, luaL_tolstring(state, 1, NULL));

	return 1;
}

LUA_FUNCTION(_print)
{
	LUA_ARGS(argc);

	for (int i = 1; i <= argc; i++) {
		Msg(luaL_tolstring(state, i, NULL));
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

LUA_FUNCTION(_type)
{
	LUA_ARGS_MIN(argc, 1);

	lua_pushstring(state, luaL_typename(state, 1));

	return 1;
}