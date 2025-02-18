//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"

#include "utlhashtable.h"
#ifndef GC
#include "igamesystem.h"
#endif
#include "gamestringpool.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: The actual storage for pooled per-level strings
//-----------------------------------------------------------------------------
#ifdef GC
class CGameStringPool
#else
class CGameStringPool : public CBaseGameSystem
#endif
{
	virtual char const *Name() { return "CGameStringPool"; }
	virtual void LevelShutdownPostEntity() { FreeAll(); }

	void FreeAll()
	{
#if 0 && _DEBUG
		m_Strings.DbgCheckIntegrity();
		m_KeyLookupCache.DbgCheckIntegrity();
#endif
		m_Strings.Purge();
		m_KeyLookupCache.Purge();
	}

	CUtlHashtable<CUtlConstString> m_Strings;
	CUtlHashtable<const void*, const char*> m_KeyLookupCache;

public:

	CGameStringPool() : m_Strings(256) { }

	~CGameStringPool() { FreeAll(); }

	void Dump( void )
	{
		CUtlVector<const char*> strings( 0, m_Strings.Count() );
		for (UtlHashHandle_t i = m_Strings.FirstHandle(); i != m_Strings.InvalidHandle(); i = m_Strings.NextHandle(i))
		{
			strings.AddToTail( strings[i] );
		}
		struct _Local {
			static int __cdecl F(const char * const *a, const char * const *b) { return strcmp(*a, *b); }
		};
		strings.Sort( _Local::F );
		
		for ( int i = 0; i < strings.Count(); ++i )
		{
			DevMsg( "  %d (0x%p) : %s\n", i, strings[i], strings[i] );
		}
		DevMsg( "\n" );
		DevMsg( "Size:  %d items\n", strings.Count() );
	}

	const char *Find(const char *string)
	{
		UtlHashHandle_t i = m_Strings.Find( string );
		return i == m_Strings.InvalidHandle() ? NULL : m_Strings[ i ].Get();
	}

	const char *Allocate(const char *string)
	{
		return m_Strings[ m_Strings.Insert( string ) ].Get();
	}

	const char *AllocateWithKey(const char *string, const void* key)
	{
		const char * &cached = m_KeyLookupCache[ m_KeyLookupCache.Insert( key, NULL ) ];
		if (cached == NULL)
		{
			cached = Allocate( string );
		}
		return cached;
	}
};

static CGameStringPool g_GameStringPool;

#ifndef GC
//-----------------------------------------------------------------------------
// String system accessor
//-----------------------------------------------------------------------------
IGameSystem *GameStringSystem()
{
	return &g_GameStringPool;
}
#endif


//-----------------------------------------------------------------------------
// Purpose: The public accessor for the level-global pooled strings
//-----------------------------------------------------------------------------
string_t AllocPooledString( const char * pszValue )
{
	if (pszValue && *pszValue)
		return MAKE_STRING( g_GameStringPool.Allocate( pszValue ) );
	return NULL_STRING;
}

string_t AllocPooledString_StaticConstantStringPointer( const char * pszGlobalConstValue )
{
	Assert(pszGlobalConstValue && *pszGlobalConstValue);
	return MAKE_STRING( g_GameStringPool.AllocateWithKey( pszGlobalConstValue, pszGlobalConstValue ) );
}

string_t FindPooledString( const char *pszValue )
{
	return MAKE_STRING( g_GameStringPool.Find( pszValue ) );
}

#if !defined(CLIENT_DLL) && !defined( GC )
//------------------------------------------------------------------------------
// Purpose: 
//------------------------------------------------------------------------------
void CC_DumpGameStringTable( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	g_GameStringPool.Dump();
}
static ConCommand dumpgamestringtable("dumpgamestringtable", CC_DumpGameStringTable, "Dump the contents of the game string table to the console.", FCVAR_CHEAT);
#endif
