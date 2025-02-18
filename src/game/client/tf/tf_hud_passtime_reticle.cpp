//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "c_func_passtime_goal.h"
#include "c_tf_passtime_logic.h"
#include "tf_hud_passtime_reticle.h"
#include "passtime_convars.h"
#include "tf_weapon_passtime_gun.h"
#include "c_tf_player.h"
#include "view.h"
#include "c_tf_playerresource.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// The team colors from g_PR are wrong and I couldn't fix that fast enough.
// These colors were sampled from HUD art.
static Color GetTeamColor( int iTeam )
{
	switch( iTeam )
	{
	case TF_TEAM_RED: return Color(0xFF, 0x51, 0x51);
	case TF_TEAM_BLUE: return Color(0xA5, 0xDE, 0xFF);
	default: return Color(0xF5, 0xE7, 0xDE);
	};
}

//-----------------------------------------------------------------------------
// C_PasstimeReticle
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
C_PasstimeReticle::~C_PasstimeReticle() 
{
	for( int i = 0; i < m_pSprites.Count(); ++i )
	{
		clienteffects->RemoveEffect( m_pSprites[i] );
	}
}

//-----------------------------------------------------------------------------
void C_PasstimeReticle::OnClientThink()
{
	if ( !Update() )
	{
		SetAllAlphas( 0 );
	}
}

//-----------------------------------------------------------------------------
void C_PasstimeReticle::AddSprite( CFXQuad *pQuad )
{
	Assert( pQuad );
	m_pSprites.AddToTail( pQuad );
}

//-----------------------------------------------------------------------------
void C_PasstimeReticle::SetAllOrigins( const Vector &vec )
{
	for ( int i = 0; i < m_pSprites.Count(); ++i )
	{
		m_pSprites[i]->m_FXData.SetOrigin( vec );
	}
}

//-----------------------------------------------------------------------------
void C_PasstimeReticle::SetAllNormals( const Vector &vec )
{
	for ( int i = 0; i < m_pSprites.Count(); ++i )
	{
		m_pSprites[i]->m_FXData.SetNormal( vec );
	}
}

//-----------------------------------------------------------------------------
void C_PasstimeReticle::SetAllAlphas( byte iA )
{
	auto flA = iA / 255.0f;
	for ( int i = 0; i < m_pSprites.Count(); ++i )
	{
		m_pSprites[i]->m_FXData.SetAlpha( flA, flA );
	}
}

//-----------------------------------------------------------------------------
void C_PasstimeReticle::SetAllScales( float flScale )
{
	for ( int i = 0; i < m_pSprites.Count(); ++i )
	{
		m_pSprites[i]->m_FXData.SetScale( flScale, flScale );
	}
}

//-----------------------------------------------------------------------------
void C_PasstimeReticle::SetOrigin( int i, const Vector &vec )
{
	m_pSprites[i]->m_FXData.SetOrigin( vec );
}

//-----------------------------------------------------------------------------
void C_PasstimeReticle::SetNormal( int i, const Vector &normal )
{
	m_pSprites[i]->m_FXData.SetNormal( normal );
}

//-----------------------------------------------------------------------------
void C_PasstimeReticle::SetAlpha( int i, byte iA )
{
	auto flA = iA / 255.0f;
	m_pSprites[i]->m_FXData.SetAlpha( flA, flA );
}

//-----------------------------------------------------------------------------
void C_PasstimeReticle::SetRgba( int i, byte iR, byte iG, byte iB, byte iA )
{
	m_pSprites[i]->m_FXData.SetColor( iR / 255.0f, iG / 255.0f, iB / 255.0f );
	auto flA = iA / 255.0;
	m_pSprites[i]->m_FXData.SetAlpha( flA, flA );
}

//-----------------------------------------------------------------------------
void C_PasstimeReticle::SetScale( int i, float flScale )
{
	m_pSprites[i]->m_FXData.SetScale( flScale, flScale );
}


