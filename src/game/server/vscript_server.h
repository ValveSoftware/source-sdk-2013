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

// ----------------------------------------------------------------------------
// Player Messages access
// ----------------------------------------------------------------------------
#define MAX_SCRIPT_USERMESSAGE_DATA 48

enum eScriptPlayerMessageType {
	TYPE_NULL,
	TYPE_BYTE,
	TYPE_CHAR,
	TYPE_SHORT,
	TYPE_WORD,
	TYPE_LONG,
	TYPE_FLOAT,
	TYPE_ANGLE,
	TYPE_COORD,
	TYPE_VEC3COORD,
	TYPE_VEC3NORMAL,
	TYPE_ANGLES,
	TYPE_STRING,
	TYPE_BOOL,
	TYPE_MAX
};

class CScriptUserMessage
{
public:
	CScriptUserMessage( const char* szMessageName );

	void Execute( HSCRIPT filterParams );

	void WriteByte( int iValue );
	void WriteChar( int iValue );
	void WriteShort( int iValue );
	void WriteWord( int iValue );
	void WriteLong( int iValue );
	void WriteFloat( float flValue );
	void WriteAngle( float flValue );
	void WriteCoord( float flValue );
	void WriteVec3Coord( const Vector& rgflValue );
	void WriteVec3Normal( const Vector& rgflValue );
	void WriteAngles( const QAngle& rgflValue );
	void WriteString( const char* szValue );
	void WriteBool( bool bValue );

private:
	void WriteValue( const char* szValue, int iType );
	
	char m_szMessageName[64];

	int iDataCount = 0;
	char m_szDataValues[MAX_SCRIPT_USERMESSAGE_DATA][512];
	int m_iDataTypes[MAX_SCRIPT_USERMESSAGE_DATA];
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
