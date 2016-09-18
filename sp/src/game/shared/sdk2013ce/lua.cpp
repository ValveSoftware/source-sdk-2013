#include "cbase.h"
#include "tier0/dbg.h"
#include "Filesystem.h"
#include "string.h"
#include "../../lua/lua.hpp"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#include "lua.h"
#include "lua_lib.h"

lua_State* CLuaManager::state = NULL;

// Wrap calls to lua_pcall etc in this.
#define LUA_CATCH(x) if ((x) != LUA_OK) {_error(state);}


void CLuaManager::loadFile(IFileSystem* filesystem, const char* path)
{
	DevMsg("[Lua] Loading %s\n", path);

	FileHandle_t f = filesystem->Open(path, "rb", "MOD");
	if (!f) {
		Warning("[Lua] Failed to load %s\n", path);
		return;
	}

	// load file into a null-terminated buffer
	int fileSize = filesystem->Size(f);
	unsigned bufSize = ((IFileSystem *)filesystem)->GetOptimalReadSize(f, fileSize + 1);

	char *buffer = (char*)((IFileSystem *)filesystem)->AllocOptimalReadBuffer(f, bufSize);
	//Assert(buffer);

	((IFileSystem *)filesystem)->ReadEx(buffer, bufSize, fileSize, f);
	buffer[fileSize] = '\0'; // null terminate file as EOF
	filesystem->Close(f);

	// Load the file
	LUA_CATCH (
		luaL_loadbuffer(state, buffer, fileSize, path)
	)

	// TODO: Don't do this here.
	LUA_CATCH (
		lua_pcall(state, 0, LUA_MULTRET, 0) 
	)
}

void CLuaManager::loadDir(IFileSystem* filesystem, const char* dir)
{
	char searchPath[MAX_PATH];
	V_snprintf(searchPath, MAX_PATH, "%s/*", dir);
	V_FixDoubleSlashes(searchPath);
	V_FixSlashes(searchPath);

	DevMsg("[Lua] Loading directory %s\n", searchPath);

	FileFindHandle_t finder;
	const char* pFileName = filesystem->FindFirstEx(searchPath, "MOD", &finder);

	while (pFileName)
	{
		//		   Sample:
		// pFileName:	test.lua
		// path:		lua/test.lua
		char path[MAX_PATH];
		V_snprintf(path, MAX_PATH, "%s/%s", dir, pFileName);
		V_FixSlashes(path);

		if (filesystem->IsDirectory(path, "MOD")) {
			if (pFileName[0] != '.') {
				loadDir(filesystem, path);
			}
		} else {
			loadFile(filesystem, path);
		}

		pFileName = filesystem->FindNext(finder);
	}

	filesystem->FindClose(finder);
}


void CLuaManager::doString(const char* str)
{
	DevMsg("[Lua] Running String: %s\n", str);

	LUA_CATCH (
		luaL_dostring(state, str)
	)
}

void CLuaManager::call(const char* hook)
{
	lua_getglobal(state, hook);
	LUA_CATCH(lua_pcall(state, 0, 0, 0));
}


void CLuaManager::init(IFileSystem* filesystem)
{
	Msg("[Lua] Initializing\n");

	state = luaL_newstate();

	lua_atpanic(state, _error);

	// Load Libraries

	// The base lib gets special treatment,
	// its functions are global.
	lua_pushglobaltable(state);
	luaL_setfuncs(state, lib_base, 0);

	// set _G
	lua_pushvalue(state, -1);
	lua_setfield(state, -2, "_G");

	// set _VERSION
	lua_pushliteral(state, LUA_VERSION);
	lua_setfield(state, -2, "_VERSION");

	loadDir(filesystem, "lua/init");
}

void CLuaManager::close()
{
	Msg("[Lua] Shutting down.\n");
	lua_close(state);
}