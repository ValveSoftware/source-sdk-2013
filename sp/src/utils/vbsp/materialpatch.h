//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef MATERIALPATCH_H
#define MATERIALPATCH_H
#ifdef _WIN32
#pragma once
#endif

#include "utilmatlib.h"

struct MaterialPatchInfo_t
{
	const char *m_pKey;
	const char *m_pRequiredOriginalValue;  // NULL if you don't require one.
	const char *m_pValue;
	MaterialPatchInfo_t()
	{
		memset( this, 0, sizeof( *this ) );
	}
};

enum MaterialPatchType_t
{
	PATCH_INSERT = 0,	// Add the key no matter what
	PATCH_REPLACE,		// Add the key only if it exists
};

void CreateMaterialPatch( const char *pOriginalMaterialName, const char *pNewMaterialName,
						 const char *pNewKey, const char *pNewValue, MaterialPatchType_t nPatchType );

// A version which allows you to use multiple key values
void CreateMaterialPatch( const char *pOriginalMaterialName, const char *pNewMaterialName,
						 int nKeys, const MaterialPatchInfo_t *pInfo, MaterialPatchType_t nPatchType );

// This gets a keyvalue from the *unpatched* version of the passed-in material
bool GetValueFromMaterial( const char *pMaterialName, const char *pKey, char *pValue, int len );

// Gets a keyvalue from a *patched* material
bool GetValueFromPatchedMaterial( const char *pMaterialName, const char *pKey, char *pValue, int len );

const char *GetOriginalMaterialNameForPatchedMaterial( const char *pPatchMaterialName );

MaterialSystemMaterial_t FindOriginalMaterial( const char *materialName, bool *pFound, bool bComplain = true );

bool DoesMaterialHaveKeyValuePair( const char *pMaterialName, const char *pKeyName, const char *pSearchValue );
bool DoesMaterialHaveKey( const char *pMaterialName, const char *pKeyName );

enum LoadMaterialKeyValuesFlags_t
{
	LOAD_MATERIAL_KEY_VALUES_FLAGS_EXPAND_PATCH = 1,
};

KeyValues *LoadMaterialKeyValues( const char *pMaterialName, unsigned int nFlags );
void WriteMaterialKeyValuesToPak( const char *pMaterialName, KeyValues *kv );

#endif // MATERIALPATCH_H
