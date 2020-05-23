//========== Copyright © 2008, Valve Corporation, All rights reserved. ========
//
// Purpose:
//
//=============================================================================

#ifndef VSCRIPT_SERVER_H
#define VSCRIPT_SERVER_H

#include "vscript/ivscript.h"
#include "tier1/KeyValues.h"
#include "vscript_shared.h"

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

// Only allow scripts to create entities during map initialization
bool IsEntityCreationAllowedInScripts( void );

// ----------------------------------------------------------------------------
// KeyValues access
// ----------------------------------------------------------------------------
class CScriptKeyValues
{
public:
	CScriptKeyValues( KeyValues *pKeyValues );
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
#ifdef MAPBASE_VSCRIPT
	const char *ScriptGetName();
	int ScriptGetInt();
	float ScriptGetFloat();
	const char *ScriptGetString();
	bool ScriptGetBool();

	void ScriptSetKeyValueInt( const char *pszName, int iValue );
	void ScriptSetKeyValueFloat( const char *pszName, float flValue );
	void ScriptSetKeyValueString( const char *pszName, const char *pszValue );
	void ScriptSetKeyValueBool( const char *pszName, bool bValue );
	void ScriptSetName( const char *pszValue );
	void ScriptSetInt( int iValue );
	void ScriptSetFloat( float flValue );
	void ScriptSetString( const char *pszValue );
	void ScriptSetBool( bool bValue );
#endif

	KeyValues *m_pKeyValues;	// actual KeyValue entity
};

#endif // VSCRIPT_SERVER_H
