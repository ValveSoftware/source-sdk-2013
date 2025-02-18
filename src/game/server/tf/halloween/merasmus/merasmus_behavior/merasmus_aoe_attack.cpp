//========= Copyright Valve Corporation, All rights reserved. ============//
//
//
//
//=============================================================================
#include "cbase.h"

#include "tf_gamerules.h"
#include "particle_parse.h"
#include "nav_mesh/tf_nav_area.h"
#include "nav_mesh/tf_nav_mesh.h"
#include "tf/halloween/eyeball_boss/teleport_vortex.h"
#include "tf_weaponbase_grenadeproj.h"
#include "sceneentity.h"

#include "../merasmus.h"
#include "merasmus_aoe_attack.h"

//---------------------------------------------------------------------------------------------
#define MAX_BOMBS_PER_TICK		4

//---------------------------------------------------------------------------------------------
ActionResult< CMerasmus >	CMerasmusAOEAttack::OnStart( CMerasmus *me, Action< CMerasmus > *priorAction )
{
	m_aoeStartTimer.Start( 4.f );
	m_state = AOE_BEGIN;

 	me->StartFlying();

	CTFNavArea *pointArea = TheTFNavMesh()->GetControlPointCenterArea( 0 );

	if ( !pointArea )
	{
		return Done( "No control point!" );
	}

	const float surroundRange = 400.f;
	CollectSurroundingAreas( &m_wanderAreaVector, pointArea, surroundRange, StepHeight, StepHeight );

	if ( m_wanderAreaVector.Count() == 0 )
	{
		return Done( "No nav areas near control point!" );
	}

	me->GetBodyInterface()->StartActivity( ACT_RANGE_ATTACK1 );
	
	// This is only to play sound since animation doesn't work with the vcd
	CFmtStr vcdName( "scenes/bot/merasmus/low/bomb_attack_00%d.vcd", RandomInt( 1, 9 ) );
	InstancedScriptedScene( me, vcdName.Get(), NULL, 0.0f, false, NULL, true );

	m_wanderArea = NULL;

	return Continue();
}


#define BOMB_SCALE 1.0f // 0.85f
#define BOMB_VERT_VEL 750.0f
#define BOMB_START_OFFSET	Vector( 0.0f, 0.0f, 150.0f )

//---------------------------------------------------------------------------------------------
void CMerasmusAOEAttack::QueueSingleGrenadeForLaunch( const Vector &vecVelocity )
{
	m_vecGrenadesToCreate.AddToTail( MerasmusGrenadeCreateSpec_t( vecVelocity ) );
}

//---------------------------------------------------------------------------------------------
void CMerasmusAOEAttack::ClearPendingGrenades()
{
	m_vecGrenadesToCreate.RemoveAll();
}

//---------------------------------------------------------------------------------------------
void CMerasmusAOEAttack::LaunchPendingGrenades( CMerasmus *me )
{
	int nNumGrenadesLeftToCreate = MIN( m_vecGrenadesToCreate.Count(), MAX_BOMBS_PER_TICK );
	while ( nNumGrenadesLeftToCreate > 0 )
	{
		// Create the first one in the list
		MerasmusGrenadeCreateSpec_t &info = m_vecGrenadesToCreate[0];
		CMerasmus::CreateMerasmusGrenade( me->WorldSpaceCenter() + BOMB_START_OFFSET, info.m_vecVelocity, me, BOMB_SCALE );

		// Remove the first one in the list
		m_vecGrenadesToCreate.Remove( 0 );

		--nNumGrenadesLeftToCreate;
	}
}

//---------------------------------------------------------------------------------------------
void CMerasmusAOEAttack::QueueBombRingsForLaunch( CMerasmus *me )
{
	ClearPendingGrenades();

	const float bombRingMinHorizVel = 100.0f;
	const float bombRingMaxHorizVel = 2000.0f;

	QAngle myAngles = me->EyeAngles();

	float deltaVel = bombRingMaxHorizVel - bombRingMinHorizVel;
	const int ringCount = 2;

	for( int r=0; r<ringCount; ++r )
	{
		float u = (float)(r+1)/(float)ringCount;

		float horizVel = bombRingMinHorizVel + u * deltaVel;

// 		float angleDelta = 10.0f + 20.0f * ( 1.0f - u );
		float angleDelta = 20.0f + 30.0f * ( 1.0f - u );

		for( float angle=0.0f; angle<360.0f; angle += angleDelta )
		{
			Vector forward;
			AngleVectors( myAngles, &forward );

			Vector vecVelocity( horizVel * forward.x, horizVel * forward.y, BOMB_VERT_VEL );

			QueueSingleGrenadeForLaunch( vecVelocity );

			myAngles.y += angleDelta;
		}
	}
}


