//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "tf_weapon_parachute.h"
#include "tf_gamerules.h"

// Client specific.
#ifdef CLIENT_DLL
#include "c_tf_player.h"
// Server specific.
#else
#include "tf_player.h"
#include "tf_gamestats.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//=============================================================================
//
// Weapon Buff Item tables.
//
CREATE_SIMPLE_WEAPON_TABLE( TFParachute, tf_weapon_parachute )
CREATE_SIMPLE_WEAPON_TABLE( TFParachute_Primary, tf_weapon_parachute_primary )
CREATE_SIMPLE_WEAPON_TABLE( TFParachute_Secondary, tf_weapon_parachute_secondary )


//-----------------------------------------------------------------------------
CTFParachute::CTFParachute()
{
#ifdef CLIENT_DLL
	m_iParachuteAnimState = EParachuteRetracted;
#endif // CLIENT_DLL
}


//-----------------------------------------------------------------------------
void CTFParachute::CreateBanner()
{
	if ( m_hBannerEntity )
		return;

	BaseClass::CreateBanner();

#ifdef CLIENT_DLL
	if ( m_hBannerEntity )
	{
		m_iParachuteAnimState = EParachuteRetracted_Idle;
		int sequence = m_hBannerEntity->SelectWeightedSequence( ACT_PARACHUTE_RETRACT_IDLE );
		m_hBannerEntity->ResetSequence( sequence );
		m_flParachuteToIdleTime = -1;
	}
#endif // CLIENT_DLL
}


#ifdef CLIENT_DLL

//-----------------------------------------------------------------------------
void CTFParachute::ClientThink( void )
{
	BaseClass::ClientThink();

	ParachuteAnimThink();
}


//-----------------------------------------------------------------------------
void CTFParachute::ParachuteAnimThink( void )
{
	if ( m_iBuffType == -1 || m_iBuffType == 0 )
	{
		m_iBuffType = GetBuffType();
	}

	if ( m_iBuffType != EParachute )
		return;

	CTFPlayer* pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( !pPlayer )
		return;

	CreateBanner();
	if ( !m_hBannerEntity )
		return;

	bool bInCondition = pPlayer->m_Shared.InCond( TF_COND_PARACHUTE_ACTIVE );
	if ( pPlayer->m_Shared.InCond( TF_COND_HALLOWEEN_KART ) )
	{
		// Halloween Kart has its own parachute and we don't want to interfer with it here
		bInCondition = false;
	}

	bool bIsDeployed = m_iParachuteAnimState == EParachuteDeployed || m_iParachuteAnimState == EParachuteDeployed_Idle;
	int iAct = -1;

	// Track Anim State
	if ( m_flParachuteToIdleTime < gpGlobals->curtime && m_flParachuteToIdleTime != -1 )
	{
		if ( m_iParachuteAnimState == EParachuteDeployed )
		{
			iAct = ACT_PARACHUTE_DEPLOY_IDLE;
			m_iParachuteAnimState = EParachuteDeployed_Idle;
		}
		else
		{
			iAct = ACT_PARACHUTE_RETRACT_IDLE;
			m_iParachuteAnimState = EParachuteRetracted_Idle;
		}
		m_flParachuteToIdleTime = -1;
	}

	if ( bIsDeployed != bInCondition )
	{
		if ( bInCondition )
		{
			iAct = ACT_PARACHUTE_DEPLOY;
			m_iParachuteAnimState = EParachuteDeployed;
		}
		else
		{
			iAct = ACT_PARACHUTE_RETRACT;
			m_iParachuteAnimState = EParachuteRetracted;
		}
	}

	if ( iAct != -1 )
	{
		int sequence = m_hBannerEntity->SelectWeightedSequence( iAct );
		m_hBannerEntity->ResetSequence( sequence );
		if ( m_iParachuteAnimState == EParachuteDeployed || m_iParachuteAnimState == EParachuteRetracted )
		{
			m_flParachuteToIdleTime = m_hBannerEntity->SequenceDuration() + gpGlobals->curtime;
		}
	}
}

#endif // CLIENT_DLL
