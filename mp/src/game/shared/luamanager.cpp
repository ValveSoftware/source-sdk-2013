//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: Contains the implementation of Lua for scripting.
//
//===========================================================================//

#include "cbase.h"
#include "filesystem.h"
#ifndef CLIENT_DLL
#include "gameinterface.h"
#endif
#include "steam/isteamfriends.h"
#include "networkstringtabledefs.h"
#ifndef CLIENT_DLL
#include "basescriptedtrigger.h"
#endif
#include "basescripted.h"
#include "weapon_hl2mpbase_scriptedweapon.h"
#include "luamanager.h"
#include "luasrclib.h"
#include "luacachefile.h"
#include "lconvar.h"
#include "licvar.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar gamemode( "gamemode", "sandbox", FCVAR_ARCHIVE | FCVAR_REPLICATED );
static char contentSearchPath[MAX_PATH];

static void tag_error (lua_State *L, int narg, int tag) {
  luaL_typerror(L, narg, lua_typename(L, tag));
}


LUALIB_API int luaL_checkboolean (lua_State *L, int narg) {
  int d = lua_toboolean(L, narg);
  if (d == 0 && !lua_isboolean(L, narg))  /* avoid extra test when d is not 0 */
    tag_error(L, narg, LUA_TBOOLEAN);
  return d;
}


LUALIB_API int luaL_optboolean (lua_State *L, int narg,
                                              int def) {
  return luaL_opt(L, luaL_checkboolean, narg, def);
}


#ifdef CLIENT_DLL
lua_State *LGameUI;
#endif

lua_State *L;

// Lua system
bool g_bLuaInitialized;

static int luasrc_print (lua_State *L) {
  int n = lua_gettop(L);  /* number of arguments */
  int i;
  lua_getglobal(L, "tostring");
  for (i=1; i<=n; i++) {
    const char *s;
    lua_pushvalue(L, -1);  /* function to be called */
    lua_pushvalue(L, i);   /* value to print */
    lua_call(L, 1, 1);
    s = lua_tostring(L, -1);  /* get result */
    if (s == NULL)
      return luaL_error(L, LUA_QL("tostring") " must return a string to "
                           LUA_QL("print"));
    if (i>1) Msg("\t");
    Msg(s);
    lua_pop(L, 1);  /* pop result */
  }
  Msg("\n");
  return 0;
}


static int luasrc_type (lua_State *L) {
  luaL_checkany(L, 1);
  if (lua_getmetatable(L, 1)) {
    lua_pushstring(L, "__type");
	lua_rawget(L, -2);
	lua_remove(L, -2);
	if (!lua_isstring(L, -1))
	  lua_pop(L, 1);
	else
	  return 1;
  }
  lua_pushstring(L, luaL_typename(L, 1));
  return 1;
}


static int luasrc_include (lua_State *L) {
  lua_Debug ar1;
  lua_getstack(L, 1, &ar1);
  lua_getinfo(L, "f", &ar1);
  lua_Debug ar2;
  lua_getinfo(L, ">S", &ar2);
  int iLength = Q_strlen( ar2.source );
  char source[MAX_PATH];
  Q_StrRight( ar2.source, iLength-1, source, sizeof( source ) );
  Q_StripFilename( source );
  char filename[MAX_PATH];
  Q_snprintf( filename, sizeof( filename ), "%s\\%s", source, luaL_checkstring(L, 1) );
  luasrc_dofile(L, filename);
  return 0;
}


static const luaL_Reg base_funcs[] = {
  {"print", luasrc_print},
  {"type", luasrc_type},
  {"include", luasrc_include},
  {NULL, NULL}
};


static void base_open (lua_State *L) {
  /* set global _R */
  lua_pushvalue(L, LUA_REGISTRYINDEX);
  lua_setglobal(L, "_R");
  /* open lib into global table */
  luaL_register(L, "_G", base_funcs);
  lua_pop(L, 1);
  /* set global _E */
  lua_newtable(L);
  lua_setglobal(L, "_E");
#ifdef CLIENT_DLL
  lua_pushboolean(L, 1);
  lua_setglobal(L, "_CLIENT");  /* set global _CLIENT */
#else
  lua_pushboolean(L, 1);
  lua_setglobal(L, "_GAME");  /* set global _GAME */
#endif
}


