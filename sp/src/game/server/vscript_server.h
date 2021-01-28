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

bool VScriptServerReplaceClosures( const char *pszScriptName, HSCRIPT hScope, bool bWarnMissing = false );

// Only allow scripts to create entities during map initialization
bool IsEntityCreationAllowedInScripts( void );

#ifndef MAPBASE_VSCRIPT // Mapbase adds this to the base library so that CScriptKeyValues can be accessed anywhere, like VBSP.
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

	KeyValues *m_pKeyValues;	// actual KeyValue entity
};
#endif

#endif // VSCRIPT_SERVER_H
