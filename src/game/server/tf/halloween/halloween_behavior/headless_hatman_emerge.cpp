//========= Copyright Valve Corporation, All rights reserved. ============//
// headless_hatman_emerge.cpp
// The Halloween Boss emerging from the ground
// Michael Booth, October 2010

#include "cbase.h"
#include "tf_player.h"
#include "tf_gamerules.h"
#include "tf_team.h"
#include "nav_mesh/tf_nav_area.h"
#include "particle_parse.h"

#include "../headless_hatman.h"
#include "headless_hatman_emerge.h"
#include "headless_hatman_attack.h"
#include "crybaby_boss_escape.h"


//---------------------------------------------------------------------------------------------
ActionResult< CHeadlessHatman >	CHeadlessHatmanEmerge::OnStart( CHeadlessHatman *me, Action< CHeadlessHatman > *priorAction )
{
	me->GetBodyInterface()->StartActivity( ACT_TRANSITION );

	DispatchParticleEffect( "halloween_boss_summon", me->GetAbsOrigin(), me->GetAbsAngles() );

	m_riseTimer.Start( 3.0f );
	m_emergePos = me->GetAbsOrigin() + Vector( 0, 0, 10.0f );

	m_height = 200.0f;
	me->SetAbsOrigin( m_emergePos + Vector( 0, 0, -m_height ) );
	me->EmitSound( "Halloween.HeadlessBossSpawnRumble" );

	// face towards a nearby player
	CUtlVector< CTFPlayer * > playerVector;
	CollectPlayers( &playerVector, TF_TEAM_RED, COLLECT_ONLY_LIVING_PLAYERS );
	CollectPlayers( &playerVector, TF_TEAM_BLUE, COLLECT_ONLY_LIVING_PLAYERS, APPEND_PLAYERS );

	float closeRangeSq = FLT_MAX;
	CTFPlayer *close = NULL;

	for( int i=0; i<playerVector.Count(); ++i )
	{
		CTFPlayer *player = playerVector[i];

		float rangeSq = me->GetRangeSquaredTo( player );
		if ( rangeSq < closeRangeSq )
		{
			closeRangeSq = rangeSq;
			close = player;
		}
	}

	QAngle facingAngle;

	if ( close )
	{
		Vector toPlayer = close->GetAbsOrigin() - me->GetAbsOrigin();
		toPlayer.z = 0.0f;
		toPlayer.NormalizeInPlace();

		VectorAngles( toPlayer, Vector(0,0,1), facingAngle );
	}
	else
	{
		facingAngle.x = 0.0f;
		facingAngle.y = RandomFloat( 0.0f, 360.0f );
		facingAngle.z = 0.0f;
	}

	me->SetAbsAngles( facingAngle );


	IGameEvent *event = gameeventmanager->CreateEvent( "pumpkin_lord_summoned" );
	if ( event )
	{
		gameeventmanager->FireEvent( event );
	}

	return Continue();
}


//----------------------------------------------------------------------------------
ActionResult< CHeadlessHatman >	CHeadlessHatmanEmerge::Update( CHeadlessHatman *me, float interval )
{
	if ( !m_riseTimer.IsElapsed() )
	{
		me->SetAbsOrigin( m_emergePos + Vector( 0, 0, -m_height * m_riseTimer.GetRemainingTime() / m_riseTimer.GetCountdownDuration() ) );

		if ( m_rumbleTimer.IsElapsed() )
		{
			m_rumbleTimer.Start( 0.25f );

			// shake nearby players' screens.
			UTIL_ScreenShake( me->GetAbsOrigin(), 15.0f, 5.0f, 1.0f, 1000.0f, SHAKE_START );
		}
	}

	if ( me->IsActivityFinished() )
	{
		return ChangeTo( new CHeadlessHatmanAttack, "Here I come!" );
	}

	// push players away to avoid penetration issues
	CUtlVector< CTFPlayer * > playerVector;
	CollectPlayers( &playerVector, TF_TEAM_RED, COLLECT_ONLY_LIVING_PLAYERS );
	CollectPlayers( &playerVector, TF_TEAM_BLUE, COLLECT_ONLY_LIVING_PLAYERS, APPEND_PLAYERS );

	const float pushRange = 250.0f;
	const float pushForce = 200.0f;

	for( int i=0; i<playerVector.Count(); ++i )
	{
		CTFPlayer *player = playerVector[i];

		Vector toPlayer = player->EyePosition() - m_emergePos;
		float range = toPlayer.NormalizeInPlace();

		if ( range < pushRange )
		{
			// make sure we push players up and away
			toPlayer.z = 0.0f;
			toPlayer.NormalizeInPlace();
			toPlayer.z = 1.0f;

			Vector push = pushForce * toPlayer;

			player->ApplyAbsVelocityImpulse( push );
		}
	}

	return Continue();
}