//-----------------------------------------------------------------------------
// C_PasstimeBallReticle
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
static const float k_flBallReticleSize = 64;
C_PasstimeBallReticle::C_PasstimeBallReticle()
{
	AddSprite( CreateReticleSprite( "passtime/hud/passtime_ball_reticle_piece_1", k_flBallReticleSize, 360 ) ); // the O
	AddSprite( CreateReticleSprite( "passtime/hud/passtime_ball_reticle_piece_2", k_flBallReticleSize, 360 ) ); // the ><
}

//-----------------------------------------------------------------------------
bool C_PasstimeBallReticle::Update()
{
	if ( !g_pPasstimeLogic || !g_pPasstimeLogic->GetBall() ) 
	{
		return false;
	}

	auto *pBall = g_pPasstimeLogic->GetBall();
	auto *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	C_BaseEntity *pTarget = 0;
	auto bHomingActive = false;
	auto bHaveTarget = g_pPasstimeLogic->GetBallReticleTarget( &pTarget, &bHomingActive );
	if ( !pBall || !pLocalPlayer || !bHaveTarget )
	{
		return false;
	}
	
	auto vecTargetPos = pTarget->WorldSpaceCenter();
	SetAllOrigins( vecTargetPos );
	SetAllNormals( -MainViewForward() );
		
	auto teamColor = GetTeamColor( pTarget->GetTeamNumber() );
	auto iAlpha = ( bHomingActive || pLocalPlayer->m_Shared.IsTargetedForPasstimePass() )
		? (int)( (fmodf( gpGlobals->curtime * 3.0f, 1.0f )) * 255 )
		: 180;

	SetRgba( 0, teamColor.r(), teamColor.g(), teamColor.b(), iAlpha );
	SetRgba( 1, teamColor.r(), teamColor.g(), teamColor.b(), iAlpha );

	auto flDist = (vecTargetPos - MainViewOrigin()).Length();
	auto flScale = RemapValClamped( flDist, 768.0f, 4096.0f, 1.0f, 3.0f );
	flScale *= k_flBallReticleSize;
	if ( bHomingActive || pLocalPlayer->m_Shared.IsTargetedForPasstimePass() )
	{
		flScale *= 2;
	}
	SetScale( 0, flScale );
	SetScale( 1, flScale );

	return true;
}

//-----------------------------------------------------------------------------
// C_PasstimeGoalReticle
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
C_PasstimeGoalReticle::C_PasstimeGoalReticle( C_FuncPasstimeGoal *pGoal )
{
	Assert( pGoal );
	m_hGoal.Set( pGoal );
	AddSprite( CreateReticleSprite( "passtime/hud/passtime_ball_reticle_piece_1", 256, 50 ) );
	AddSprite( CreateReticleSprite( "passtime/hud/passtime_ball_reticle_piece_2", 128, 0 ) );
}

//-----------------------------------------------------------------------------
bool C_PasstimeGoalReticle::Update()
{
	if ( !g_pPasstimeLogic || !g_pPasstimeLogic->GetBall() )
	{
		return false;
	}

	// don't show if ball isn't being carried by local player
	auto *pEnt = g_pPasstimeLogic->GetBall()->GetCarrier();
	if ( !pEnt || (pEnt != C_BasePlayer::GetLocalPlayer()) )
	{
		return false;
	}

	auto *pGoal = m_hGoal.Get();
	if ( !g_pPasstimeLogic || !g_pPasstimeLogic->GetBall() || IsLocalPlayerSpectator() 
		|| !pGoal || pGoal->BGoalTriggerDisabled() || (pGoal->GetTeamNumber() != pEnt->GetTeamNumber()) )
	{
		return false;
	}

	auto teamColor = GetTeamColor( pEnt->GetTeamNumber() );

	auto vec = pGoal->WorldSpaceCenter();
	auto facingFrac = MainViewForward().Dot( (vec - MainViewOrigin()).Normalized() );
	if ( facingFrac < 0.6 )
	{
		return false;
	}
	facingFrac = RemapValClamped( facingFrac, 0.8f, 1.0f, 1.0f, 0.3f );

	// ring
	SetOrigin( 0, vec );
	auto flPulseSpeed = 10.0f;
	auto flPulseFrac = Clamp( FastCos( gpGlobals->curtime * flPulseSpeed ), 0.0f, 1.0f );
	SetRgba( 0, teamColor.r(), teamColor.g(), teamColor.b(), flPulseFrac * (120 * facingFrac) );
	
	// arrow
	float tmp;
	flPulseFrac = 1.0f - modff( gpGlobals->curtime, &tmp );
	SetOrigin( 1, vec + Vector(0, 0, flPulseFrac * 64) );
	SetAllNormals( -MainViewForward() );
	SetRgba( 1, teamColor.r(), teamColor.g(), teamColor.b(), flPulseFrac * (255 * facingFrac) );
	return true;
}


