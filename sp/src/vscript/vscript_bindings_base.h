//========= Mapbase - https://github.com/mapbase-source/source-sdk-2013 =================
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef VSCRIPT_BINDINGS_BASE
#define VSCRIPT_BINDINGS_BASE
#ifdef _WIN32
#pragma once
#endif

#include "vscript/ivscript.h"
#include "tier1/KeyValues.h"

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

	// Functions below are new with Mapbase
	void TableToSubKeys( HSCRIPT hTable );
	void SubKeysToTable( HSCRIPT hTable );

	HSCRIPT ScriptFindOrCreateKey( const char *pszName );

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

	KeyValues *GetKeyValues() { return m_pKeyValues; }

	KeyValues *m_pKeyValues;	// actual KeyValue entity
};

//-----------------------------------------------------------------------------
// Exposes Color to VScript
//-----------------------------------------------------------------------------
class CScriptColorInstanceHelper : public IScriptInstanceHelper
{
	bool ToString( void *p, char *pBuf, int bufSize );

	bool Get( void *p, const char *pszKey, ScriptVariant_t &variant );
	bool Set( void *p, const char *pszKey, ScriptVariant_t &variant );
};

void RegisterBaseBindings( IScriptVM *pVM );

#endif