void luasrc_setmodulepaths(lua_State *L) {
  lua_getglobal(L, LUA_LOADLIBNAME);
#ifdef CLIENT_DLL
	const char *gamePath = engine->GetGameDirectory();
#else
	char gamePath[ 256 ];
	engine->GetGameDir( gamePath, 256 );
#endif

  //Andrew; set package.cpath.
  lua_getfield(L, -1, "cpath");
  //MAX_PATH + package.cpath:len();
  char lookupCPath[MAX_PATH+99];
  Q_snprintf( lookupCPath, sizeof( lookupCPath ), "%s\\%s;%s", gamePath,
#ifdef _WIN32
    LUA_PATH_MODULES "\\?.dll",
#elif _LINUX
    LUA_PATH_MODULES "\\?.so",
#endif
	luaL_checkstring(L, -1) );
  Q_strlower( lookupCPath );
  Q_FixSlashes( lookupCPath );
  lua_pop(L, 1);  /* pop result */
  lua_pushstring(L, lookupCPath);
  lua_setfield(L, -2, "cpath");

  //Andrew; set package.path.
  lua_getfield(L, -1, "path");
  //MAX_PATH + package.path:len();
  char lookupPath[MAX_PATH+197];
  Q_snprintf( lookupPath, sizeof( lookupPath ), "%s\\%s;%s", gamePath, LUA_PATH_MODULES "\\?.lua", luaL_checkstring(L, -1) );
  Q_strlower( lookupPath );
  Q_FixSlashes( lookupPath );
  lua_pop(L, 1);  /* pop result */
  lua_pushstring(L, lookupPath);
  lua_setfield(L, -2, "path");

  lua_pop(L, 1);  /* pop result */
}

#ifdef CLIENT_DLL
void luasrc_init_gameui (void) {
  LGameUI = luaL_newstate();

  luaL_openlibs(LGameUI);
  base_open(LGameUI);
  lua_pushboolean(LGameUI, 1);
  lua_setglobal(LGameUI, "_GAMEUI");  /* set global _GAMEUI */

  luasrc_setmodulepaths(LGameUI);

  luaopen_ConCommand(LGameUI);
  luaopen_dbg(LGameUI);
  luaopen_engine(LGameUI);
  luaopen_enginevgui(LGameUI);
  luaopen_FCVAR(LGameUI);
  luaopen_KeyValues(LGameUI);
  luaopen_Panel(LGameUI);
  luaopen_surface(LGameUI);
  luaopen_vgui(LGameUI);
}

void luasrc_shutdown_gameui (void) {
  ResetGameUIConCommandDatabase();

  lua_close(LGameUI);
}
#endif

void luasrc_init (void) {
  if (g_bLuaInitialized)
	  return;
  g_bLuaInitialized = true;

  L = lua_open();

  luaL_openlibs(L);
  base_open(L);
  lcf_open(L);

  // Andrew; Someone set us up the path for great justice
  luasrc_setmodulepaths(L);

  luasrc_openlibs(L);

  Msg( "Lua initialized (" LUA_VERSION ")\n" );
}

void luasrc_shutdown (void) {
  if (!g_bLuaInitialized)
	  return;

  g_bLuaInitialized = false;

  filesystem->RemoveSearchPath( contentSearchPath, "MOD" );

  ResetConCommandDatabase();

  RemoveGlobalChangeCallbacks();
  ResetConVarDatabase();

#ifndef CLIENT_DLL
  ResetTriggerFactoryDatabase();
#endif
  ResetEntityFactoryDatabase();
  ResetWeaponFactoryDatabase();

  lcf_close(L);
  lua_close(L);
}

LUA_API int luasrc_dostring (lua_State *L, const char *string) {
  int iError = luaL_dostring(L, string);
  if (iError != 0) {
    Warning( "%s\n", lua_tostring(L, -1) );
    lua_pop(L, 1);
  }
  return iError;
}

