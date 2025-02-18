//========== Copyright � 2008, Valve Corporation, All rights reserved. ========
//
// Purpose:
//
//=============================================================================

#ifndef VSCRIPT_SERVER_H
#define VSCRIPT_SERVER_H

#include "vscript/ivscript.h"
#include "tier1/KeyValues.h"
#include "vscript_shared.h"
#include "tier1/utlsymbol.h"
#include "GameEventListener.h"

#if defined( _WIN32 )
#pragma once
#endif

class ISaveRestoreBlockHandler;

bool VScriptServerReplaceClosures( const char *pszScriptName, HSCRIPT hScope, bool bWarnMissing = false );
ISaveRestoreBlockHandler *GetVScriptSaveRestoreBlockHandler();


class CBaseEntityScriptInstanceHelper : public IScriptInstanceHelper
{
	bool ToString( void *p, char *pBuf, int bufSize );
	void *BindOnRead( HSCRIPT hInstance, void *pOld, const char *pszId );
};

extern CBaseEntityScriptInstanceHelper g_BaseEntityScriptInstanceHelper;

#ifdef TF_DLL
class CNavAreaScriptInstanceHelper : public IScriptInstanceHelper
{
	bool ToString( void *p, char *pBuf, int bufSize );
};

extern CNavAreaScriptInstanceHelper g_NavAreaScriptInstanceHelper;

class INextBotComponentScriptInstanceHelper : public IScriptInstanceHelper
{
	bool ToString( void *p, char *pBuf, int bufSize );
};

extern INextBotComponentScriptInstanceHelper g_NextBotComponentScriptInstanceHelper;
#endif

// Only allow scripts to create entities during map initialization
bool IsEntityCreationAllowedInScripts( void );

// ----------------------------------------------------------------------------
// KeyValues access
// ----------------------------------------------------------------------------
class CScriptKeyValues
{
public:
	CScriptKeyValues( KeyValues *pKeyValues = NULL );
	~CScriptKeyValues( );

	HSCRIPT ScriptFindKey( const char *pszName );
	HSCRIPT ScriptGetFirstSubKey( void );
	HSCRIPT ScriptGetNextKey( void );
	int ScriptGetKeyValueInt( const char *pszName );
	float ScriptGetKeyValueFloat( const char *pszName );
	const char *ScriptGetKeyValueString( const char *pszName );
	bool ScriptIsKeyValueEmpty( const char *pszName );
	bool ScriptGetKeyValueBool( const char *pszName );
	void ScriptReleaseKeyValues( );

	KeyValues *m_pKeyValues;	// actual KeyValue entity
};

class CVScriptGameEventListener : public CGameEventListener
{
public:
	virtual void FireGameEvent( IGameEvent *event );
	bool FireScriptHook( const char *pszHookName, HSCRIPT params );
	
	void RunGameEventCallbacks( const char* szName, HSCRIPT params );
	void RunScriptHookCallbacks( const char* szName, HSCRIPT params );

	void Init();
	void CollectGameEventCallbacksInScope( HSCRIPT scope );

	void ListenForScriptHook( const char *szName );
	bool HasScriptHook( const char *szName );
	void ClearAllScriptHooks();

private:

	CUtlSymbolTable m_ScriptHooks;

	HSCRIPT m_RunGameEventCallbacksFunc;
	HSCRIPT m_CollectGameEventCallbacksFunc;
	HSCRIPT m_ScriptHookCallbacksFunc;
};

extern CVScriptGameEventListener g_VScriptGameEventListener;

bool ScriptHooksEnabled( void );
bool ScriptHookEnabled( const char *pszName );
bool RunScriptHook( const char *pszHookName, HSCRIPT params );

#endif // VSCRIPT_SERVER_H
