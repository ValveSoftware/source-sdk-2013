//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
/*

===== grenade_base.cpp ========================================================

  Base Handling for all the player's grenades

*/
#include "cbase.h"
#include "grenadethrown.h"
#include "ammodef.h"
#include "vstdlib/random.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Precaches a grenade and ensures clients know of it's "ammo"
void UTIL_PrecacheOtherGrenade( const char *szClassname )
{
	CBaseEntity *pEntity = CreateEntityByName( szClassname );
	if ( !pEntity )
	{
		Msg( "NULL Ent in UTIL_PrecacheOtherGrenade\n" );
		return;
	}
	
	CThrownGrenade *pGrenade = dynamic_cast<CThrownGrenade *>( pEntity );

	if (pGrenade)
	{
		pGrenade->Precache( );
	}

	UTIL_Remove( pEntity );
}

//-----------------------------------------------------------------------------
// Purpose: Setup basic values for Thrown grens
//-----------------------------------------------------------------------------
void CThrownGrenade::Spawn( void )
{
	// point sized, solid, bouncing
	SetMoveType( MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_BOUNCE );
	SetSolid( SOLID_BBOX );
	UTIL_SetSize(this, vec3_origin, vec3_origin);

	// Movement
	SetGravity( UTIL_ScaleForGravity( 648 ) );
	SetFriction(0.6);
	QAngle angles;
	VectorAngles( GetAbsVelocity(), angles );
	SetLocalAngles( angles );
	QAngle vecAngVel( random->RandomFloat ( -100, -500 ), 0, 0 );
	SetLocalAngularVelocity( vecAngVel );
	
	SetTouch( &CThrownGrenade::BounceTouch );
}

//-----------------------------------------------------------------------------
// Purpose: Throw the grenade.
// Input  : vecOrigin - Starting position
//			vecVelocity - Starting velocity
//			flExplodeTime - Time at which to detonate
//-----------------------------------------------------------------------------
void CThrownGrenade::Thrown( Vector vecOrigin, Vector vecVelocity, float flExplodeTime )
{
	// Throw
	SetLocalOrigin( vecOrigin );
	SetAbsVelocity( vecVelocity );

	// Explode in 3 seconds
	SetThink( &CThrownGrenade::Detonate );
	SetNextThink( gpGlobals->curtime + flExplodeTime );
}