LUA_API int luasrc_dofile (lua_State *L, const char *filename) {
  int iError = luaL_dofile(L, filename);
  if (iError != 0) {
    Warning( "%s\n", lua_tostring(L, -1) );
    lua_pop(L, 1);
  }
  return iError;
}

LUA_API void luasrc_dofolder (lua_State *L, const char *path)
{
	FileFindHandle_t fh;

	char searchPath[ 512 ];
	Q_snprintf( searchPath, sizeof( searchPath ), "%s\\*.lua", path );

	char const *fn = g_pFullFileSystem->FindFirstEx( searchPath, "MOD", &fh );
	while ( fn )
	{
		if ( fn[0] != '.' )
		{
			char ext[ 10 ];
			Q_ExtractFileExtension( fn, ext, sizeof( ext ) );

			if ( !Q_stricmp( ext, "lua" ) )
			{
				char relative[ 512 ];
				char loadname[ 512 ];
				Q_snprintf( relative, sizeof( relative ), "%s\\%s", path, fn );
				filesystem->RelativePathToFullPath( relative, "MOD", loadname, sizeof( loadname ) );
				luasrc_dofile( L, loadname );
			}
		}

		fn = g_pFullFileSystem->FindNext( fh );
	}
	g_pFullFileSystem->FindClose( fh );
}

LUA_API int luasrc_pcall (lua_State *L, int nargs, int nresults, int errfunc) {
  int iError = lua_pcall(L, nargs, nresults, errfunc);
  if (iError != 0) {
	Warning( "%s\n", lua_tostring(L, -1) );
	lua_pop(L, 1);
  }
  return iError;
}

LUA_API void luasrc_print(lua_State *L, int narg) {
  lua_getglobal(L, "tostring");
  const char *s;
  lua_pushvalue(L, -1);  /* function to be called */
  lua_pushvalue(L, narg);   /* value to print */
  lua_call(L, 1, 1);
  s = lua_tostring(L, -1);  /* get result */
  Msg( " %d:\t%s\n", narg, s );
  lua_pop(L, 1);  /* pop result */
  lua_pop(L, 1);  /* pop function */
}

LUA_API void luasrc_dumpstack(lua_State *L) {
  int n = lua_gettop(L);  /* number of objects */
  int i;
  lua_getglobal(L, "tostring");
  for (i=1; i<=n; i++) {
	const char *s;
	lua_pushvalue(L, -1);  /* function to be called */
	lua_pushvalue(L, i);   /* value to print */
	lua_call(L, 1, 1);
	s = lua_tostring(L, -1);  /* get result */
	Msg( " %d:\t%s\n", i, s );
	lua_pop(L, 1);  /* pop result */
  }
  lua_pop(L, 1);  /* pop function */
}

