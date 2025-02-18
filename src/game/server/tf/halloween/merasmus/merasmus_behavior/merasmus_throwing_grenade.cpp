//========= Copyright Valve Corporation, All rights reserved. ============//
//
//
//
//=============================================================================
#include "cbase.h"

#include "tf_player.h"
#include "tf_gamerules.h"
#include "particle_parse.h"

#include "../merasmus.h"
#include "merasmus_throwing_grenade.h"
#include "merasmus_stunned.h"

CMerasmusThrowingGrenade::CMerasmusThrowingGrenade( CTFPlayer* pTarget )
{
	m_hTarget = pTarget;
}


ActionResult< CMerasmus > CMerasmusThrowingGrenade::OnStart( CMerasmus *me, Action< CMerasmus > *priorAction )
{
	if ( m_hTarget == NULL )
	{
		return Done( "No Target" );
	}

	if ( !me->IsLineOfSightClear( m_hTarget ) )
	{
		CUtlVector< CTFPlayer * > playerVector;

		// collect everyone
		CollectPlayers( &playerVector, TF_TEAM_RED, COLLECT_ONLY_LIVING_PLAYERS );
		CollectPlayers( &playerVector, TF_TEAM_BLUE, COLLECT_ONLY_LIVING_PLAYERS, APPEND_PLAYERS );

		CUtlVector< CTFPlayer * > newTargetVector;
		for ( int i=0; i<playerVector.Count(); ++i )
		{
			if ( playerVector[i] == m_hTarget )
			{
				continue;
			}

			if ( !me->IsLineOfSightClear( playerVector[i] ) )
			{
				continue;
			}

			newTargetVector.AddToTail( playerVector[i] );
		}

		if ( newTargetVector.Count() == 0 )
		{
			m_hTarget = NULL;
		}
		else
		{
			int which = RandomInt( 0, newTargetVector.Count() - 1 );
			m_hTarget = newTargetVector[ which ];
		}
	}

	if ( m_hTarget == NULL )
	{
		return Done( "No Target" );
	}

	me->GetLocomotionInterface()->FaceTowards( m_hTarget->WorldSpaceCenter() );
	int iLayer = me->AddGesture( ACT_MP_ATTACK_STAND_ITEM1 );
	float flDuration = me->GetLayerDuration( iLayer );
	m_throwTimer.Start( flDuration );

	// we want to release the grenade mid-animation
	m_releaseGrenadeTimer.Start( 0.25f );

	// hide his staff
	int staffBodyGroup = me->FindBodygroupByName( "staff" );
	me->SetBodygroup( staffBodyGroup, 2 );

	// smooth out the bot's path following by moving toward a point farther down the path
	m_path.SetMinLookAheadDistance( 100.0f );

	return Continue();
}


ActionResult< CMerasmus > CMerasmusThrowingGrenade::Update( CMerasmus *me, float interval )
{
	// Interupt if stunned
	if ( me->HasStunTimer() )
	{
		return ChangeTo( new CMerasmusStunned, "Stun Interupt!" );
	}

	if ( m_releaseGrenadeTimer.HasStarted() && m_releaseGrenadeTimer.IsElapsed() )
	{
		m_releaseGrenadeTimer.Invalidate();

		DispatchParticleEffect( "merasmus_shoot", PATTACH_ABSORIGIN_FOLLOW, me, "effect_hand_R" );

		Vector vPos;
		QAngle qAngles;
		me->GetAttachment( "effect_hand_R", vPos, qAngles );

		Vector vForward, vRight, vUp;
		AngleVectors( me->EyeAngles(), &vForward, &vRight, &vUp );
		float flLaunchSpeed = RandomFloat( 1500.f, 2000.f );
		Vector vecVelocity = ( vForward * flLaunchSpeed ) + ( vUp * 200.0f ) + ( RandomFloat( -10.0f, 10.0f ) * vRight ) +	( RandomFloat( -10.0f, 10.0f ) * vUp );
		CTFWeaponBaseGrenadeProj* pGrenade = CMerasmus::CreateMerasmusGrenade( vPos, vecVelocity, me );
		if ( pGrenade )
		{
			if ( RandomInt( 0, 6 ) == 0 )
			{
				CPVSFilter filter( me->WorldSpaceCenter() );
				if ( RandomInt(1,10) == 1 )
				{
					me->PlayLowPrioritySound( filter, "Halloween.MerasmusGrenadeThrowRare" );
				}
				else
				{
					me->PlayLowPrioritySound( filter, "Halloween.MerasmusGrenadeThrow" );
				}
			}
		}
	}

	if ( m_hTarget )
	{
		if ( me->IsRangeGreaterThan( m_hTarget, 100.f ) || !me->IsLineOfSightClear( m_hTarget ) )
		{
			if ( m_path.GetAge() > 1.0f )
			{
				CMerasmusPathCost cost( me );
				m_path.Compute( me, m_hTarget, cost );
			}

			m_path.Update( me );
		}

		me->GetLocomotionInterface()->FaceTowards( m_hTarget->WorldSpaceCenter() );
	}


	if ( m_throwTimer.IsElapsed() )
	{
		return Done( "Fire in the hole!" );
	}

	return Continue();
}


void CMerasmusThrowingGrenade::OnEnd( CMerasmus *me, Action< CMerasmus > *nextAction )
{
	// turn his staff back on
	int staffBodyGroup = me->FindBodygroupByName( "staff" );
	me->SetBodygroup( staffBodyGroup, 0 );
}