//-----------------------------------------------------------------------------
// C_PasstimePassReticle
//-----------------------------------------------------------------------------

const float kPassReticleScale = 64;
//-----------------------------------------------------------------------------
C_PasstimePassReticle::C_PasstimePassReticle() 
{
	m_flTargetScore = FLT_MAX;
	AddSprite( CreateReticleSprite( "passtime/hud/passtime_ball_reticle_passlock", kPassReticleScale, 100 ) ); // the *
	AddSprite( CreateReticleSprite( "passtime/hud/passtime_ball_reticle_piece_1", kPassReticleScale, 0 ) ); // the O
	AddSprite( CreateReticleSprite( "passtime/hud/passtime_ball_reticle_piece_2", kPassReticleScale, 0 ) ); // the ><
}

//-----------------------------------------------------------------------------
bool C_PasstimePassReticle::Update()
{
	if ( !g_pPasstimeLogic || !g_pPasstimeLogic->GetBall() || IsLocalPlayerSpectator() )
	{
		return false;
	}

	auto *pBallCarrier = g_pPasstimeLogic->GetBall()->GetCarrier();
	if ( !pBallCarrier )
	{
		return false;
	}
	if ( (pBallCarrier != C_BasePlayer::GetLocalPlayer()) )
	{
		return false;
	}
	
	SetAllNormals( -MainViewForward() );

	// the player's actual pass target always takes precedence, but if it's
	// not set, try to find a candidate and display a hint for that
	auto *pPassTarget = pBallCarrier->m_Shared.GetPasstimePassTarget();
	if ( pPassTarget )
	{
		m_hTarget = pPassTarget;
		auto vecTargetPos = pPassTarget->WorldSpaceCenter();
		SetAllOrigins( vecTargetPos );

		auto teamColor = GetTeamColor( pBallCarrier->GetTeamNumber() );
		auto neutralColor = GetTeamColor( TEAM_UNASSIGNED );
		auto iAlpha = (int)( (fmodf( gpGlobals->curtime * 3.0f, 1.0f )) * 255 );
		SetRgba( 0, teamColor.r(), teamColor.g(), teamColor.b(), iAlpha );
		SetRgba( 1, neutralColor.r(), neutralColor.g(), neutralColor.b(), 255 );
		SetRgba( 2, teamColor.r(), teamColor.g(), teamColor.b(), iAlpha );

		auto flDist = (vecTargetPos - MainViewOrigin()).Length();
		auto flScale = RemapValClamped( flDist, 768.0f, 8192.0f, 1.0f, 8.0f );
		SetAllScales( flScale * kPassReticleScale * 2 );
	}
	else
	{
		FindPassHintTarget( pBallCarrier );
		if ( !m_hTarget )
		{
			return false;
		}

		auto flPulseSpeed = 20;
		auto flPulseFrac = Clamp( FastCos( gpGlobals->curtime * flPulseSpeed ), 0.3f, 1.0f );
		auto iAlpha = 200 * flPulseFrac * Clamp( m_flTargetScore + 0.5f, 0.0f, 1.0f ); // higher flScore is better

		auto teamColor = GetTeamColor( TEAM_UNASSIGNED );
		auto neutralColor = GetTeamColor( TEAM_UNASSIGNED );
		SetRgba( 0, teamColor.r(), teamColor.g(), teamColor.b(), iAlpha );
		SetRgba( 1, neutralColor.r(), neutralColor.g(), neutralColor.b(), 80 );
		SetRgba( 2, teamColor.r(), teamColor.g(), teamColor.b(), iAlpha );
		
		auto vecTargetPos = m_hTarget->WorldSpaceCenter();
		SetAllOrigins( vecTargetPos );
	
		auto flDist = (vecTargetPos - MainViewOrigin()).Length();
		auto flScale = RemapValClamped( flDist, 768.0f, 8192.0f, 1.0f, 8.0f );
		SetAllScales( flScale * kPassReticleScale );
	}
	
	return true;
}

