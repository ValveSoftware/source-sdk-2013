//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "passtime_ballcontroller_homing.h"
#include "passtime_convars.h"
#include "tf_passtime_logic.h"
#include "tf_passtime_ball.h"
#include "tf_weapon_passtime_gun.h"
#include "tf_gamestats.h"
#include "tf_gamerules.h"
#include "tf_player.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
CPasstimeBallControllerHoming::CPasstimeBallControllerHoming() 
	: CPasstimeBallController( 1 )
	, m_fTargetSpeed( 0 )
	, m_bIsHoming( false ) 
	, m_fHomingStrength( 0 )
{}

//-----------------------------------------------------------------------------
CPasstimeBallControllerHoming::~CPasstimeBallControllerHoming()
{
	StopHoming();
}

//-----------------------------------------------------------------------------
void CPasstimeBallControllerHoming::StartHoming( CPasstimeBall *pBall, CTFPlayer *pTarget, bool isCharged )
{
	Assert( pTarget && pBall );
	SetIsEnabled( true );
	m_bIsHoming = true;
	m_hTarget = pTarget;
	m_hBall = pBall;
	pBall->VPhysicsGetObject()->EnableGravity( false );
	m_fHomingStrength = 0.01f; // totally arbitrary
	pBall->SetHomingTarget( pTarget );

	if ( tf_passtime_experiment_instapass.GetBool() && (!tf_passtime_experiment_instapass_charge.GetBool() || isCharged) )
	{
		auto pos = pTarget->EyePosition();
		pBall->Teleport( &pos, nullptr, nullptr );
	}
}

//-----------------------------------------------------------------------------
void CPasstimeBallControllerHoming::StopHoming()
{
	if ( !m_bIsHoming )
		return;
	CPasstimeBall *pBall = m_hBall.Get();
	if ( pBall )
	{
		if( pBall->VPhysicsGetObject() )
		{
			pBall->VPhysicsGetObject()->EnableGravity( true );
		}
		pBall->SetHomingTarget( 0 );
	}
	m_bIsHoming = false;
	m_hTarget = 0;
	m_hBall = 0;
}

//-----------------------------------------------------------------------------
void CPasstimeBallControllerHoming::SetTargetSpeed( float f ) 
{ 
	m_fTargetSpeed = MAX(1.0f, f);
}

//-----------------------------------------------------------------------------
bool CPasstimeBallControllerHoming::IsActive() const 
{ 
	return m_bIsHoming;
}

//-----------------------------------------------------------------------------
bool CPasstimeBallControllerHoming::Apply( CPasstimeBall *ball )
{
	if ( ball != m_hBall )
		return false;

	if ( !m_hBall.Get() )
	{
		StopHoming();
		return false;
	}
	
	CTFPlayer *pPlayer = m_hTarget.Get();
	if( !pPlayer )
	{
		StopHoming();
		return false;
	}

	HudNotification_t cantPickUpReason;
	if ( pPlayer && !g_pPasstimeLogic->BCanPlayerPickUpBall( pPlayer, &cantPickUpReason ) )
	{
		if ( cantPickUpReason && TFGameRules() )
		{
			CSingleUserReliableRecipientFilter filter( pPlayer );
			TFGameRules()->SendHudNotification( filter, cantPickUpReason );
		}
		if ( m_bIsHoming )
		{
			++CTF_GameStats.m_passtimeStats.summary.nTotalPassesFailed;
			CTF_GameStats.m_passtimeStats.AddPassTravelDistSample( ball->GetAirtimeDistance() );
			StopHoming();
		}
		return false;
	}

	Assert( IsActive() && IsEnabled() );
	IPhysicsObject *pPhys = ball->VPhysicsGetObject();
	Vector ballpos;
	pPhys->GetPosition( &ballpos, 0 );

	Vector targetvel = pPlayer->EyePosition() - ballpos;
	targetvel.NormalizeInPlace();
	targetvel *= m_fTargetSpeed;
	
	Vector currentvel;
	pPhys->GetVelocity( &currentvel, 0 );
	Vector steer = targetvel - currentvel;
	pPhys->ApplyForceCenter( steer * m_fHomingStrength );
	m_fHomingStrength = clamp(m_fHomingStrength * 1.1f, 0, 1); // totally arbitrary

	return true;
}

//-----------------------------------------------------------------------------
void CPasstimeBallControllerHoming::OnBallCollision( 
	CPasstimeBall *ball, int index, gamevcollisionevent_t *ev )
{
	Assert( IsActive() && IsEnabled() );
	if ( ball != m_hBall )
		return;
	if ( m_iMaxBounces <= 0 )
	{
		if ( m_bIsHoming )
		{
			++CTF_GameStats.m_passtimeStats.summary.nTotalPassesFailed;
			CTF_GameStats.m_passtimeStats.AddPassTravelDistSample( ball->GetAirtimeDistance() );
			StopHoming();
		}
	}
	else
		--m_iMaxBounces;
}

//-----------------------------------------------------------------------------
void CPasstimeBallControllerHoming::OnBallPickedUp( 
	CPasstimeBall *ball, CTFPlayer *catcher )
{
	Assert( IsActive() && IsEnabled() );
	if ( ball == m_hBall )
		StopHoming();
}

//-----------------------------------------------------------------------------
void CPasstimeBallControllerHoming::OnBallDamaged( CPasstimeBall *ball ) 
{
	Assert( IsActive() && IsEnabled() );
	if ( ball == m_hBall && m_bIsHoming )
	{
		++CTF_GameStats.m_passtimeStats.summary.nTotalPassesShotDown;
		++CTF_GameStats.m_passtimeStats.summary.nTotalPassesFailed;
		CTF_GameStats.m_passtimeStats.AddPassTravelDistSample( ball->GetAirtimeDistance() );
		StopHoming();
	}
}

//-----------------------------------------------------------------------------
void CPasstimeBallControllerHoming::OnBallSpawned( CPasstimeBall *ball ) 
{
	Assert( IsActive() && IsEnabled() );
	StopHoming();
}

//-----------------------------------------------------------------------------
void CPasstimeBallControllerHoming::OnDisabled() 
{ 
	if ( m_bIsHoming )
	{
		++CTF_GameStats.m_passtimeStats.summary.nTotalPassesShotDown;
		++CTF_GameStats.m_passtimeStats.summary.nTotalPassesFailed;
		CTF_GameStats.m_passtimeStats.AddPassTravelDistSample( m_hBall->GetAirtimeDistance() );
	}
	StopHoming(); 
}
