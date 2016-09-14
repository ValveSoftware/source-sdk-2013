#ifndef LUA_LIB_H
#define LUA_LIB_H

#ifdef _WIN32
#pragma once
#endif

#include "lua.h"

LUA_FUNCTION(_assert);
LUA_FUNCTION(_tostring);
LUA_FUNCTION(_print);
LUA_FUNCTION(_error);
LUA_FUNCTION(_type);

const luaL_Reg lib_base[] = {
	{ "print", _print },
	{ "tostring", _tostring },
	{ "error", _error },
	{ "type",  _type },
	{ NULL, NULL }
};

#endif