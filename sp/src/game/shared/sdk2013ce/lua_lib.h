#ifndef LUA_LIB_H
#define LUA_LIB_H

#ifdef _WIN32
#pragma once
#endif

#include "scriptmanager.h"

// TODO: load, loadfile, dofile

ScriptVariable_t* _print(ScriptVariable_t* args, int argc);
ScriptVariable_t* _error(ScriptVariable_t* args, int argc);

ScriptVariable_t* __print(ScriptVariable_t* args, int argc);

int luaopen_overrides(lua_State* state);

#endif