//========= Copyright Valve Corporation, All rights reserved. ============//
// eyeball_boss_launch_rockets.cpp
// The 2011 Halloween Boss
// Michael Booth, October 2011

#include "cbase.h"

#include "tf_projectile_rocket.h"
#include "tf_obj_sentrygun.h"

#include "../eyeball_boss.h"
#include "eyeball_boss_launch_rockets.h"


//---------------------------------------------------------------------------------------------
ActionResult< CEyeballBoss > CEyeballBossLaunchRockets::OnStart( CEyeballBoss *me, Action< CEyeballBoss > *priorAction )
{
	if ( me->GetVictim() == NULL )
	{
		return Done( "No target" );
	}

	m_lastTargetPosition = me->GetVictim()->GetAbsOrigin();

	const char *firingAnimation = NULL;

	if ( me->IsEnraged() )
	{
		m_rocketsLeft = 3;
		m_initialDelayTimer.Start( 0.25f );
		firingAnimation = "firing3";

		me->EmitSound( "Halloween.EyeballBossRage" );
	}
	else if ( me->IsGrumpy() )
	{
		m_rocketsLeft = 3;
		m_initialDelayTimer.Start( 0.25f );
		firingAnimation = "firing2";
	}
	else
	{
		m_rocketsLeft = 1;
		m_initialDelayTimer.Start( 0.5f );
		firingAnimation = "firing1";
	}

	int animSequence = me->LookupSequence( firingAnimation );
	if ( animSequence )
	{
		me->SetSequence( animSequence );
		me->SetPlaybackRate( 1.0f );
		me->SetCycle( 0 );
		me->ResetSequenceInfo();
	}

	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CEyeballBoss > CEyeballBossLaunchRockets::Update( CEyeballBoss *me, float interval )
{
	if ( me->GetVictim() && me->GetVictim()->IsAlive() )
	{
		m_lastTargetPosition = me->GetVictim()->GetAbsOrigin();

		// target upper part of sentry guns
		if ( me->GetVictim()->IsBaseObject() )
		{
			CObjectSentrygun *sentry = dynamic_cast< CObjectSentrygun * >( const_cast< CBaseCombatCharacter * >( me->GetVictim() ) );
			if ( sentry )
			{
				m_lastTargetPosition = sentry->EyePosition();
			}
		}

		// circle around them
		Vector forward, right;
		me->GetVectors( &forward, &right, NULL );

		me->GetLocomotionInterface()->SetDesiredSpeed( tf_eyeball_boss_speed.GetFloat() );

		if ( !me->IsSpell() )
		{
			if ( me->GetVictim()->entindex() & 0x1 )
			{
				me->GetLocomotionInterface()->Approach( 100.0f * right + me->WorldSpaceCenter() );
			}
			else
			{
				me->GetLocomotionInterface()->Approach( -100.0f * right + me->WorldSpaceCenter() );
			}
		}

		me->GetBodyInterface()->AimHeadTowards( me->GetVictim() );
	}
	else
	{
		me->GetBodyInterface()->AimHeadTowards( m_lastTargetPosition );
	}

	// fire rockets
	if ( m_initialDelayTimer.IsElapsed() && m_launchTimer.IsElapsed() )
	{
		--m_rocketsLeft;
		m_launchTimer.Start( 0.3f );

		const float rocketDamage = 50.0f;

		// if I'm enraged, shoot full speed rockets, otherwise shoot slow ones
		float speedFactor = me->IsEnraged() ? 1.0f : 0.3f;
		float rocketSpeed = 1100.0f * speedFactor;

		Vector targetSpot = m_lastTargetPosition;

		if ( me->IsEnraged() && me->GetVictim() )
		{
			// lead our target
			float rangeBetween = me->GetRangeTo( targetSpot );

			const float veryCloseRange = 150.0f;
			if ( rangeBetween > veryCloseRange )
			{
				float timeToTravel = rangeBetween / rocketSpeed;

				Vector leadOffset = timeToTravel * me->GetVictim()->GetAbsVelocity();

				CTraceFilterNoNPCsOrPlayer filter( me, COLLISION_GROUP_NONE );
				trace_t result;

				UTIL_TraceLine( me->WorldSpaceCenter(), me->GetVictim()->GetAbsOrigin() + leadOffset, MASK_SOLID_BRUSHONLY, &filter, &result );

				if ( result.DidHit() )
				{
					const float errorTolerance = 300.0f;
					if ( ( result.endpos - targetSpot ).IsLengthGreaterThan( errorTolerance ) )
					{
						// rocket is going to hit an obstruction not near our victim - just aim right for them and hope for splash
						leadOffset = vec3_origin;
					}
				}

				targetSpot = me->GetVictim()->GetAbsOrigin() + leadOffset;
			}
		}

		QAngle launchAngles;
		Vector toTarget = targetSpot - me->WorldSpaceCenter();
		VectorAngles( toTarget, launchAngles );

		CBaseEntity *pScorer = me->GetOwnerEntity() ? me->GetOwnerEntity() : me;

		CTFProjectile_Rocket *pRocket = CTFProjectile_Rocket::Create( me, me->WorldSpaceCenter(), launchAngles, me, pScorer );
		if ( pRocket )
		{
			pRocket->SetModel( "models/props_halloween/eyeball_projectile.mdl" );

			Vector forward;
			AngleVectors( launchAngles, &forward, NULL, NULL );

			Vector vecVelocity = forward * rocketSpeed;
			pRocket->SetAbsVelocity( vecVelocity );	
			pRocket->SetupInitialTransmittedGrenadeVelocity( vecVelocity );

			pRocket->EmitSound( "Weapon_RPG.SingleCrit" );
			pRocket->SetDamage( rocketDamage );
			pRocket->SetCritical( !me->IsSpell() );
			pRocket->SetEyeBallRocket( !me->IsSpell() );
			pRocket->SetSpell( me->IsSpell() );
			pRocket->ChangeTeam( me->GetTeamNumber() );
		}

		if ( !m_rocketsLeft )
		{
			return Done();
		}
	}

	return Continue();
}

