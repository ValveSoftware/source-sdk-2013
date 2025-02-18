//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "baseanimating.h"
#include "tf_player.h"
#include "entity_soldier_statue.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


#define ENTITY_SOLDIER_STATUE_THINK				"StatueThink"
#define ENTITY_SOLDIER_STATUE_THINK_INTERVAL	1.f
#define ENTITY_SOLDIER_STATUE_SPEAK_INTERVAL	10.f
#define ENTITY_SOLDIER_STATUE_DISTANCE			62500.f
#define ENTITY_SOLDIER_STATUE_MODEL				"models/soldier_statue/soldier_statue.mdl"
#define ENTITY_SOLDIER_STATUE_SOUND				"Soldier.Statue"

IMPLEMENT_AUTO_LIST( ISoldierStatueAutoList );

LINK_ENTITY_TO_CLASS( entity_soldier_statue, CEntitySoldierStatue );


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEntitySoldierStatue::Precache( void )
{
	BaseClass::Precache();

	// We deliberately allow late precache here
	bool bAllowPrecache = CBaseEntity::IsPrecacheAllowed();
	CBaseEntity::SetAllowPrecache( true );
	PrecacheModel( ENTITY_SOLDIER_STATUE_MODEL );
	PrecacheScriptSound( ENTITY_SOLDIER_STATUE_SOUND );
	CBaseEntity::SetAllowPrecache( bAllowPrecache );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEntitySoldierStatue::Spawn( void )
{
	Precache();
	BaseClass::Spawn();

	SetModel( ENTITY_SOLDIER_STATUE_MODEL );
	SetMoveType( MOVETYPE_NONE );
	SetSolid( SOLID_BBOX );
	SetSequence( 0 );

	SetContextThink( &CEntitySoldierStatue::StatueThink, gpGlobals->curtime + ENTITY_SOLDIER_STATUE_THINK_INTERVAL, ENTITY_SOLDIER_STATUE_THINK );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEntitySoldierStatue::StatueThink( void )
{
	if ( m_flNextSpeakTime < gpGlobals->curtime )
	{
		CUtlVector< CTFPlayer * > vecPlayers;
		CollectPlayers( &vecPlayers, TF_TEAM_RED, true );
		CollectPlayers( &vecPlayers, TF_TEAM_BLUE, true, true );

		FOR_EACH_VEC( vecPlayers, i )
		{
			CTFPlayer *pPlayer = vecPlayers[i];

			if ( pPlayer &&
				 pPlayer->GetAbsVelocity().IsZero() && 
				 ( ( pPlayer->GetAbsOrigin() - GetAbsOrigin() ).LengthSqr() < ENTITY_SOLDIER_STATUE_DISTANCE ) &&
				 pPlayer->IsLineOfSightClear( this ) )
			{
				PlaySound();
			}
		}
	}
	
	SetContextThink( &CEntitySoldierStatue::StatueThink, gpGlobals->curtime + ENTITY_SOLDIER_STATUE_THINK_INTERVAL, ENTITY_SOLDIER_STATUE_THINK );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEntitySoldierStatue::PlaySound( void )
{
	CPASAttenuationFilter filter( this );
	CSoundParameters params;

	if ( CBaseEntity::GetParametersForSound( ENTITY_SOLDIER_STATUE_SOUND, params, NULL ) )
	{
		EmitSound_t es( params );
		EmitSound( filter, entindex(), es );

		// GetSoundDuration is not supported on Linux or for MP3s so just put in a random delay
		m_flNextSpeakTime = ( gpGlobals->curtime + RandomFloat( 12.f, 17.f ) );
	}
}

