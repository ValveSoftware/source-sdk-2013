#ifndef LUA_LIB_H
#define LUA_LIB_H

#ifdef _WIN32
#pragma once
#endif

#include "lua.h"

LUA_FUNCTION(_assert);

//LUA_FUNCTION(_collectgarbage);

LUA_FUNCTION(_tostring);
//LUA_FUNCTION(_tonumber);

//LUA_FUNCTION(_load);
//LUA_FUNCTION(_loadfile);
//LUA_FUNCTION(_dofile);
//LUA_FUNCTION(_require);

LUA_FUNCTION(_print);
LUA_FUNCTION(_error);
LUA_FUNCTION(_type);

//LUA_FUNCTION(_ipairs);
//LUA_FUNCTION(_next);
//LUA_FUNCTION(_pairs);
//LUA_FUNCTION(_select);

//LUA_FUNCTION(_getmetatable);
//LUA_FUNCTION(_setmetatable);

//LUA_FUNCTION(_rawequal);
//LUA_FUNCTION(_rawget);
//LUA_FUNCTION(_rawlen);
//LUA_FUNCTION(_rawset);

//LUA_FUNCTION(_pcall);
//LUA_FUNCTION(_xpcall);

const luaL_Reg lib_base[] = {
	{ "print", _print },
	{ "tostring", _tostring },
	{ "error", _error },
	{ "type",  _type },
	{ NULL, NULL }
};
#endif