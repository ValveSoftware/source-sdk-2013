//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#include "vbsp.h"
#include "UtlBuffer.h"
#include "utlsymbol.h"
#include "utlrbtree.h"
#include "KeyValues.h"
#include "bsplib.h"
#include "materialpatch.h"
#include "tier1/strtools.h"

// case insensitive
static CUtlSymbolTable s_SymbolTable( 0, 32, true );

struct NameTranslationLookup_t
{
	CUtlSymbol m_OriginalFileName;
	CUtlSymbol m_PatchFileName;
};

static bool NameTranslationLessFunc( NameTranslationLookup_t const& src1, 
							  NameTranslationLookup_t const& src2 )
{
	return src1.m_PatchFileName < src2.m_PatchFileName;
}

CUtlRBTree<NameTranslationLookup_t, int> s_MapPatchedMatToOriginalMat( 0, 256, NameTranslationLessFunc );

void AddNewTranslation( const char *pOriginalMaterialName, const char *pNewMaterialName )
{
	NameTranslationLookup_t newEntry;
	
	newEntry.m_OriginalFileName = s_SymbolTable.AddString( pOriginalMaterialName );
	newEntry.m_PatchFileName = s_SymbolTable.AddString( pNewMaterialName );

	s_MapPatchedMatToOriginalMat.Insert( newEntry );
}

const char *GetOriginalMaterialNameForPatchedMaterial( const char *pPatchMaterialName )
{
	const char *pRetName = NULL;
	int id;
	NameTranslationLookup_t lookup;
	lookup.m_PatchFileName = s_SymbolTable.AddString( pPatchMaterialName );
	do
	{
		id = s_MapPatchedMatToOriginalMat.Find( lookup );
		if( id >= 0 )
		{
			NameTranslationLookup_t &found = s_MapPatchedMatToOriginalMat[id];
			lookup.m_PatchFileName = found.m_OriginalFileName;
			pRetName = s_SymbolTable.String( found.m_OriginalFileName );
		}
	} while( id >= 0 );
	if( !pRetName )
	{
		// This isn't a patched material, so just return the original name.
		return pPatchMaterialName;
	}
	return pRetName;
}


void CreateMaterialPatchRecursive( KeyValues *pOriginalKeyValues, KeyValues *pPatchKeyValues, int nKeys, const MaterialPatchInfo_t *pInfo )
{
	int i;
	for( i = 0; i < nKeys; i++ )
	{
		const char *pVal = pOriginalKeyValues->GetString( pInfo[i].m_pKey, NULL );
		if( !pVal )
			continue;
		if( pInfo[i].m_pRequiredOriginalValue && Q_stricmp( pVal, pInfo[i].m_pRequiredOriginalValue ) != 0 )
			continue;
		pPatchKeyValues->SetString( pInfo[i].m_pKey, pInfo[i].m_pValue );
	}
	KeyValues *pScan;
	for( pScan = pOriginalKeyValues->GetFirstTrueSubKey(); pScan; pScan = pScan->GetNextTrueSubKey() )
	{
		CreateMaterialPatchRecursive( pScan, pPatchKeyValues->FindKey( pScan->GetName(), true ), nKeys, pInfo );
	}
}

