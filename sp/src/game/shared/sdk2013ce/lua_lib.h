#ifndef LUA_LIB_H
#define LUA_LIB_H

#ifdef _WIN32
#pragma once
#endif

#include "lua.h"

// TODO: load, loadfile, dofile

LUA_FUNCTION(_print);
LUA_FUNCTION(_error);

const luaL_Reg lib_base[] = {
	{ "print", _print },
	{ "error", _error },
	{ NULL, NULL }
};

int luaopen_overrides(lua_State* state);

#endif