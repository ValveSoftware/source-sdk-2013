//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "tf_passtime_ball.h"
#include "tf_passtime_logic.h"
#include "trigger_passtime_ball.h"
#include "tf_player.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//----------------------------------------------------------------------
LINK_ENTITY_TO_CLASS( trigger_passtime_ball, CTriggerPasstimeBall );

//-----------------------------------------------------------------------------
BEGIN_DATADESC( CTriggerPasstimeBall )
	DEFINE_OUTPUT( m_onBallEnter, "OnBallEnter" ),
	DEFINE_OUTPUT( m_onBallExit, "OnBallExit" ),
END_DATADESC()

//-----------------------------------------------------------------------------
void CTriggerPasstimeBall::Spawn()
{
	m_bPresent = false;
	BaseClass::Spawn();

	SetSolid( SOLID_BSP );	
	AddSolidFlags( FSOLID_NOT_SOLID | FSOLID_TRIGGER );
	SetMoveType( MOVETYPE_NONE );
	SetModel( STRING( GetModelName() ) );    // set size and link into world
	AddEffects( EF_NODRAW );

	SetThink( &CTriggerPasstimeBall::Update );
	SetNextThink( gpGlobals->curtime );
}

//-----------------------------------------------------------------------------
bool CTriggerPasstimeBall::BTouching( CBaseEntity *pEnt )
{
	Ray_t ray;
	trace_t tr;
	ICollideable *pCollide = CollisionProp();
	ray.Init( pEnt->GetAbsOrigin(), pEnt->GetAbsOrigin() );
	enginetrace->ClipRayToCollideable( ray, MASK_ALL, pCollide, &tr );
	return ( tr.startsolid );
}

//-----------------------------------------------------------------------------
void CTriggerPasstimeBall::Update()
{
	// This is a bad way to do this, but I couldn't find any way to make
	// a normal trigger do what I want because I want enter/exit to be handled
	// correctly when the ball is hidden.
	// It would be more efficient to have the ball do this, but I'm 
	// trying to isolate this hack to where it makes the most sense.

	SetNextThink( gpGlobals->curtime );
	
	if ( !g_pPasstimeLogic || !g_pPasstimeLogic->GetBall() )
		return;

	CPasstimeBall *pBall = g_pPasstimeLogic->GetBall();
	CBaseEntity *pEnt = pBall->GetCarrier();
	if ( !pEnt ) pEnt = pBall;

	bool bPresentNow = (pEnt && BTouching( pEnt ));
	if ( bPresentNow && !m_bPresent )
	{
		m_onBallEnter.FireOutput( this, this );
	}
	else if ( !bPresentNow && m_bPresent )
	{
		m_onBallExit.FireOutput( this, this );
	}
	m_bPresent = bPresentNow;
}