//-----------------------------------------------------------------------------
// A version which allows you to patch multiple key values
//-----------------------------------------------------------------------------
void CreateMaterialPatch( const char *pOriginalMaterialName, const char *pNewMaterialName,
						 int nKeys, const MaterialPatchInfo_t *pInfo, MaterialPatchType_t nPatchType )
{	
	char pOldVMTFile[ 512 ];
	char pNewVMTFile[ 512 ];

	AddNewTranslation( pOriginalMaterialName, pNewMaterialName );
	
	Q_snprintf( pOldVMTFile, 512, "materials/%s.vmt", pOriginalMaterialName );
	Q_snprintf( pNewVMTFile, 512, "materials/%s.vmt", pNewMaterialName );

//	printf( "Creating material patch file %s which points at %s\n", newVMTFile, oldVMTFile );

	KeyValues *kv = new KeyValues( "patch" );
	if ( !kv )
	{
		Error( "Couldn't allocate KeyValues for %s!!!", pNewMaterialName );
	}

	kv->SetString( "include", pOldVMTFile );

	const char *pSectionName = (nPatchType == PATCH_INSERT) ? "insert" : "replace";
	KeyValues *section = kv->FindKey( pSectionName, true );

	if( nPatchType == PATCH_REPLACE )
	{
		char name[512];
		Q_snprintf( name, 512, "materials/%s.vmt", GetOriginalMaterialNameForPatchedMaterial( pOriginalMaterialName ) );
		KeyValues *origkv = new KeyValues( "blah" );

		if ( !origkv->LoadFromFile( g_pFileSystem, name ) )
		{
			origkv->deleteThis();
			Assert( 0 );
			return;
		}

		CreateMaterialPatchRecursive( origkv, section, nKeys, pInfo );
		origkv->deleteThis();
	}
	else
	{
		for ( int i = 0; i < nKeys; ++i )
		{
			section->SetString( pInfo[i].m_pKey, pInfo[i].m_pValue );
		}
	}
	
	// Write patched .vmt into a memory buffer
	CUtlBuffer buf( 0, 0, CUtlBuffer::TEXT_BUFFER );
	kv->RecursiveSaveToFile( buf, 0 );

	// Add to pak file for this .bsp
	AddBufferToPak( GetPakFile(), pNewVMTFile, (void*)buf.Base(), buf.TellPut(), true );

	// Cleanup
	kv->deleteThis();
}


//-----------------------------------------------------------------------------
// Patches a single keyvalue in a material 
//-----------------------------------------------------------------------------
void CreateMaterialPatch( const char *pOriginalMaterialName, const char *pNewMaterialName,
						 const char *pNewKey, const char *pNewValue, MaterialPatchType_t nPatchType )
{
	MaterialPatchInfo_t info;
	info.m_pKey = pNewKey;
	info.m_pValue = pNewValue;
	CreateMaterialPatch( pOriginalMaterialName, pNewMaterialName, 1, &info, nPatchType );
}


//-----------------------------------------------------------------------------
// Scan material + all subsections for key
//-----------------------------------------------------------------------------
static bool DoesMaterialHaveKey( KeyValues *pKeyValues, const char *pKeyName )
{
	const char *pVal;
	pVal = pKeyValues->GetString( pKeyName, NULL );
	if ( pVal != NULL  )
		return true;

	for( KeyValues *pSubKey = pKeyValues->GetFirstTrueSubKey(); pSubKey; pSubKey = pSubKey->GetNextTrueSubKey() )
	{
		if ( DoesMaterialHaveKey( pSubKey, pKeyName) )
			return true;
	}
	
	return false;
}

//-----------------------------------------------------------------------------
// Scan material + all subsections for key/value pair
//-----------------------------------------------------------------------------
static bool DoesMaterialHaveKeyValuePair( KeyValues *pKeyValues, const char *pKeyName, const char *pSearchValue )
{
	const char *pVal;
	pVal = pKeyValues->GetString( pKeyName, NULL );
	if ( pVal != NULL && ( Q_stricmp( pSearchValue, pVal ) == 0 ) )
		return true;

	for( KeyValues *pSubKey = pKeyValues->GetFirstTrueSubKey(); pSubKey; pSubKey = pSubKey->GetNextTrueSubKey() )
	{
		if ( DoesMaterialHaveKeyValuePair( pSubKey, pKeyName, pSearchValue ) )
			return true;
	}
	
	return false;
}

//-----------------------------------------------------------------------------
// Scan material + all subsections for key
//-----------------------------------------------------------------------------
bool DoesMaterialHaveKey( const char *pMaterialName, const char *pKeyName )
{
	char name[512];
	Q_snprintf( name, 512, "materials/%s.vmt", GetOriginalMaterialNameForPatchedMaterial( pMaterialName ) );
	KeyValues *kv = new KeyValues( "blah" );

	if ( !kv->LoadFromFile( g_pFileSystem, name ) )
	{
		kv->deleteThis();
		return NULL;
	}

	bool retVal = DoesMaterialHaveKey( kv, pKeyName );

	kv->deleteThis();
	return retVal;
}

