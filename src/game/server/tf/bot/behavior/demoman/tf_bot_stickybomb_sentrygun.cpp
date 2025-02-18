//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_bot_stickybomb_sentrygun.cpp
// Destroy the given sentrygun with stickybombs
// Michael Booth, August 2010

#include "cbase.h"
#include "tf_player.h"
#include "bot/tf_bot.h"
#include "bot/behavior/demoman/tf_bot_stickybomb_sentrygun.h"
#include "tf_weapon_pipebomblauncher.h"
#include "tf_obj_sentrygun.h"
#include "NextBotUtil.h"

ConVar tf_bot_sticky_base_range( "tf_bot_sticky_base_range", "800", FCVAR_CHEAT );
ConVar tf_bot_sticky_charge_rate( "tf_bot_sticky_charge_rate", "0.01", FCVAR_CHEAT, "Seconds of charge per unit range beyond base" );


//---------------------------------------------------------------------------------------------
CTFBotStickybombSentrygun::CTFBotStickybombSentrygun( CObjectSentrygun *sentrygun )
{
	m_sentrygun = sentrygun;
	m_hasGivenAim = false;
}


//---------------------------------------------------------------------------------------------
CTFBotStickybombSentrygun::CTFBotStickybombSentrygun( CObjectSentrygun *sentrygun, float aimYaw, float aimPitch, float aimCharge )
{
	m_sentrygun = sentrygun;
	m_hasGivenAim = true;
	m_givenYaw = aimYaw;
	m_givenPitch = aimPitch;
	m_givenCharge = aimCharge;
}


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot >	CTFBotStickybombSentrygun::OnStart( CTFBot *me, Action< CTFBot > *priorAction )
{
	// detonate old set of stickies
	me->PressAltFireButton();

	// own our view updating so we can aim
	me->StopLookingAroundForEnemies();

	m_isFullReloadNeeded = true;

	// STOP
	me->SetAbsVelocity( vec3_origin );

	m_searchPitch = 0.0f;
	m_hasTarget = false;
	m_searchTimer.Start( 3.0f );

	m_isChargingShot = false;

	if ( m_hasGivenAim )
	{
		m_hasTarget = true;

		// remember where we are standing - if we move for any reason, we'll need to re-search
		m_launchSpot = me->GetAbsOrigin();

		// start charging up the sticky launch
		m_chargeToLaunch = m_givenCharge;
		m_isChargingShot = true;

		// aim along given pitch/yaw
		QAngle angles;
		angles.x = m_givenPitch;
		angles.y = m_givenYaw;
		angles.z = 0.0f;

		Vector aimForward;
		AngleVectors( angles, &aimForward );

		m_eyeAimTarget = me->EyePosition() + 1500.0f * aimForward;
	}

	return Continue();
}