void luasrc_LoadEntities (const char *path)
{
	FileFindHandle_t fh;

	if ( !path )
	{
		path = "";
	}

	char root[ MAX_PATH ] = { 0 };

	char filename[ MAX_PATH ] = { 0 };
	char fullpath[ MAX_PATH ] = { 0 };
	char className[ 255 ] = { 0 };

	Q_snprintf( root, sizeof( root ), "%s" LUA_PATH_ENTITIES "\\*", path );

	char const *fn = g_pFullFileSystem->FindFirstEx( root, "MOD", &fh );
	while ( fn )
	{
		Q_strcpy( className, fn );
		Q_strlower( className );
		if ( fn[0] != '.' )
		{
			if ( g_pFullFileSystem->FindIsDirectory( fh ) )
			{
#ifdef CLIENT_DLL
				Q_snprintf( filename, sizeof( filename ), "%s" LUA_PATH_ENTITIES "\\%s\\cl_init.lua", path, className );
#else
				Q_snprintf( filename, sizeof( filename ), "%s" LUA_PATH_ENTITIES "\\%s\\init.lua", path, className );
#endif
				if ( filesystem->FileExists( filename, "MOD" ) )
				{
					filesystem->RelativePathToFullPath( filename, "MOD", fullpath, sizeof( fullpath ) );
					lua_newtable( L );
					char entDir[ MAX_PATH ];
					Q_snprintf( entDir, sizeof( entDir ), "entities\\%s", className );
					lua_pushstring( L, entDir );
					lua_setfield( L, -2, "__folder" );
					lua_pushstring( L, LUA_BASE_ENTITY_CLASS );
					lua_setfield( L, -2, "__base" );
					lua_pushstring( L, LUA_BASE_ENTITY_FACTORY );
					lua_setfield( L, -2, "__factory" );
					lua_setglobal( L, "ENT" );
					if ( luasrc_dofile( L, fullpath ) == 0 )
					{
						lua_getglobal( L, "entity" );
						if ( lua_istable( L, -1 ) )
						{
							lua_getfield( L, -1, "register" );
							if ( lua_isfunction( L, -1 ) )
							{
								lua_remove( L, -2 );
								lua_getglobal( L, "ENT" );
								lua_pushstring( L, className );
								luasrc_pcall( L, 2, 0, 0 );
								lua_getglobal( L, "ENT" );
								if ( lua_istable( L, -1 ) )
								{
									lua_getfield( L, -1, "__factory" );
									if ( lua_isstring( L, -1 ) )
									{
										const char *pszClassname = lua_tostring( L, -1 );
										if (Q_strcmp(pszClassname, "CBaseAnimating") == 0)
											RegisterScriptedEntity( className );
#ifndef CLIENT_DLL
										else if (Q_strcmp(pszClassname, "CBaseTrigger") == 0)
											RegisterScriptedTrigger( className );
#endif
									}
									lua_pop( L, 2 );
								}
								else
								{
									lua_pop( L, 1 );
								}
							}
							else
							{
								lua_pop( L, 2 );
							}
						}
						else
						{
							lua_pop( L, 1 );
						}
					}
					lua_pushnil( L );
					lua_setglobal( L, "ENT" );
				}
			}
		}

		fn = g_pFullFileSystem->FindNext( fh );
	}
	g_pFullFileSystem->FindClose( fh );
}

void luasrc_LoadWeapons (const char *path)
{
	FileFindHandle_t fh;

	if ( !path )
	{
		path = "";
	}

	char root[ MAX_PATH ] = { 0 };

	char filename[ MAX_PATH ] = { 0 };
	char fullpath[ MAX_PATH ] = { 0 };
	char className[ MAX_WEAPON_STRING ] = { 0 };

	Q_snprintf( root, sizeof( root ), "%s" LUA_PATH_WEAPONS "\\*", path );

	char const *fn = g_pFullFileSystem->FindFirstEx( root, "MOD", &fh );
	while ( fn )
	{
		Q_strcpy( className, fn );
		Q_strlower( className );
		if ( fn[0] != '.' )
		{
			if ( g_pFullFileSystem->FindIsDirectory( fh ) )
			{
#ifdef CLIENT_DLL
				Q_snprintf( filename, sizeof( filename ), "%s" LUA_PATH_WEAPONS "\\%s\\cl_init.lua", path, className );
#else
				Q_snprintf( filename, sizeof( filename ), "%s" LUA_PATH_WEAPONS "\\%s\\init.lua", path, className );
#endif
				if ( filesystem->FileExists( filename, "MOD" ) )
				{
					filesystem->RelativePathToFullPath( filename, "MOD", fullpath, sizeof( fullpath ) );
					lua_newtable( L );
					char entDir[ MAX_PATH ];
					Q_snprintf( entDir, sizeof( entDir ), "weapons\\%s", className );
					lua_pushstring( L, entDir );
					lua_setfield( L, -2, "__folder" );
					lua_pushstring( L, LUA_BASE_WEAPON );
					lua_setfield( L, -2, "__base" );
					lua_setglobal( L, "SWEP" );
					if ( luasrc_dofile( L, fullpath ) == 0 )
					{
						lua_getglobal( L, "weapon" );
						if ( lua_istable( L, -1 ) )
						{
							lua_getfield( L, -1, "register" );
							if ( lua_isfunction( L, -1 ) )
							{
								lua_remove( L, -2 );
								lua_getglobal( L, "SWEP" );
								lua_pushstring( L, className );
								luasrc_pcall( L, 2, 0, 0 );
								RegisterScriptedWeapon( className );
							}
							else
							{
								lua_pop( L, 2 );
							}
						}
						else
						{
							lua_pop( L, 1 );
						}
					}
					lua_pushnil( L );
					lua_setglobal( L, "SWEP" );
				}
			}
		}

		fn = g_pFullFileSystem->FindNext( fh );
	}
	g_pFullFileSystem->FindClose( fh );
}

