
//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef LUAMANAGER_H
#define LUAMANAGER_H
#ifdef _WIN32
#pragma once
#endif

#include "lua.hpp"

#define LUA_ROOT					"lua" // Can't be "LUA_PATH" because luaconf.h uses it.
#define LUA_PATH_CACHE				"lua_cache"
#define LUA_PATH_ADDONS				"addons"
#define LUA_PATH_ENUM				LUA_ROOT "\\includes\\enum"
#define LUA_PATH_EXTENSIONS			LUA_ROOT "\\includes\\extensions"
#define LUA_PATH_MODULES			LUA_ROOT "\\includes\\modules"
#define LUA_PATH_INCLUDES			LUA_ROOT "\\includes"
#define LUA_PATH_GAME_CLIENT		LUA_ROOT "\\game\\client"
#define LUA_PATH_GAME_SERVER		LUA_ROOT "\\game\\server"
#define LUA_PATH_GAME_SHARED		LUA_ROOT "\\game\\shared"
#define LUA_PATH_EFFECTS			LUA_ROOT "\\effects"
#define LUA_PATH_ENTITIES			LUA_ROOT "\\entities"
#define LUA_PATH_GAMEUI				LUA_ROOT "\\gameui"
#define LUA_PATH_WEAPONS			LUA_ROOT "\\weapons"


#define LUA_BASE_ENTITY_CLASS		"prop_scripted"
#define LUA_BASE_ENTITY_FACTORY		"CBaseAnimating"
#define LUA_BASE_WEAPON				"weapon_hl2mpbase_scriptedweapon"
#define LUA_BASE_GAMEMODE			"deathmatch"


#define LUA_MAX_WEAPON_ACTIVITIES	32


#pragma warning( disable: 4800 )	// forcing value to bool 'true' or 'false' (performance warning)

#define BEGIN_LUA_SET_ENUM_LIB(L, libraryName) \
  const char *lib = libraryName; \
  lua_getglobal(L, "_E"); \
  lua_newtable(L);

#define lua_pushenum(L, enum, shortname) \
  lua_pushinteger(L, enum); \
  lua_setfield(L, -2, shortname);

#define END_LUA_SET_ENUM_LIB(L) \
  lua_setfield(L, -2, lib); \
  lua_pop(L, 1);

#define BEGIN_LUA_CALL_HOOK(functionName) \
  lua_getglobal(L, "hook"); \
  if (lua_istable(L, -1)) { \
    lua_getfield(L, -1, "call"); \
	if (lua_isfunction(L, -1)) { \
	  lua_remove(L, -2); \
	  int args = 0; \
	  lua_pushstring(L, functionName); \
	  lua_getglobal(L, "_GAMEMODE"); \
	  args = 2;

#define END_LUA_CALL_HOOK(nArgs, nresults) \
	  args += nArgs; \
	  luasrc_pcall(L, args, nresults, 0); \
	} \
	else \
	  lua_pop(L, 2); \
  } \
  else \
    lua_pop(L, 1);

#define BEGIN_LUA_CALL_WEAPON_METHOD(functionName) \
  lua_getref(L, m_nTableReference); \
  lua_getfield(L, -1, functionName); \
  lua_remove(L, -2); \
  if (lua_isfunction(L, -1)) { \
    int args = 0; \
	lua_pushweapon(L, this); \
	++args;

#define END_LUA_CALL_WEAPON_METHOD(nArgs, nresults) \
	args += nArgs; \
	luasrc_pcall(L, args, nresults, 0); \
  } \
  else \
    lua_pop(L, 1);

#define BEGIN_LUA_CALL_WEAPON_HOOK(functionName, pWeapon) \
  if (pWeapon->IsScripted()) { \
    lua_getref(L, pWeapon->m_nTableReference); \
    lua_getfield(L, -1, functionName); \
    lua_remove(L, -2); \
    int args = 0; \
    lua_pushweapon(L, pWeapon); \
    ++args;

#define END_LUA_CALL_WEAPON_HOOK(nArgs, nresults) \
    args += nArgs; \
    luasrc_pcall(L, args, nresults, 0); \
  }