//-----------------------------------------------------------------------------
extern int HudTransform( const Vector &point, Vector &screen );
void C_PasstimePassReticle::FindPassHintTarget( C_TFPlayer *pLocalPlayer )
{
	m_hTarget = 0;
	m_flTargetScore = -FLT_MAX;

	auto flFovDeg = 70;
	auto flDotFov = cosf( DEG2RAD( flFovDeg / 2.0f ) );
	auto vecViewPos = MainViewOrigin();
	auto vecViewFwd = MainViewForward();

	auto flMaxPassDist = g_pPasstimeLogic->GetMaxPassRange() - 400; // arbitrary, based on TF_MAX_SPEED

	// for each player in front of the local player,
	for( int i = 1; i <= MAX_PLAYERS; i++ )
	{
		auto *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
		if ( !C_PasstimeGun::BValidPassTarget( pLocalPlayer, pPlayer ) )
		{
			continue;
		}

		auto vecTargetPos = pPlayer->EyePosition();
		auto vecToTarget = vecTargetPos - vecViewPos;
		if ( vecToTarget.NormalizeInPlace() > flMaxPassDist ) 
		{
			// the server may disagree on this, so the client is less permissive
			// when it guesses in order to prevent false positives
			continue; // too far away, probably
		}

		auto fDotTarget = vecToTarget.Dot( vecViewFwd );
		if ( fDotTarget <= flDotFov )
		{
			continue; // not in front
		}

		// determine player's distance from center of screen 
		Vector vecScreenPos;
		auto bBehindViewplane = HudTransform( vecTargetPos, vecScreenPos );
		if ( bBehindViewplane )
		{
			continue; // paranoia
		}
		auto flScore = 0.5f - vecScreenPos.Length2D();
		if ( flScore <= m_flTargetScore )
		{
			continue; // someone else already found that's closer
		}

		// trace to see if they are visible
		// this is the same trace the gun does
		trace_t tr;
		CTraceFilterSimple tracefilter( pLocalPlayer, COLLISION_GROUP_PROJECTILE );
		UTIL_TraceLine( vecViewPos, vecTargetPos, MASK_PLAYERSOLID, &tracefilter, &tr );
		if ( tr.m_pEnt != pPlayer )
		{
			continue; // not visible
		}
	
		m_flTargetScore = flScore;
		m_hTarget = pPlayer;
	}
}


//-----------------------------------------------------------------------------
// C_PasstimeBounceReticle
//-----------------------------------------------------------------------------
C_PasstimeBounceReticle::C_PasstimeBounceReticle()
{
	AddSprite( CreateReticleSprite( "passtime/hud/passtime_ball_reticle_passlock", 160, 0 ) ); // the *
	AddSprite( CreateReticleSprite( "passtime/hud/passtime_ball_reticle_piece_1", 80, 200 ) ); // the O
}