bool luasrc_LoadGamemode (const char *gamemode) {
  lua_newtable(L);
  lua_pushstring(L, "__folder");
  char gamemodepath[MAX_PATH];
  Q_snprintf( gamemodepath, sizeof( gamemodepath ), "gamemodes\\%s", gamemode );
  lua_pushstring(L, gamemodepath);
  lua_settable(L, -3);
  lua_setglobal(L, "GM");
  char filename[MAX_PATH];
  char fullpath[MAX_PATH];
#ifdef CLIENT_DLL
  Q_snprintf( filename, sizeof( filename ), "%s\\gamemode\\cl_init.lua", gamemodepath );
#else
  Q_snprintf( filename, sizeof( filename ), "%s\\gamemode\\init.lua", gamemodepath );
#endif
  if ( filesystem->FileExists( filename, "MOD" ) )
  {
    filesystem->RelativePathToFullPath( filename, "MOD", fullpath, sizeof( fullpath ) );
	if (luasrc_dofile(L, fullpath) == 0) {
	  lua_getglobal(L, "gamemode");
	  lua_getfield(L, -1, "register");
	  lua_remove(L, -2);
	  lua_getglobal(L, "GM");
	  lua_pushstring(L, gamemode);
	  lua_getfield(L, -2, "__base");
	  if (lua_isnoneornil(L, -1) && Q_strcmp(gamemode, LUA_BASE_GAMEMODE) != 0) {
	    lua_pop(L, 1);
		lua_pushstring(L, LUA_BASE_GAMEMODE);
	  }
	  luasrc_pcall(L, 3, 0, 0);
	  lua_pushnil(L);
	  lua_setglobal(L, "GM");
	  return true;
	}
	else
	{
	  lua_pushnil(L);
	  lua_setglobal(L, "GM");
	  Warning( "ERROR: Attempted to load an invalid gamemode!\n" );
	  return false;
	}
  }
  else
  {
    lua_pushnil(L);
	lua_setglobal(L, "GM");
	Warning( "ERROR: Attempted to load an invalid gamemode!\n" );
    return false;
  }
}

bool luasrc_SetGamemode (const char *gamemode) {
  lua_getglobal(L, "gamemode");
  if (lua_istable(L, -1)) {
    lua_getfield(L, -1, "get");
	if (lua_isfunction(L, -1)) {
	  lua_remove(L, -2);
	  lua_pushstring(L, gamemode);
	  luasrc_pcall(L, 1, 1, 0);
	  lua_setglobal(L, "_GAMEMODE");
	  Q_snprintf( contentSearchPath, sizeof( contentSearchPath ), "gamemodes\\%s\\content", gamemode );
	  filesystem->AddSearchPath( contentSearchPath, "MOD" );
	  char loadPath[MAX_PATH];
	  Q_snprintf( loadPath, sizeof( loadPath ), "%s\\", contentSearchPath );
	  luasrc_LoadWeapons( loadPath );
	  luasrc_LoadEntities( loadPath );
	  // luasrc_LoadEffects( loadPath );
	  BEGIN_LUA_CALL_HOOK("Initialize");
	  END_LUA_CALL_HOOK(0,0);
	  return true;
	}
	else
	{
	  lua_pop(L, 2);
	  Warning( "ERROR: Failed to set gamemode!\n" );
	  return false;
	}
  }
  else
  {
    lua_pop(L, 1);
	Warning( "ERROR: Failed to load gamemode module!\n" );
	return false;
  }
}

