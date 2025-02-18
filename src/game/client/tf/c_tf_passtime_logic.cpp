//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "tempent.h"
#include "c_tf_passtime_logic.h"
#include "tf_hud_passtime_reticle.h"
#include "passtime_convars.h"
#include "passtime_game_events.h"
#include "tf_shareddefs.h"
#include "tf_classdata.h"
#include "c_tf_player.h"
#include "c_func_passtime_goal.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

void C_TFPasstimeLogic::PostDataUpdate( DataUpdateType_t updateType )
{
	BaseClass::PostDataUpdate( updateType );

	if ( updateType == DATA_UPDATE_CREATED )
	{
		SetNextClientThink( CLIENT_THINK_ALWAYS );
		m_pBallReticle = new C_PasstimeBallReticle();
		m_pPassReticle = new C_PasstimePassReticle();
		for( auto *pGoal : C_FuncPasstimeGoal::GetAutoList() ) 
		{
			m_pGoalReticles.AddToTail( new C_PasstimeGoalReticle( 
				static_cast<C_FuncPasstimeGoal*>( pGoal ) ) );
		}
	}
}

//-----------------------------------------------------------------------------
C_TFPasstimeLogic* g_pPasstimeLogic;
extern ConVar hud_fastswitch;

//-----------------------------------------------------------------------------
IMPLEMENT_CLIENTCLASS_DT( C_TFPasstimeLogic, DT_TFPasstimeLogic, CTFPasstimeLogic )
	RecvPropEHandle( RECVINFO( m_hBall ) ),
	RecvPropArray( RecvPropVector( RECVINFO( m_trackPoints[0] ) ), m_trackPoints ),
	RecvPropInt( RECVINFO( m_iNumSections ) ),
	RecvPropInt( RECVINFO( m_iCurrentSection ) ),
	RecvPropFloat( RECVINFO( m_flMaxPassRange ) ),
	RecvPropInt( RECVINFO( m_iBallPower ), 8 ),
	RecvPropFloat( RECVINFO( m_flPackSpeed ) ),
	RecvPropArray3( RECVINFO_ARRAY( m_bPlayerIsPackMember ), RecvPropInt( RECVINFO( m_bPlayerIsPackMember[0] ) ) ),
END_RECV_TABLE()

LINK_ENTITY_TO_CLASS( passtime_logic, C_TFPasstimeLogic );

//-----------------------------------------------------------------------------
C_TFPasstimeLogic::C_TFPasstimeLogic()
{
	m_pBallReticle = nullptr;
	m_pPassReticle = nullptr;
	memset( m_apPackBeams, 0, sizeof( m_apPackBeams ) );
	memset( m_bPlayerIsPackMember, 0, sizeof( m_bPlayerIsPackMember ) );
	for( int i = 0; i < m_trackPoints.Count(); ++i )
	{
		m_trackPoints.GetForModify(i).Zero();
	}

	g_pPasstimeLogic = this;
}

//-----------------------------------------------------------------------------
C_TFPasstimeLogic::~C_TFPasstimeLogic()
{
	delete m_pBallReticle;
	m_pGoalReticles.PurgeAndDeleteElements();
	delete m_pPassReticle;

	// Don't set g_pPasstimeLogic to null here because sometimes this destructor
	// happens after the contructor of the new object
	// FIXME: what's the right way to do this?
}

//-----------------------------------------------------------------------------
void C_TFPasstimeLogic::Spawn()
{
}

//-----------------------------------------------------------------------------
void C_TFPasstimeLogic::ClientThink()
{
	BaseClass::ClientThink();
	
	SetNextClientThink( CLIENT_THINK_ALWAYS );
	m_pBallReticle->OnClientThink();
	for ( auto *pGoal : m_pGoalReticles )
	{
		pGoal->OnClientThink();
	}
	m_pPassReticle->OnClientThink();
	UpdateBeams();
}

//-----------------------------------------------------------------------------
void C_TFPasstimeLogic::DestroyBeams( C_PasstimeBall *pBall )
{
	for ( CNewParticleEffect *pBeam : m_apPackBeams )
	{
		if ( pBeam )
		{
			pBall->ParticleProp()->StopEmission( pBeam );
		}
	}
	memset( m_apPackBeams, 0, sizeof( m_apPackBeams ) );
}

//-----------------------------------------------------------------------------
void C_TFPasstimeLogic::DestroyBeam( int i, C_PasstimeBall *pBall )
{
	CNewParticleEffect *pBeam = m_apPackBeams[i];
	if ( pBeam )
	{
		pBall->ParticleProp()->StopEmissionAndDestroyImmediately( pBeam );
		m_apPackBeams[i] = nullptr;
	}
}

