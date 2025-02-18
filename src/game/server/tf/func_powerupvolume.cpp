//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: CTF Powerup Volume.
//
//=============================================================================//

#include "cbase.h"
#include "tf_player.h"
#include "tf_item.h"
#include "tf_team.h"
#include "func_powerupvolume.h"
#include "tf_gamerules.h"
#include "eventqueue.h"

LINK_ENTITY_TO_CLASS( func_powerupvolume, CPowerupVolume );

#define TF_POWERUPVOLUME_SOUND			"Powerup.Volume.Use"
#define TF_POWERUPVOLUME_ANNOUNCE		"Announcer.Powerup.Volume.Starting"

IMPLEMENT_AUTO_LIST( IFuncPowerupVolumeAutoList );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CPowerupVolume::CPowerupVolume()
{
	m_bDisabled = false;
}

//-----------------------------------------------------------------------------
// Purpose: Spawn function for the entity
//-----------------------------------------------------------------------------
void CPowerupVolume::Spawn( void )
{
	Precache();
	InitTrigger();
	SetTouch( &CPowerupVolume::Touch );
}

//-----------------------------------------------------------------------------
// Purpose: Precache function for the entity
//-----------------------------------------------------------------------------
void CPowerupVolume::Precache( void )
{
	PrecacheScriptSound( TF_POWERUPVOLUME_SOUND );
	PrecacheScriptSound( TF_POWERUPVOLUME_ANNOUNCE );
}

//-----------------------------------------------------------------------------
// Purpose:		
//-----------------------------------------------------------------------------
void CPowerupVolume::Touch( CBaseEntity *pOther )
{
	if ( !IsDisabled() && pOther->IsPlayer() )
	{
		CTFPlayer *pPlayer = ToTFPlayer( pOther );
		if ( pPlayer && !( pPlayer->m_Shared.InCond( TF_COND_RUNE_IMBALANCE ) ) )
		{
			if ( pPlayer->IsTaunting() )
				return;

			if ( TFGameRules()->State_Get() == GR_STATE_TEAM_WIN )
				return;

			if ( TFGameRules()->InStalemate() )
				return;

			int iTeam = GetTeamNumber();
			
			if ( iTeam && ( pPlayer->GetTeamNumber() != iTeam ) )
					return;

			// call think function here so the first time the volume is used, it starts its timeout delay, and fires the tf_gamerules outputs when it times out
			
			pPlayer->m_Shared.AddCond( TF_COND_RUNE_IMBALANCE, 20.f );
			pPlayer->m_Shared.AddCond( TF_COND_CRITBOOSTED_RUNE_TEMP, 20.f );
			pPlayer->EmitSound( TF_POWERUPVOLUME_SOUND );
			m_nNumberOfTimesUsed++;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CPowerupVolume::IsDisabled( void )
{
	return m_bDisabled;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CPowerupVolume::SetDisabled( bool bDisabled )
{
	m_bDisabled = bDisabled;
}