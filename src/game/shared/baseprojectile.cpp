//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "baseprojectile.h"


IMPLEMENT_NETWORKCLASS_ALIASED( BaseProjectile, DT_BaseProjectile )

BEGIN_NETWORK_TABLE( CBaseProjectile, DT_BaseProjectile )
#if !defined( CLIENT_DLL )
	SendPropEHandle( SENDINFO( m_hOriginalLauncher ) ),
#else
	RecvPropEHandle( RECVINFO( m_hOriginalLauncher ) ),
#endif // CLIENT_DLL
END_NETWORK_TABLE()


#ifndef CLIENT_DLL
IMPLEMENT_AUTO_LIST( IBaseProjectileAutoList );
#endif // !CLIENT_DLL


//-----------------------------------------------------------------------------
// Purpose: Constructor.
//-----------------------------------------------------------------------------
CBaseProjectile::CBaseProjectile()
{
#ifdef GAME_DLL
	m_iDestroyableHitCount = 0;

	m_bCanCollideWithTeammates = false;
#endif
	m_hOriginalLauncher = NULL;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseProjectile::SetLauncher( CBaseEntity *pLauncher )
{
	if ( m_hOriginalLauncher == NULL )
	{
		m_hOriginalLauncher = pLauncher;
	}

#ifdef GAME_DLL
	ResetCollideWithTeammates();
#endif // GAME_DLL
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseProjectile::Spawn()
{
	BaseClass::Spawn();

#ifdef GAME_DLL
	ResetCollideWithTeammates();
#endif // GAME_DLL
}


#ifdef GAME_DLL

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseProjectile::CollideWithTeammatesThink()
{
	m_bCanCollideWithTeammates = true;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseProjectile::ResetCollideWithTeammates()
{
	// Don't collide with players on the owner's team for the first bit of our life
	m_bCanCollideWithTeammates = false;
	
	SetContextThink( &CBaseProjectile::CollideWithTeammatesThink, gpGlobals->curtime + GetCollideWithTeammatesDelay(), "CollideWithTeammates" );
}

#endif // GAME_DLL