//-----------------------------------------------------------------------------
// Scan material + all subsections for key/value pair
//-----------------------------------------------------------------------------
bool DoesMaterialHaveKeyValuePair( const char *pMaterialName, const char *pKeyName, const char *pSearchValue )
{
	char name[512];
	Q_snprintf( name, 512, "materials/%s.vmt", GetOriginalMaterialNameForPatchedMaterial( pMaterialName ) );
	KeyValues *kv = new KeyValues( "blah" );

	if ( !kv->LoadFromFile( g_pFileSystem, name ) )
	{
		kv->deleteThis();
		return NULL;
	}

	bool retVal = DoesMaterialHaveKeyValuePair( kv, pKeyName, pSearchValue );

	kv->deleteThis();
	return retVal;
}

//-----------------------------------------------------------------------------
// Gets a material value from a material. Ignores all patches
//-----------------------------------------------------------------------------
bool GetValueFromMaterial( const char *pMaterialName, const char *pKey, char *pValue, int len )
{
	char name[512];
	Q_snprintf( name, 512, "materials/%s.vmt", GetOriginalMaterialNameForPatchedMaterial( pMaterialName ) );
	KeyValues *kv = new KeyValues( "blah" );

	if ( !kv->LoadFromFile( g_pFileSystem, name ) )
	{
//		Assert( 0 );
		kv->deleteThis();
		return NULL;
	}

	const char *pTmpValue = kv->GetString( pKey, NULL );
	if( pTmpValue )
	{
		Q_strncpy( pValue, pTmpValue, len );
	}

	kv->deleteThis();
	return ( pTmpValue != NULL );
}


//-----------------------------------------------------------------------------
// Finds the original material associated with a patched material
//-----------------------------------------------------------------------------
MaterialSystemMaterial_t FindOriginalMaterial( const char *materialName, bool *pFound, bool bComplain )
{
	MaterialSystemMaterial_t matID;
	matID = FindMaterial( GetOriginalMaterialNameForPatchedMaterial( materialName ), pFound, bComplain );
	return matID;
}


//-----------------------------------------------------------------------------
// Load keyvalues from the local pack file, or from a file
//-----------------------------------------------------------------------------
bool LoadKeyValuesFromPackOrFile( const char *pFileName, KeyValues *pKeyValues )
{
	CUtlBuffer buf;
	if ( ReadFileFromPak( GetPakFile(), pFileName, true, buf ) )
	{
		return pKeyValues->LoadFromBuffer( pFileName, buf );
	}

	return pKeyValues->LoadFromFile( g_pFileSystem, pFileName );
}


//-----------------------------------------------------------------------------
// VMT parser
//-----------------------------------------------------------------------------
static void InsertKeyValues( KeyValues &dst, KeyValues& src, bool bCheckForExistence )
{
	KeyValues *pSrcVar = src.GetFirstSubKey();
	while( pSrcVar )
	{
		if ( !bCheckForExistence || dst.FindKey( pSrcVar->GetName() ) )
		{
			switch( pSrcVar->GetDataType() )
			{
			case KeyValues::TYPE_STRING:
				dst.SetString( pSrcVar->GetName(), pSrcVar->GetString() );
				break;
			case KeyValues::TYPE_INT:
				dst.SetInt( pSrcVar->GetName(), pSrcVar->GetInt() );
				break;
			case KeyValues::TYPE_FLOAT:
				dst.SetFloat( pSrcVar->GetName(), pSrcVar->GetFloat() );
				break;
			case KeyValues::TYPE_PTR:
				dst.SetPtr( pSrcVar->GetName(), pSrcVar->GetPtr() );
				break;
			}
		}
		pSrcVar = pSrcVar->GetNextKey();
	}
}

