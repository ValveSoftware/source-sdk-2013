#ifndef LUA_LIB_H
#define LUA_LIB_H

#ifdef _WIN32
#pragma once
#endif

#include "lua.h"

#define LUA_LIB_BEGIN()

int _print(lua_State* state);
int _error(lua_State* state);
int _type(lua_State* state);

const luaL_Reg lib_base[] = {
	{ "print", _print },
	{ "error", _error },
	{ "type",  _type },
	{ NULL, NULL }
};

#endif