#define BEGIN_LUA_CALL_ENTITY_METHOD(functionName) \
  lua_getref(L, m_nTableReference); \
  lua_getfield(L, -1, functionName); \
  lua_remove(L, -2); \
  if (lua_isfunction(L, -1)) { \
    int args = 0; \
	lua_pushanimating(L, this); \
	++args;

#define END_LUA_CALL_ENTITY_METHOD(nArgs, nresults) \
	args += nArgs; \
	luasrc_pcall(L, args, nresults, 0); \
  } \
  else \
    lua_pop(L, 1);

#define BEGIN_LUA_CALL_TRIGGER_METHOD(functionName) \
  lua_getref(L, m_nTableReference); \
  lua_getfield(L, -1, functionName); \
  lua_remove(L, -2); \
  if (lua_isfunction(L, -1)) { \
    int args = 0; \
	lua_pushentity(L, this); \
	++args;

#define END_LUA_CALL_TRIGGER_METHOD(nArgs, nresults) \
	args += nArgs; \
	luasrc_pcall(L, args, nresults, 0); \
  } \
  else \
    lua_pop(L, 1);

#define BEGIN_LUA_CALL_PANEL_METHOD(functionName) \
  if (m_nTableReference >= 0) { \
    lua_getref(m_lua_State, m_nTableReference); \
    lua_getfield(m_lua_State, -1, functionName); \
    lua_remove(m_lua_State, -2); \
    if (lua_isfunction(m_lua_State, -1)) { \
      int args = 0; \
	  lua_pushpanel(m_lua_State, this); \
	  ++args;

#define END_LUA_CALL_PANEL_METHOD(nArgs, nresults) \
	  args += nArgs; \
	  luasrc_pcall(m_lua_State, args, nresults, 0); \
    } \
    else \
      lua_pop(m_lua_State, 1); \
  }

#define RETURN_LUA_NONE() \
  if (lua_gettop(L) == 1) { \
    if (lua_isboolean(L, -1)) { \
	  bool res = (bool)luaL_checkboolean(L, -1); \
	  lua_pop(L, 1); \
	  if (!res) \
	    return; \
	} \
    else \
	  lua_pop(L, 1); \
  }

#define RETURN_LUA_PANEL_NONE() \
  if (lua_gettop(m_lua_State) == 1) { \
    if (lua_isboolean(m_lua_State, -1)) { \
	  bool res = (bool)luaL_checkboolean(m_lua_State, -1); \
	  lua_pop(m_lua_State, 1); \
	  if (!res) \
	    return; \
	} \
    else \
	  lua_pop(m_lua_State, 1); \
  }

#define RETURN_LUA_BOOLEAN() \
  if (lua_gettop(L) == 1) { \
    if (lua_isboolean(L, -1)) { \
	  bool res = (bool)luaL_checkboolean(L, -1); \
	  lua_pop(L, 1); \
	  return res; \
	} \
    else \
	  lua_pop(L, 1); \
  }

#define RETURN_LUA_PANEL_BOOLEAN() \
  if (lua_gettop(m_lua_State) == 1) { \
    if (lua_isboolean(m_lua_State, -1)) { \
	  bool res = (bool)luaL_checkboolean(m_lua_State, -1); \
	  lua_pop(m_lua_State, 1); \
	  return res; \
	} \
    else \
	  lua_pop(m_lua_State, 1); \
  }

#define RETURN_LUA_NUMBER() \
  if (lua_gettop(L) == 1) { \
    if (lua_isnumber(L, -1)) { \
	  float res = luaL_checknumber(L, -1); \
	  lua_pop(L, 1); \
	  return res; \
	} \
    else \
	  lua_pop(L, 1); \
  }

#define RETURN_LUA_INTEGER() \
  if (lua_gettop(L) == 1) { \
    if (lua_isnumber(L, -1)) { \
	  int res = luaL_checkint(L, -1); \
	  lua_pop(L, 1); \
	  return res; \
	} \
    else \
	  lua_pop(L, 1); \
  }

