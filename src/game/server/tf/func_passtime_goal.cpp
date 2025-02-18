//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

// func_passtime_goal - based on func_capture_zone
#include "cbase.h"
#include "func_passtime_goal.h"
#include "tf_passtime_ball.h"
#include "tf_passtime_logic.h"
#include "passtime_convars.h"
#include "tf_team.h"
#include "tf_player.h"
#include "tf_gamerules.h"
#include "tf_gamestats.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
BEGIN_DATADESC( CFuncPasstimeGoal )
	DEFINE_KEYFIELD( m_iPoints, FIELD_INTEGER, "points" ),
	DEFINE_FUNCTION( CFuncPasstimeGoalShim::StartTouch ),
	DEFINE_FUNCTION( CFuncPasstimeGoalShim::EndTouch ),
	DEFINE_OUTPUT( m_onScoreBlu, "OnScoreBlu" ),
	DEFINE_OUTPUT( m_onScoreRed, "OnScoreRed" ),
END_DATADESC()

//-----------------------------------------------------------------------------
LINK_ENTITY_TO_CLASS( func_passtime_goal, CFuncPasstimeGoal );

//-----------------------------------------------------------------------------
IMPLEMENT_SERVERCLASS_ST( CFuncPasstimeGoal, DT_FuncPasstimeGoal )
	SendPropBool( SENDINFO( m_bTriggerDisabled ) ),
	SendPropInt( SENDINFO( m_iGoalType ) ),
END_SEND_TABLE()

//-----------------------------------------------------------------------------
CFuncPasstimeGoal::CFuncPasstimeGoal()
{
	m_iPoints = -1;
	m_bTriggerDisabled = false;
}

//-----------------------------------------------------------------------------
void CFuncPasstimeGoal::Spawn()
{
	// HACK spawnflags to work around initially wrong understanding of how triggers work; needs rewrite and map changes
	AddSpawnFlags( GetSpawnFlags() << 24 );
	RemoveSpawnFlags( 0xffffff );
	AddSpawnFlags( SF_TRIGGER_ALLOW_CLIENTS | SF_TRIGGER_ALLOW_PHYSICS );

	InitTrigger();
	m_bTriggerDisabled = m_bDisabled;
	SetThink( &CFuncPasstimeGoal::GoalThink );
	SetNextThink( gpGlobals->curtime );
	
	// set goal type
	if ( BTowerGoal() )
	{
		m_iGoalType = TYPE_TOWER;
	}
	else if ( BEnablePlayerScore() )
	{
		m_iGoalType = TYPE_ENDZONE;
	}
	else
	{
		m_iGoalType = TYPE_HOOP;
	}
}

//-----------------------------------------------------------------------------
void CFuncPasstimeGoal::GoalThink()
{
	SetNextThink( gpGlobals->curtime );
	m_bTriggerDisabled = m_bDisabled;

	for( int i = 0; i < m_hTouchingEntities.Count(); ++i )
	{
		CTFPlayer *pPlayer = ToTFPlayer( m_hTouchingEntities[i] );
		if ( pPlayer )
		{
			g_pPasstimeLogic->OnStayInGoal( pPlayer, this );
		}
	}
}

//-----------------------------------------------------------------------------
bool CFuncPasstimeGoal::CanTouchMe( CBaseEntity *pOther )
{
	return !m_bDisabled
		&& (pOther != 0)
		&& (g_pPasstimeLogic != 0);
}

//-----------------------------------------------------------------------------
void CFuncPasstimeGoal::ShimStartTouch( CBaseEntity *pOther )
{
	if ( !CanTouchMe( pOther ) )
	{
		return;
	}
	if ( CPasstimeBall *pBall = dynamic_cast<CPasstimeBall*>( pOther ) )
	{
		g_pPasstimeLogic->OnEnterGoal( pBall, this );
	}
	else if ( pOther->IsPlayer() )
	{
		g_pPasstimeLogic->OnEnterGoal( ToTFPlayer( pOther ), this );
	}
}

//-----------------------------------------------------------------------------
void CFuncPasstimeGoal::ShimEndTouch( CBaseEntity *pOther )
{
	if ( !CanTouchMe( pOther ) )
	{
		return;
	}
	if ( CPasstimeBall *pBall = dynamic_cast<CPasstimeBall*>( pOther ) )
	{
		g_pPasstimeLogic->OnExitGoal( pBall, this );
	}
}

//-----------------------------------------------------------------------------
void CFuncPasstimeGoal::OnScore( int iTeam ) 
{
	if( iTeam == TF_TEAM_RED )
	{
		m_onScoreRed.FireOutput( this, this );
	}
	else if( iTeam == TF_TEAM_BLUE )
	{
		m_onScoreBlu.FireOutput( this, this );
	}
}

//-----------------------------------------------------------------------------
int CFuncPasstimeGoal::UpdateTransmitState()
{
	// so the hud can point to it
	return SetTransmitState( FL_EDICT_ALWAYS );
}
