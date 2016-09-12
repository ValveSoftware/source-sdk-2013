#ifndef LUA_H
#define LUA_H

#ifdef _WIN32
#pragma once
#endif

#include "..\..\lua\lua.hpp"

class CLuaHandle
{
private:
	lua_State* state;
public:
	CLuaHandle(IFileSystem* pFilesystem);
	~CLuaHandle();

	// Load a file relative to the 'MOD' directory.
	void LoadFile(IFileSystem* pFilesystem, const char* filename);
	// Recursively load every .lua file in a folder.
	void LoadDir(IFileSystem* pFilesystem, const char* dirname);
};
#endif