//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"

#include "utlhashtable.h"
#include "igamesystem.h"
#include "gamestringpool.h"

#include "tier1/stringpool.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: The actual storage for pooled per-level strings
//-----------------------------------------------------------------------------
class CGameStringPool : public CStringPool,	public CBaseGameSystem
{
	virtual char const *Name() { return "CGameStringPool"; }

	virtual void LevelShutdownPostEntity() 
	{
		Cleanup();
	}

public:
	~CGameStringPool()
	{
		Cleanup();
	}

	void Cleanup()
	{
		FreeAll();
		PurgeDeferredDeleteList();
		PurgeKeyLookupCache();
	}
	
	void PurgeDeferredDeleteList()
	{
		for ( int i = 0; i < m_DeferredDeleteList.Count(); ++ i )
		{
			free( ( void * )m_DeferredDeleteList[ i ] );
		}
		m_DeferredDeleteList.Purge();
	}

	void PurgeKeyLookupCache()
	{
		m_KeyLookupCache.Purge();
	}

	void Dump( void )
	{
		for ( int i = m_Strings.FirstInorder(); i != m_Strings.InvalidIndex(); i = m_Strings.NextInorder(i) )
		{
			DevMsg( "  %d (0x%p) : %s\n", i, m_Strings[i], m_Strings[i] );
		}
		DevMsg( "\n" );
		DevMsg( "Size:  %d items\n", m_Strings.Count() );
	}

	void Remove( const char *pszValue )
	{
		int i = m_Strings.Find( pszValue );
		if ( i != m_Strings.InvalidIndex() )
		{
			m_DeferredDeleteList.AddToTail( m_Strings[ i ] );
			m_Strings.RemoveAt( i );
		}
	}

	const char *AllocateWithKey(const char *string, const void* key)
	{
		const char * &cached = m_KeyLookupCache[ m_KeyLookupCache.Insert( key, NULL ) ];
		if ( cached == NULL )
		{
			cached = Allocate( string );
		}
		return cached;
	}

private:
	CUtlVector< const char * > m_DeferredDeleteList;

	CUtlHashtable< const void*, const char* > m_KeyLookupCache;
};

static CGameStringPool g_GameStringPool;

//-----------------------------------------------------------------------------
// String system accessor
//-----------------------------------------------------------------------------
IGameSystem *GameStringSystem()
{
	return &g_GameStringPool;
}


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

void RemovePooledString( const char *pszValue )
{
	g_GameStringPool.Remove( pszValue );
}

void PurgeDeferredPooledStrings()
{
	g_GameStringPool.PurgeDeferredDeleteList();
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