void C_PasstimeBounceReticle::Show( const Vector &vec, const Vector &normal )
{
	SetOrigin( 0, vec );
	SetOrigin( 1, vec );//+ (normal * 16) );
	SetNormal( 0, normal );
	SetNormal( 1, -MainViewForward() );
	SetRgba( 0, 255, 255, 0, 200 );
	SetRgba( 1, 255, 255, 0, 200 );
}

void C_PasstimeBounceReticle::Hide()
{
	SetAllAlphas( 0 );
}

bool C_PasstimeBounceReticle::Update()
{
	return !IsLocalPlayerSpectator();
}

//-----------------------------------------------------------------------------
// C_PasstimePlayerReticle
//-----------------------------------------------------------------------------
C_PasstimePlayerReticle::C_PasstimePlayerReticle( C_TFPlayer *pPlayer )
{
	m_hPlayer.Set( pPlayer );
	AddSprite( CreateReticleSprite( "passtime/hud/passtime_teamicon_red", 128, 0 ) );
	AddSprite( CreateReticleSprite( "passtime/hud/passtime_teamicon_blue", 128, 0 ) );
}

//-----------------------------------------------------------------------------
bool C_PasstimePlayerReticle::Update()
{
	if ( !g_pPasstimeLogic ) 
	{
		return false;
	}

	auto *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	auto *pPlayer = m_hPlayer.Get();
	if ( !pLocalPlayer || pLocalPlayer->IsPlayerDead() 
		|| !pPlayer || pPlayer->IsPlayerDead() )
	{
		return false;
	}

	if ( pPlayer->m_Shared.GetPercentInvisible() > 0 ) 
	{
		// dont' show because player is invisible, friend or foe
		return false;
	}

	auto iFriendsDetail = tf_passtime_player_reticles_friends.GetInt();
	auto iEnemiesDetail = tf_passtime_player_reticles_enemies.GetInt();

	auto bIsDisguisedEnemy = pPlayer->m_Shared.InCond( TF_COND_DISGUISED ) 
		&& ( pPlayer->m_Shared.GetDisguiseTeam() == pLocalPlayer->GetTeamNumber() )
		&& !pPlayer->m_Shared.IsFullyInvisible();

	auto bIsFriend = pLocalPlayer->InSameTeam( pPlayer ) || bIsDisguisedEnemy;

	if ( (bIsFriend && (iFriendsDetail <= 0)) 
		|| (!bIsFriend && (iEnemiesDetail <= 0)) )
	{
		// don't show because disabled
		return false;
	}

	if ( !pLocalPlayer->m_Shared.HasPasstimeBall()
		&& ((bIsFriend && (iFriendsDetail == 1)) 
			|| (!bIsFriend && (iEnemiesDetail == 1))))
	{
		// don't show because not visible unless have ball
		return false;
	}

	if ( pPlayer->IsDormant() )
	{
		// don't show because not getting updated from server for some reason
		// probably not in PVS
		return false;
	}

	auto nTeamNumber = pPlayer->GetTeamNumber();
	if ( !pLocalPlayer->InSameTeam( pPlayer ) && bIsFriend )	// they're not on my team but they're showing as a friend, they must be a spy so use my team color
	{
		nTeamNumber = pLocalPlayer->GetTeamNumber();
	}

	int iShowSprite, iHideSprite;
	if ( nTeamNumber == TF_TEAM_RED )
	{
		iShowSprite = 0;
		iHideSprite = 1;
	}
	else if ( nTeamNumber == TF_TEAM_BLUE )
	{
		iShowSprite = 1;
		iHideSprite = 0;
	}
	else
	{
		return false;
	}

	auto vecTarget = pPlayer->EyePosition();
	int iX, iY;
	auto bOnScreen = GetVectorInHudSpace( vecTarget, iX, iY );
	if ( !bOnScreen )
	{
		return false;
	}

	trace_t	tr;
	CTraceFilterIgnorePlayers tracefilter( pLocalPlayer, COLLISION_GROUP_PROJECTILE );
	UTIL_TraceLine( MainViewOrigin(), vecTarget, MASK_PLAYERSOLID, &tracefilter, &tr );
	if ( tr.fraction == 1 )
	{
		// made it all the way, the guy is visible so hide the icon
		return false;
	}

	auto flDist = (vecTarget - MainViewOrigin()).Length();
	auto flScale = RemapValClamped( flDist, 1024.0f, 4096.0f, 80, 128 );

	SetAlpha( iHideSprite, 0 );
	SetAlpha( iShowSprite, 100 );
	SetAllScales( flScale );
	SetAllOrigins( vecTarget );
	SetAllNormals( -MainViewForward() );
	return true;
}