//---------------------------------------------------------------------------------------------
void CMerasmusAOEAttack::QueueBombSpokesForLaunch( CMerasmus *me )
{
	ClearPendingGrenades();

	const float bombSpokeAngle = 45.0f;
	const int bombSpokeCount = 4; // 10;
	const float bombSpokeMinHorizVel = 100.0f;
	const float bombSpokeMaxHorizVel = 2000.0f;

	float deltaVel = bombSpokeMaxHorizVel - bombSpokeMinHorizVel;
	float angleDelta = bombSpokeAngle;

	QAngle myAngles = me->EyeAngles();

	for( float angle=0.0f; angle<360.0f; angle += angleDelta )
	{
		Vector forward;
		AngleVectors( myAngles, &forward );

		int spokeCount = bombSpokeCount;

		for( int i=0; i<spokeCount; ++i )
		{
			float u = (float)(i+1)/(float)spokeCount;

			float horizVel = bombSpokeMinHorizVel + u * deltaVel;

			Vector vecVelocity( horizVel * forward.x, horizVel * forward.y, BOMB_VERT_VEL );

			QueueSingleGrenadeForLaunch( vecVelocity );
		}

		myAngles.y += angleDelta;
	}
}


//---------------------------------------------------------------------------------------------
ActionResult< CMerasmus >	CMerasmusAOEAttack::Update( CMerasmus *me, float interval )
{
	if ( me->IsActivityFinished() )
	{
		me->StopAOEAttack();

		return Done();
	}

	switch ( m_state )
	{
	case AOE_BEGIN:
		{
			if ( m_aoeStartTimer.IsElapsed() )
			{
				m_launchTimer.Start( 0.5f );
				
				m_state = AOE_FIRING;
			}
		}
		break;
	case AOE_FIRING:
		{
			// Start the AOE particles
			me->StartAOEAttack();
			
			CMerasmusFlyingLocomotion *fly = (CMerasmusFlyingLocomotion *)me->GetLocomotionInterface();

			// float up and down a bit
			fly->SetDesiredAltitude( 175.0f + 25.0f * FastCos( gpGlobals->curtime ) );

			if ( m_launchTimer.IsElapsed() )
			{
				if ( RandomInt( 1, 100 ) < 50 )
				{
					QueueBombSpokesForLaunch( me );
				}
				else
				{
					QueueBombRingsForLaunch( me );
				}

				m_launchTimer.Start( 2.0f );
			}

			// Go through and launch any pending grenades
			LaunchPendingGrenades( me );

			// wander among nav areas near the cap point
			if ( m_wanderTimer.IsElapsed() || m_wanderArea == NULL )
			{
				m_wanderTimer.Start( RandomFloat( 1.0f, 3.0f ) );

				m_wanderArea = (CTFNavArea *)m_wanderAreaVector[ RandomInt( 0, m_wanderAreaVector.Count()-1 ) ];
			}

			if ( m_wanderArea )
			{
				Vector flySpot = m_wanderArea->GetCenter();
				flySpot.z = me->GetAbsOrigin().z;

				me->GetLocomotionInterface()->Approach( flySpot );
				me->GetLocomotionInterface()->FaceTowards( flySpot );
			}
		}
		break;
	}

	return Continue();
}


//---------------------------------------------------------------------------------------------
void CMerasmusAOEAttack::OnEnd( CMerasmus *me, Action< CMerasmus > *nextAction )
{
 	me->StopFlying();

	// The animation sometime doesn't turn off the bodygroup correctly. Slam it in code.
	int staffBodyGroup = me->FindBodygroupByName( "staff" );
	me->SetBodygroup( staffBodyGroup, 0 );

	me->ResetBombHitCount();
}
