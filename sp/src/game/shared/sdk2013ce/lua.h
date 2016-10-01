#ifndef LUA_H
#define LUA_H

#ifdef _WIN32
#pragma once
#endif

#include "..\..\lua\lua.hpp"
#include "IScriptingLanguage.h"

// Deprecated
//#define LUA_FUNCTION(name)	int name(lua_State* state)

//#define LUA_ARGS(argc)		int argc = lua_gettop(state)
//#define LUA_ARGS_MIN(argc, minArgs)		\
//	int argc = lua_gettop(state);		\
//	luaL_argcheck(state, argc >= minArgs, 1, "")

// TODO: Should extend IScriptingLanguage
class CLuaManager : public IScriptingLanguage
{
private:
	lua_State* state;

	void AddLibs();
	
	CUtlVector<luaL_Reg> functionBinds;
	CUtlVector<const char*> hooks;
public:
	
	CLuaManager(IFileSystem* filesystem) { Init(filesystem); };

	inline lua_State* GetState() {
		return state;
	}

	void Init(IFileSystem* filesystem);
	void Close();

	// Load a file relative to the 'MOD' directory.
	void LoadFile(IFileSystem* pFilesystem, const char* filename);
	// Recursively load every .lua file in a folder.
	void LoadDir(IFileSystem* pFilesystem, const char* dirname);

	void BindFunction(BindFunction_t func, const char* funcName);
	CUtlVector<const char*>* GetBinds();
	CUtlVector<const char*>* GetHooks();

	// Executes str.
	void DoString(const char* str);
	void DoFile(IFileSystem* pFilesystem, const char* filename);

	// Calls hook hookName.
	void Call(const char* hookName);
	void AddHook(const char* hookName) { hooks.AddToTail(hookName); };
};

#endif