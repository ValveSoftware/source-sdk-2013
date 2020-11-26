//========= Mapbase - https://github.com/mapbase-source/source-sdk-2013 ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "tier1/KeyValues.h"
#include "tier1/fmtstr.h"

#include "vbsp.h"
#include "map.h"
#include "fgdlib/fgdlib.h"

#include "vscript_vbsp.h"
#include "vscript_funcs_vmfs.h"

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

static HSCRIPT VMFKV_CreateBlank()
{
	KeyValues *pKV = new KeyValues("VMF");

	KeyValues *pWorld = pKV->FindKey( "world", true );
	if (pWorld)
	{
		pWorld->SetString( "classname", "worldspawn" );
	}

	return scriptmanager->CreateScriptKeyValues( g_pScriptVM, pKV, true );
}

static bool VMFKV_SaveToFile( const char *szFile, HSCRIPT hKV )
{
	Warning( "Getting keyvalues from thing\n" );

	KeyValues *pKV = scriptmanager->GetKeyValuesFromScriptKV( g_pScriptVM, hKV );
	if (!pKV)
		return false;

	CUtlBuffer buf( 0, 0, CUtlBuffer::TEXT_BUFFER );
	for (KeyValues *pSubKey = pKV->GetFirstSubKey(); pSubKey; pSubKey = pSubKey->GetNextKey())
	{
		pSubKey->RecursiveSaveToFile( buf, 0 );
	}

	char pszFullName[MAX_PATH];
	Q_ExtractFilePath( source, pszFullName, sizeof(pszFullName) );
	V_snprintf( pszFullName, sizeof(pszFullName), "%s/vscript_io/%s", pszFullName, szFile );

	if ( !V_RemoveDotSlashes( pszFullName, CORRECT_PATH_SEPARATOR, true ) )
	{
		Warning( "Invalid file location : %s\n", szFile );
		buf.Purge();
		return false;
	}

	int nSize = V_strlen(pszFullName) + 1;
	char *pszDir = (char*)stackalloc(nSize);
	V_memcpy( pszDir, pszFullName, nSize );
	V_StripFilename( pszDir );

	//g_pFullFileSystem->RelativePathToFullPath( szFile, NULL, pszFullName, sizeof( pszFullName ) );
	Warning( "Full path is %s!\n", pszFullName );
	g_pFullFileSystem->CreateDirHierarchy( pszDir, NULL );
	bool res = g_pFullFileSystem->WriteFile( pszFullName, NULL, buf );
	buf.Purge();
	return res;
}

static HSCRIPT VMFKV_LoadFromFile( const char *szFile )
{
	char pszFullName[MAX_PATH];
	V_snprintf( pszFullName, sizeof(pszFullName), NULL, szFile );

	if ( !V_RemoveDotSlashes( pszFullName, CORRECT_PATH_SEPARATOR, true ) )
	{
		DevWarning( 2, "Invalid file location : %s\n", szFile );
		return NULL;
	}

	KeyValues *pKV = new KeyValues( szFile );
	if ( !pKV->LoadFromFile( g_pFullFileSystem, pszFullName, NULL ) )
	{
		return NULL;
	}

	HSCRIPT hScript = scriptmanager->CreateScriptKeyValues( g_pScriptVM, pKV, true ); // bAllowDestruct is supposed to automatically remove the involved KV

	return hScript;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

inline void ScriptVarsToKV( ScriptVariant_t &varKey, ScriptVariant_t &varValue, KeyValues *pKV )
{
	switch (varValue.m_type)
	{
		case FIELD_CSTRING:		pKV->SetString( varKey.m_pszString, varValue.m_pszString ); break;
		case FIELD_INTEGER:		pKV->SetInt( varKey.m_pszString, varValue.m_int ); break;
		case FIELD_FLOAT:		pKV->SetFloat( varKey.m_pszString, varValue.m_float ); break;
		case FIELD_BOOLEAN:		pKV->SetBool( varKey.m_pszString, varValue.m_bool ); break;
		case FIELD_VECTOR:		pKV->SetString( varKey.m_pszString, CFmtStr( "%f %f %f", varValue.m_pVector->x, varValue.m_pVector->y, varValue.m_pVector->z ) ); break;
	}
}

static HSCRIPT VMFKV_AddEntityFromTables( HSCRIPT hVMF, HSCRIPT hKV, HSCRIPT hIO )
{
	KeyValues *pVMF = scriptmanager->GetKeyValuesFromScriptKV( g_pScriptVM, hVMF );
	if (!pVMF)
		return false;

	KeyValues *pEnt = pVMF->CreateNewKey();
	if (!pEnt)
		return false;

	pEnt->SetName( "entity" );

	int nIterator = -1;
	ScriptVariant_t varKey, varValue;
	while ((nIterator = g_pScriptVM->GetKeyValue( hKV, nIterator, &varKey, &varValue )) != -1)
	{
		ScriptVarsToKV( varKey, varValue, pEnt );

		g_pScriptVM->ReleaseValue( varKey );
		g_pScriptVM->ReleaseValue( varValue );
	}

	KeyValues *pConnections = pEnt->FindKey( "connections", true );
	if (hIO && pConnections)
	{
		nIterator = -1;
		while ((nIterator = g_pScriptVM->GetKeyValue( hIO, nIterator, &varKey, &varValue )) != -1)
		{
			ScriptVarsToKV( varKey, varValue, pEnt );

			g_pScriptVM->ReleaseValue( varKey );
			g_pScriptVM->ReleaseValue( varValue );
		}
	}

	return scriptmanager->CreateScriptKeyValues( g_pScriptVM, pEnt, false );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void RegisterVMFScriptFunctions()
{
	ScriptRegisterFunction( g_pScriptVM, VMFKV_CreateBlank, "Creates a CScriptKeyValues instance with VMF formatting." );
	ScriptRegisterFunction( g_pScriptVM, VMFKV_SaveToFile, "Saves a CScriptKeyValues instance with VMF formatting." );
	ScriptRegisterFunction( g_pScriptVM, VMFKV_LoadFromFile, "Loads a VMF as a CScriptKeyValues instance with VMF formatting." );
	ScriptRegisterFunction( g_pScriptVM, VMFKV_AddEntityFromTables, "Adds a VMF-formatted entity to a CScriptKeyValues instance." );
}