#define RETURN_LUA_ACTIVITY() \
  if (lua_gettop(L) == 1) { \
    if (lua_isnumber(L, -1)) { \
	  int res = luaL_checkint(L, -1); \
	  lua_pop(L, 1); \
	  return (Activity)res; \
	} \
    else \
	  lua_pop(L, 1); \
  }

#define RETURN_LUA_STRING() \
  if (lua_gettop(L) == 1) { \
    if (lua_isstring(L, -1)) { \
	  const char *res = luaL_checkstring(L, -1); \
	  lua_pop(L, 1); \
	  return res; \
	} \
    else \
	  lua_pop(L, 1); \
  }

#define RETURN_LUA_WEAPON() \
  if (lua_gettop(L) == 1) { \
    if (lua_isuserdata(L, -1) && luaL_checkudata(L, -1, "CBaseCombatWeapon")) { \
	  CBaseCombatWeapon *res = luaL_checkweapon(L, -1); \
	  lua_pop(L, 1); \
	  return res; \
	} \
    else \
	  lua_pop(L, 1); \
  }

#define RETURN_LUA_ENTITY() \
  if (lua_gettop(L) == 1) { \
    if (lua_isuserdata(L, -1) && luaL_checkudata(L, -1, "CBaseEntity")) { \
	  CBaseEntity *res = luaL_checkentity(L, -1); \
	  lua_pop(L, 1); \
	  return res; \
	} \
    else \
	  lua_pop(L, 1); \
  }

#define RETURN_LUA_PLAYER() \
  if (lua_gettop(L) == 1) { \
    if (lua_isuserdata(L, -1) && luaL_checkudata(L, -1, "CBasePlayer")) { \
	  CBasePlayer *res = luaL_checkplayer(L, -1); \
	  lua_pop(L, 1); \
	  return res; \
	} \
    else \
	  lua_pop(L, 1); \
  }

#define RETURN_LUA_VECTOR() \
  if (lua_gettop(L) == 1) { \
    if (lua_isuserdata(L, -1) && luaL_checkudata(L, -1, "Vector")) { \
	  Vector res = luaL_checkvector(L, -1); \
	  lua_pop(L, 1); \
	  return res; \
	} \
    else \
	  lua_pop(L, 1); \
  }

#define RETURN_LUA_ANGLE() \
  if (lua_gettop(L) == 1) { \
    if (lua_isuserdata(L, -1) && luaL_checkudata(L, -1, "QAngle")) { \
	  QAngle res = luaL_checkangle(L, -1); \
	  lua_pop(L, 1); \
	  return res; \
	} \
    else \
	  lua_pop(L, 1); \
  }

extern ConVar gamemode;

LUALIB_API int luaL_checkboolean (lua_State *L, int narg);
LUALIB_API int luaL_optboolean (lua_State *L, int narg,
                                              int def);

#ifdef CLIENT_DLL
extern lua_State *LGameUI; // gameui state
#endif

extern lua_State *L;


// Set to true between LevelInit and LevelShutdown.
extern bool	g_bLuaInitialized;

#ifdef CLIENT_DLL
void       luasrc_init_gameui (void);
void       luasrc_shutdown_gameui (void);
#endif

void       luasrc_init (void);
void       luasrc_shutdown (void);

LUA_API int   (luasrc_dostring) (lua_State *L, const char *string);
LUA_API int   (luasrc_dofile) (lua_State *L, const char *filename);
LUA_API void  (luasrc_dofolder) (lua_State *L, const char *path);

LUA_API int   (luasrc_pcall) (lua_State *L, int nargs, int nresults, int errfunc);
LUA_API void  (luasrc_print) (lua_State *L, int narg);
LUA_API void  (luasrc_dumpstack) (lua_State *L);

// void    luasrc_LoadEffects (const char *path = 0);
void       luasrc_LoadEntities (const char *path = 0);
void       luasrc_LoadWeapons (const char *path = 0);

bool       luasrc_LoadGamemode (const char *gamemode);
bool       luasrc_SetGamemode (const char *gamemode);

#endif // LUAMANAGER_H