#ifdef LUA_SDK
#ifdef CLIENT_DLL
	CON_COMMAND( lua_dostring_cl, "Run a Lua string" )
	{
		if ( !g_bLuaInitialized )
			return;

		if ( args.ArgC() == 1 )
		{
			Msg( "Usage: lua_dostring_cl <string>\n" );
			return;
		}

		int status = luasrc_dostring( L, args.ArgS() );
		if (status == 0 && lua_gettop(L) > 0) {  /* any result to print? */
		  lua_getglobal(L, "print");
		  lua_insert(L, 1);
		  if (lua_pcall(L, lua_gettop(L)-1, 0, 0) != 0)
			Warning("%s", lua_pushfstring(L,
							  "error calling " LUA_QL("print") " (%s)",
							  lua_tostring(L, -1)));
		}
		lua_settop(L, 0);  /* clear stack */
	}
#else
	CON_COMMAND( lua_dostring, "Run a Lua string" )
	{
		if ( !g_bLuaInitialized )
			return;

		if ( !UTIL_IsCommandIssuedByServerAdmin() )
			return;

		if ( args.ArgC() == 1 )
		{
			Msg( "Usage: lua_dostring <string>\n" );
			return;
		}

		int status = luasrc_dostring( L, args.ArgS() );
		if (status == 0 && lua_gettop(L) > 0) {  /* any result to print? */
		  lua_getglobal(L, "print");
		  lua_insert(L, 1);
		  if (lua_pcall(L, lua_gettop(L)-1, 0, 0) != 0)
			Warning("%s", lua_pushfstring(L,
							  "error calling " LUA_QL("print") " (%s)",
							  lua_tostring(L, -1)));
		}
		lua_settop(L, 0);  /* clear stack */
	}
#endif

static int DoFileCompletion( const char *partial, char commands[ COMMAND_COMPLETION_MAXITEMS ][ COMMAND_COMPLETION_ITEM_LENGTH ] )
{
	int current = 0;

#ifdef CLIENT_DLL
	const char *cmdname = "lua_dofile_cl";
#else
	const char *cmdname = "lua_dofile";
#endif
	char *substring = NULL;
	int substringLen = 0;
	if ( Q_strstr( partial, cmdname ) && strlen(partial) > strlen(cmdname) + 1 )
	{
		substring = (char *)partial + strlen( cmdname ) + 1;
		substringLen = strlen(substring);
	}
	
	FileFindHandle_t fh;

	char WildCard[ MAX_PATH ] = { 0 };
	if ( substring == NULL )
		substring = "";
	Q_snprintf( WildCard, sizeof( WildCard ), LUA_ROOT "\\%s*", substring );
	Q_FixSlashes( WildCard );
	char const *fn = g_pFullFileSystem->FindFirstEx( WildCard, "MOD", &fh );
	while ( fn && current < COMMAND_COMPLETION_MAXITEMS )
	{
		if ( fn[0] != '.' )
		{
			char filename[ MAX_PATH ] = { 0 };
			Q_snprintf( filename, sizeof( filename ), LUA_ROOT "\\%s\\%s", substring, fn );
			Q_FixSlashes( filename );
			if ( filesystem->FileExists( filename, "MOD" ) )
			{
				Q_snprintf( commands[ current ], sizeof( commands[ current ] ), "%s %s%s", cmdname, substring, fn );
				current++;
			}
		}

		fn = g_pFullFileSystem->FindNext( fh );
	}
	g_pFullFileSystem->FindClose( fh );

	return current;
}