//-----------------------------------------------------------------------------
void C_TFPasstimeLogic::UpdateBeams()
{
	C_PasstimeBall *pBall = GetBall();
	if ( !pBall )
	{
		return;
	}

	C_TFPlayer *pCarrier = pBall->GetCarrier();
	if ( !pCarrier )
	{
		DestroyBeams( pBall );
		return;
	}

	const char *pEffectName = "passtime_beam";
	CParticleProperty *pParticles = pBall->ParticleProp();

	for ( int i = 1; i <= MAX_PLAYERS; ++i )
	{
		if ( !m_bPlayerIsPackMember[i] )
		{
			DestroyBeam( i, pBall );
			continue;
		}

		CTFPlayer *pPlayer = (CTFPlayer*) UTIL_PlayerByIndex( i );
		if ( !pPlayer || ( pPlayer == pCarrier ) || !pPlayer->IsAlive() )
		{
			DestroyBeam( i, pBall );
			continue;
		}

		if ( !m_apPackBeams[i] )
		{
			CNewParticleEffect *pBeam = pParticles->Create( pEffectName, PATTACH_ABSORIGIN_FOLLOW );
			pParticles->AddControlPoint( pBeam, 1, pPlayer, PATTACH_ABSORIGIN_FOLLOW, 0, Vector(0,0,16) );
			m_apPackBeams[i] = pBeam;
		}
	}
}

//-----------------------------------------------------------------------------
void C_TFPasstimeLogic::GetTrackPoints( Vector (&points)[16] )
{
	memcpy( points, m_trackPoints.Base(), sizeof(points) );
}

//-----------------------------------------------------------------------------
bool C_TFPasstimeLogic::GetImportantEntities( C_PasstimeBall **ppBall, C_TFPlayer **ppCarrier, C_TFPlayer **ppHomingTarget ) const
{
	C_PasstimeBall *pBall = GetBall();
	if ( !pBall )
	{
		return false;
	}

	if ( ppBall )
	{
		*ppBall = pBall;
	}

	if ( ppCarrier )
	{
		*ppCarrier = pBall->GetCarrier();
	}

	if ( ppHomingTarget )
	{
		*ppHomingTarget = ToTFPlayer( pBall->GetHomingTarget() );
	}

	return true;
}

//-----------------------------------------------------------------------------
bool C_TFPasstimeLogic::GetBallReticleTarget( C_BaseEntity **ppEnt, bool *bHomingActive ) const
{
	Assert( ppEnt );
	if ( !ppEnt )
	{
		return false;
	}

	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pLocalPlayer )
	{
		return false;
	}

	C_PasstimeBall *pBall = 0;
	C_TFPlayer *pCarrier = 0, *pHomingTarget = 0;
	if ( !GetImportantEntities( &pBall, &pCarrier, &pHomingTarget ) )
	{
		return false;
	}

	C_BaseEntity *pEnt = pCarrier ? pCarrier : (C_BaseEntity*)pBall;
	if ( !pEnt 
		|| (pEnt == pLocalPlayer) 
		|| (pEnt->GetEffects() & EF_NODRAW) 
		|| ((pEnt->GetTeamNumber() != TEAM_UNASSIGNED) 
			&& (pEnt->GetTeamNumber() != pLocalPlayer->GetTeamNumber()))
		|| (pLocalPlayer->IsObserver() && (GetSpectatorMode() != OBS_MODE_ROAMING) && (GetSpectatorTarget() == pEnt->index)) )
	{
		return false;
	}

	*ppEnt = pEnt;
	if ( bHomingActive ) 
	{
		*bHomingActive = pHomingTarget != 0;
	}

	return true;
}

//-----------------------------------------------------------------------------
bool C_TFPasstimeLogic::BCanPlayerPickUpBall( C_TFPlayer *pPlayer ) const
{
	return pPlayer 
		&& pPlayer->IsAllowedToPickUpFlag()
		&& pPlayer->IsAlive() // NOTE: it's possible to be alive and dead at the same time
		&& !pPlayer->m_Shared.InCond( TF_COND_INVULNERABLE )
		&& !pPlayer->m_Shared.InCond( TF_COND_PHASE )
		&& !pPlayer->m_Shared.InCond( TF_COND_INVULNERABLE_WEARINGOFF )
		&& !pPlayer->m_Shared.InCond( TF_COND_SELECTED_TO_TELEPORT )
		&& !pPlayer->m_Shared.InCond( TF_COND_STEALTHED_BLINK )
		&& !pPlayer->m_Shared.InCond( TF_COND_TAUNTING )
		&& !pPlayer->m_Shared.InCond( TF_COND_HALLOWEEN_GHOST_MODE )
		&& !pPlayer->m_Shared.IsControlStunned()
		&& !pPlayer->m_Shared.IsStealthed()
		&& !pPlayer->m_Shared.IsCarryingObject();
}