//-----------------------------------------------------------------------------
// C_PasstimeAskForBallReticle
//-----------------------------------------------------------------------------
C_PasstimeAskForBallReticle::C_PasstimeAskForBallReticle( C_TFPlayer *pPlayer )
{
	m_hPlayer.Set( pPlayer );
	AddSprite( CreateReticleSprite( "passtime/hud/passtime_pass_to_me_prompt", 128, 0 ) );
	SetRgba( 0, 255, 255, 255, 200 );
}

//-----------------------------------------------------------------------------
bool C_PasstimeAskForBallReticle::Update()
{
	if ( !g_pPasstimeLogic ) 
	{
		return false;
	}

	auto *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	auto *pPlayer = m_hPlayer.Get();

	if ( !pLocalPlayer || !pPlayer )
	{
		return false;
	}

	auto bLocalPlayerObserver = pLocalPlayer->IsObserver();
	if ( !bLocalPlayerObserver && pLocalPlayer->IsPlayerDead() )
	{
		return false;
	}

	if ( (pPlayer->m_Shared.AskForBallTime() < gpGlobals->curtime) || pPlayer->IsPlayerDead() )
	{
		return false;
	}

	if ( !bLocalPlayerObserver && !pLocalPlayer->m_Shared.HasPasstimeBall() && !pPlayer->InSameTeam( pLocalPlayer ) )
	{
		return false;
	}

	auto vecTarget = pPlayer->EyePosition();
	vecTarget.z += 16;
	int iX, iY;
	auto bOnScreen = GetVectorInHudSpace( vecTarget, iX, iY );
	if ( !bOnScreen )
	{
		return false;
	}

	auto flDist = (vecTarget - MainViewOrigin()).Length();
	auto flScale = RemapValClamped( flDist, 1024.0f, 4096.0f, 40, 200 );
	SetAllScales( flScale );
	SetAllOrigins( vecTarget );
	SetAllNormals( -MainViewForward() );
	SetRgba( 0, 255, 255, 255, (((int)(gpGlobals->curtime * 10)) & 1 ? 200 : 0) );
	return true;
}

//-----------------------------------------------------------------------------
// Functions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
CFXQuad *CreateReticleSprite( const char *pModelName, float flScale, float flSpinSpeed )
{
	FXQuadData_t q;
	memset(&q, 0, sizeof(q));
	q.m_Color.Init(1,1,1);
	q.m_flDeltaYaw = flSpinSpeed;
	q.m_flDieTime = FLT_MAX;
	q.m_flEndAlpha = 1;
	q.m_flEndScale = flScale;
	q.m_flLifeTime = 0;
	q.m_flScaleBias = 0;
	q.m_flStartAlpha = 1;
	q.m_flStartScale = flScale;
	q.m_flYaw = 180;
	q.SetMaterial( pModelName );
	q.m_uiFlags = 0;
	q.m_vecNormal.Init(1,0,0);
	q.m_vecOrigin.Init(0,0,0);
	CFXQuad *pQuad = new CFXQuad(q);
	clienteffects->AddEffect( pQuad );
	return pQuad;
}