#ifdef CLIENT_DLL
	CON_COMMAND_F_COMPLETION( lua_dofile_cl, "Load and run a Lua file", 0, DoFileCompletion )
	{
		if ( !g_bLuaInitialized )
			return;

		if ( args.ArgC() == 1 )
		{
			Msg( "Usage: lua_dofile_cl <filename>\n" );
			return;
		}

		char fullpath[ 512 ] = { 0 };
		char filename[ 256 ] = { 0 };
		Q_snprintf( filename, sizeof( filename ), LUA_ROOT "\\%s", args.ArgS() );
		Q_strlower( filename );
		Q_FixSlashes( filename );
		if ( filesystem->FileExists( filename, "MOD" ) )
		{
			filesystem->RelativePathToFullPath( filename, "MOD", fullpath, sizeof( fullpath ) );
		}
		else
		{
			Q_snprintf( fullpath, sizeof( fullpath ), "%s\\" LUA_ROOT "\\%s", engine->GetGameDirectory(), args.ArgS() );
			Q_strlower( fullpath );
			Q_FixSlashes( fullpath );
		}

		if ( Q_strstr( fullpath, ".." ) )
		{
			return;
		}
		Msg( "Running file %s...\n", args.ArgS() );
		luasrc_dofile( L, fullpath );
	}
#else
	CON_COMMAND_F_COMPLETION( lua_dofile, "Load and run a Lua file", 0, DoFileCompletion )
	{
		if ( !g_bLuaInitialized )
			return;

		if ( !UTIL_IsCommandIssuedByServerAdmin() )
			return;

		if ( args.ArgC() == 1 )
		{
			Msg( "Usage: lua_dofile <filename>\n" );
			return;
		}

		char fullpath[ 512 ] = { 0 };
		char filename[ 256 ] = { 0 };
		Q_snprintf( filename, sizeof( filename ), LUA_ROOT "lua\\%s", args.ArgS() );
		Q_strlower( filename );
		Q_FixSlashes( filename );
		if ( filesystem->FileExists( filename, "MOD" ) )
		{
			filesystem->RelativePathToFullPath( filename, "MOD", fullpath, sizeof( fullpath ) );
		}
		else
		{
			// filename is local to game dir for Steam, so we need to prepend game dir for regular file load
			char gamePath[256];
			engine->GetGameDir( gamePath, 256 );
			Q_StripTrailingSlash( gamePath );
			Q_snprintf( fullpath, sizeof( fullpath ), "%s\\" LUA_ROOT "\\%s", gamePath, args.ArgS() );
			Q_strlower( fullpath );
			Q_FixSlashes( fullpath );
		}

		if ( Q_strstr( fullpath, ".." ) )
		{
			return;
		}
		Msg( "Running file %s...\n", args.ArgS() );
		luasrc_dofile( L, fullpath );
	}
#endif

#if DEBUG
#ifdef CLIENT_DLL
	CON_COMMAND( lua_dumpstack_cl, "Prints the Lua stack" )
	{
	  if (!g_bLuaInitialized)
	    return;
	  int n = lua_gettop(L);  /* number of objects */
	  int i;
	  lua_getglobal(L, "tostring");
	  for (i=1; i<=n; i++) {
		const char *s;
		lua_pushvalue(L, -1);  /* function to be called */
		lua_pushvalue(L, i);   /* value to print */
		lua_call(L, 1, 1);
		s = lua_tostring(L, -1);  /* get result */
		Warning( " %d:\t%s\n", i, s );
		lua_pop(L, 1);  /* pop result */
	  }
	  lua_pop(L, 1);  /* pop function */
	  if (n>0)
	    Warning( "Warning: %d object(s) left on the stack!\n", n );
	}
#else
	CON_COMMAND( lua_dumpstack, "Prints the Lua stack" )
	{
	  if (!g_bLuaInitialized)
	    return;
	  int n = lua_gettop(L);  /* number of objects */
	  int i;
	  lua_getglobal(L, "tostring");
	  for (i=1; i<=n; i++) {
		const char *s;
		lua_pushvalue(L, -1);  /* function to be called */
		lua_pushvalue(L, i);   /* value to print */
		lua_call(L, 1, 1);
		s = lua_tostring(L, -1);  /* get result */
		Warning( " %d:\t%s\n", i, s );
		lua_pop(L, 1);  /* pop result */
	  }
	  lua_pop(L, 1);  /* pop function */
	  if (n>0)
	    Warning( "Warning: %d object(s) left on the stack!\n", n );
	}
#endif
#endif
#endif
