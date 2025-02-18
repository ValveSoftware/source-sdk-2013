//========= Copyright Valve Corporation, All rights reserved. ============//
//
//----------------------------------------------------------------------------------------

#include "cbase.h"
#include "playerspawncache.h"

//--------------------------------------------------------------------------------

/*static*/ CPlayerSpawnCache &CPlayerSpawnCache::Instance()
{
	static CPlayerSpawnCache s_Instance;
	return s_Instance;
}

CPlayerSpawnCache::CPlayerSpawnCache()
{
	// Clear the cache
	Reset();

	// The only event we care about
	ListenForGameEvent( "game_newmap" );
}

void CPlayerSpawnCache::Reset()
{
	V_memset( &m_Data, 0, sizeof( m_Data ) );
}

void CPlayerSpawnCache::FireGameEvent( IGameEvent *pEvent )
{
	// On new map, clear the cache
	if ( FStrEq( pEvent->GetName(), "game_newmap" ) )
	{
		Reset();
	}
}

//--------------------------------------------------------------------------------