//---------------------------------------------------------------------------------------------
bool CTFBotStickybombSentrygun::IsAimOnTarget( CTFBot *me, float pitch, float yaw, float charge )
{
	// estimate impact spot
	Vector impactSpot = me->EstimateStickybombProjectileImpactPosition( pitch, yaw, charge );

	// check if impactSpot landed near sentry
	const float explosionRadius = 75.0f;
	if ( ( m_sentrygun->WorldSpaceCenter() - impactSpot ).IsLengthLessThan( explosionRadius ) )
	{
		trace_t trace;
		NextBotTraceFilterIgnoreActors filter( NULL, COLLISION_GROUP_NONE );

		UTIL_TraceLine( m_sentrygun->WorldSpaceCenter(), impactSpot, MASK_SOLID_BRUSHONLY, &filter, &trace );
		if ( !trace.DidHit() )
		{
			// NDebugOverlay::Cross3D( impactSpot, 10.0f, 100, 255, 0, true, 60.0f );
			return true;
		}
	}

	return false;
}


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot >	CTFBotStickybombSentrygun::Update( CTFBot *me, float interval )
{
	CTFWeaponBase *myCurrentWeapon = me->m_Shared.GetActiveTFWeapon();
	CTFPipebombLauncher *stickyLauncher = dynamic_cast< CTFPipebombLauncher * >( me->Weapon_GetSlot( TF_WPN_TYPE_SECONDARY ) );

	if ( !myCurrentWeapon || !stickyLauncher )
	{
		return Done( "Missing weapon" );
	}

	if ( myCurrentWeapon->GetWeaponID() != TF_WEAPON_PIPEBOMBLAUNCHER )
	{
		me->Weapon_Switch( stickyLauncher );
	}

	if ( m_sentrygun == NULL || !m_sentrygun->IsAlive() )
	{
		return Done( "Sentry destroyed" );
	}

	if ( !m_hasTarget && m_searchTimer.IsElapsed() )
	{
		return Done( "Can't find aim" );
	}

	// reload fully
	if ( m_isFullReloadNeeded )
	{
		int maxClip = MIN( stickyLauncher->GetMaxClip1(), me->GetAmmoCount( TF_AMMO_SECONDARY ) );

		if ( stickyLauncher->Clip1() >= maxClip )
		{
			// fully reloaded
			m_isFullReloadNeeded = false;
		}

		me->PressReloadButton();

		return Continue();
	}

	int requiredStickyBombs = 3;

	if ( TFGameRules()->IsMannVsMachineMode() )
	{
		// launch more stickies to make sure we take out beefed-up sentries
		requiredStickyBombs = 5;
	}

	if ( stickyLauncher->GetPipeBombCount() >= requiredStickyBombs || me->GetAmmoCount( TF_AMMO_SECONDARY ) <= 0 )
	{
		// stickies laid - detonate them once they are on the ground
		const CUtlVector< CHandle< CTFGrenadePipebombProjectile > > &pipeVector = stickyLauncher->GetPipeBombVector();

		int i;
		for( i=0; i<pipeVector.Count(); ++i )
		{
			if ( pipeVector[i].Get() && !pipeVector[i]->m_bTouched )
			{
				break;
			}
		}

		if ( i == pipeVector.Count() )
		{
			// stickies are on the ground
			me->PressAltFireButton();

			if ( me->GetAmmoCount( TF_AMMO_SECONDARY ) <= 0 )
			{
				return Done( "Out of ammo" );
			}
		}
	}
	else if ( m_isChargingShot )
	{
		// fudge charge time a bit longer - better to overshoot
		float stickyChargeTime = 1.1f * m_chargeToLaunch * TF_PIPEBOMB_MAX_CHARGE_TIME;

		me->GetBodyInterface()->AimHeadTowards( m_eyeAimTarget, IBody::CRITICAL, 0.3f, NULL, "Aiming a sticky bomb at a sentrygun" );

		if ( gpGlobals->curtime - stickyLauncher->GetChargeBeginTime() >= stickyChargeTime )
		{
			// let go
			me->ReleaseFireButton();
			m_isChargingShot = false;
		}
		else
		{
			me->PressFireButton();
		}
	}
	else if ( stickyLauncher->m_flNextPrimaryAttack < gpGlobals->curtime )
	{
		// if we've moved, we need to re-search
		if ( m_hasTarget )
		{
			const float tolerance = 1.0f;
			if ( me->IsRangeGreaterThan( m_launchSpot, tolerance ) )
			{
				m_hasTarget = false;
				m_searchTimer.Reset();
			}
		}

		if ( !m_hasTarget )
		{
			// search for angle to land sticky near sentry
			Vector toSentry = m_sentrygun->WorldSpaceCenter() - me->EyePosition();
			
			QAngle angles;
			VectorAngles( toSentry, angles );

			float bestYaw = 0.0f;
			float bestPitch = 0.0f;
			float bestCharge = 1.0f;

			const int trials = 100;
			for( int t=0; t<trials; ++t )
			{
				float yaw = angles.y + RandomFloat( -30.0f, 30.0f );
				// float pitch = ( trials & 0x1 ) ? m_searchPitch : -m_searchPitch;
				float pitch = RandomFloat( -85.0f, 85.0f );

				float charge = 0.0f;
				if ( toSentry.IsLengthGreaterThan( tf_bot_sticky_base_range.GetBool() ) )
				{
					charge = RandomFloat( 0.1f, 1.0f );

					// skew towards zero - full charge shots are seldom required
					charge *= charge;
				}

				if ( IsAimOnTarget( me, pitch, yaw, charge ) )
				{
					// found target aim - keep one we find with least required
					// charge, because we need to be fast in combat
					if ( charge < bestCharge )
					{
						m_hasTarget = true;

						bestCharge = charge;
						m_chargeToLaunch = bestCharge;

						bestYaw = yaw;
						bestPitch = pitch;

						if ( bestCharge < 0.01 )
						{
							// as quick as possible - no need to search further
							break;
						}
					}
				}
			}

			// aim along yaw/pitch to reach impact spot
			angles.x = bestPitch;
			angles.y = bestYaw;
			angles.z = 0.0f;

			Vector aimForward;
			AngleVectors( angles, &aimForward );

			// always recompute eye aim target so we can update our view
			m_eyeAimTarget = me->EyePosition() + 500.0f * aimForward;
			me->GetBodyInterface()->AimHeadTowards( m_eyeAimTarget, IBody::CRITICAL, 0.3f, NULL, "Searching for aim..." );
		}

		if ( m_hasTarget )
		{
			// remember where we are standing - if we move for any reason, we'll need to re-search
			m_launchSpot = me->GetAbsOrigin();

			// start charging up the sticky launch
			me->PressFireButton();
			m_isChargingShot = true;
		}
	}

	return Continue();
}


//---------------------------------------------------------------------------------------------
void CTFBotStickybombSentrygun::OnEnd( CTFBot *me, Action< CTFBot > *nextAction )
{
	// detonate any stickes left out there
	me->PressAltFireButton();

	me->StartLookingAroundForEnemies();
}


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot > CTFBotStickybombSentrygun::OnSuspend( CTFBot *me, Action< CTFBot > *interruptingAction )
{
	// detonate any stickes left out there
	me->PressAltFireButton();

	// this behavior is transitory - if we need to do something else, just give up
	return Done();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CTFBot > CTFBotStickybombSentrygun::OnInjured( CTFBot *me, const CTakeDamageInfo &info )
{
	return TryDone( RESULT_IMPORTANT, "Ouch!" );
}


//---------------------------------------------------------------------------------------------
QueryResultType CTFBotStickybombSentrygun::ShouldAttack( const INextBot *me, const CKnownEntity *them ) const
{
	return ANSWER_NO;
}


//---------------------------------------------------------------------------------------------
QueryResultType CTFBotStickybombSentrygun::ShouldHurry( const INextBot *me ) const
{
	// while killing a sentry we're "hurrying" so we don't dodge
	return ANSWER_YES;
}


//---------------------------------------------------------------------------------------------
QueryResultType CTFBotStickybombSentrygun::ShouldRetreat( const INextBot *me ) const
{
	// stay stuck in to try to kill that gun!
	return ANSWER_NO;
}