static void ExpandPatchFile( KeyValues &keyValues )
{
	int nCount = 0;
	while( nCount < 10 && stricmp( keyValues.GetName(), "patch" ) == 0 )
	{
//		WriteKeyValuesToFile( "patch.txt", keyValues );
		const char *pIncludeFileName = keyValues.GetString( "include" );
		if( !pIncludeFileName )
			return;

		KeyValues * includeKeyValues = new KeyValues( "vmt" );
		int nBufLen = Q_strlen( pIncludeFileName ) +  Q_strlen( "materials/.vmt" ) + 1;
		char *pFileName = ( char * )stackalloc( nBufLen );
		Q_strncpy( pFileName, pIncludeFileName, nBufLen );
		bool bSuccess = LoadKeyValuesFromPackOrFile( pFileName, includeKeyValues );
		if ( !bSuccess )
		{
			includeKeyValues->deleteThis();
			return;
		}

		KeyValues *pInsertSection = keyValues.FindKey( "insert" );
		if( pInsertSection )
		{
			InsertKeyValues( *includeKeyValues, *pInsertSection, false );
			keyValues = *includeKeyValues;
		}

		KeyValues *pReplaceSection = keyValues.FindKey( "replace" );
		if( pReplaceSection )
		{
			InsertKeyValues( *includeKeyValues, *pReplaceSection, true );
			keyValues = *includeKeyValues;
		}

		// Could add other commands here, like "delete", "rename", etc.

		includeKeyValues->deleteThis();
		nCount++;
	}

	if( nCount >= 10 )
	{
		Warning( "Infinite recursion in patch file?\n" );
	}
}

KeyValues *LoadMaterialKeyValues( const char *pMaterialName, unsigned int nFlags )
{
	// Load the underlying file
	KeyValues *kv = new KeyValues( "blah" );

	char pFullMaterialName[512];
	Q_snprintf( pFullMaterialName, 512, "materials/%s.vmt", pMaterialName );
	if ( !LoadKeyValuesFromPackOrFile( pFullMaterialName, kv ) )
	{
		//		Assert( 0 );
		kv->deleteThis();
		return NULL;
	}

	if( nFlags & LOAD_MATERIAL_KEY_VALUES_FLAGS_EXPAND_PATCH )
	{
		ExpandPatchFile( *kv );
	}

	return kv;
}

void WriteMaterialKeyValuesToPak( const char *pMaterialName, KeyValues *kv )
{
	char pFullMaterialName[512];
	Q_snprintf( pFullMaterialName, 512, "materials/%s.vmt", pMaterialName );

	// Write patched .vmt into a memory buffer
	CUtlBuffer buf( 0, 0, CUtlBuffer::TEXT_BUFFER );
	kv->RecursiveSaveToFile( buf, 0 );

	// Add to pak file for this .bsp
	AddBufferToPak( GetPakFile(), pFullMaterialName, (void*)buf.Base(), buf.TellPut(), true );

	// Cleanup
	kv->deleteThis();
}


//-----------------------------------------------------------------------------
// Gets a keyvalue from a *patched* material
//-----------------------------------------------------------------------------
bool GetValueFromPatchedMaterial( const char *pMaterialName, const char *pKey, char *pValue, int len )
{
	// Load the underlying file so that we can check if env_cubemap is in there.
	KeyValues *kv = new KeyValues( "blah" );

	char pFullMaterialName[512];
	Q_snprintf( pFullMaterialName, 512, "materials/%s.vmt", pMaterialName );
	if ( !LoadKeyValuesFromPackOrFile( pFullMaterialName, kv ) )
	{
//		Assert( 0 );
		kv->deleteThis();
		return NULL;
	}

	ExpandPatchFile( *kv );

	const char *pTmpValue = kv->GetString( pKey, NULL );
	if( pTmpValue )
	{
		Q_strncpy( pValue, pTmpValue, len );
	}

	kv->deleteThis();
	return ( pTmpValue != NULL );
}
