//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "tf_passtime_ball.h"
#include "tf_passtime_logic.h"
#include "passtime_convars.h"
#include "passtime_ballcontroller_playerseek.h"
#include "tf_player.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

namespace {
	const int kPriority = 1; // same as homing, so they stack
}

//-----------------------------------------------------------------------------
CPasstimeBallControllerPlayerSeek::CPasstimeBallControllerPlayerSeek() 
	: CPasstimeBallController( kPriority ) // low priority
	, m_fEnableTime( 0 )
{}

//-----------------------------------------------------------------------------
bool CPasstimeBallControllerPlayerSeek::Apply( CPasstimeBall *ball )
{
	if ( gpGlobals->curtime < m_fEnableTime )
		return false;
	return Seek( ball, FindTarget( ball->GetThrower(), ball->GetAbsOrigin() ) );
}

//-----------------------------------------------------------------------------
CTFPlayer *CPasstimeBallControllerPlayerSeek::FindTarget( CTFPlayer *pIgnorePlayer, const Vector& ballOrigin ) const
{
	CTFPlayer *pTarget = 0;
	float closestPlayerDist = tf_passtime_ball_seek_range.GetFloat();
	closestPlayerDist *= closestPlayerDist; // avoid some sqrts

	// Treat this trace exactly like radius damage
	CTraceFilterIgnorePlayers traceFilter( 0, COLLISION_GROUP_PLAYER_MOVEMENT );

	for ( int i = 1; i <= MAX_PLAYERS; i++ )
	{
		CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
		if ( !pPlayer )
			continue; // wat

		if ( pPlayer == pIgnorePlayer )
			continue;

		const Vector& eyepos = pPlayer->WorldSpaceCenter();
		const float dist = (eyepos - ballOrigin).LengthSqr();
		if ( dist >= closestPlayerDist )
			continue; // to far away

		if ( pPlayer->m_Shared.InCond( TF_COND_DISGUISED ) )
			continue; // don't seek any disguised people

		if ( !g_pPasstimeLogic->BCanPlayerPickUpBall( pPlayer ) )
			continue; // can't pick it up

		trace_t trace;
		UTIL_TraceLine( ballOrigin, eyepos, MASK_PLAYERSOLID, &traceFilter, &trace );
		if ( trace.fraction != 1 )
			continue; // occluded

		// chicken dinner
		pTarget = pPlayer;
		closestPlayerDist = dist;
	}
	return pTarget;
}

extern float GetCurrentGravity( void ); // tf_gamerules.h

//-----------------------------------------------------------------------------
bool CPasstimeBallControllerPlayerSeek::Seek( CPasstimeBall *ball, CTFPlayer *pTarget ) const
{
	if ( !pTarget )
		return false;

	// taken from ballcontroller_homing
	IPhysicsObject *pPhys = ball->VPhysicsGetObject();
	Vector ballpos;
	pPhys->GetPosition( &ballpos, 0 );

	Vector targetvel = pTarget->EyePosition() - ballpos;
	targetvel.NormalizeInPlace();
	targetvel *= pTarget->TeamFortress_CalculateMaxSpeed() * 
		tf_passtime_ball_seek_speed_factor.GetFloat();
	
	Vector currentvel;
	pPhys->GetVelocity( &currentvel, 0 );
	Vector steer = targetvel - currentvel;
	pPhys->AddVelocity( &steer, 0 );
	return true;
}

//-----------------------------------------------------------------------------
void CPasstimeBallControllerPlayerSeek::OnBallSpawned( CPasstimeBall *ball ) 
{
	// NOTE: this hack is the only thing that prevents players from picking up 
	// balls immediately after they were thrown (including by nearby unrelated
	// players)
	m_fEnableTime = gpGlobals->curtime + 0.25f;
}
