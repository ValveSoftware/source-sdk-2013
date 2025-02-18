//========= Copyright Valve Corporation, All rights reserved. ============//
// boss_alpha_launch_rockets.cpp
// Michael Booth, November 2010

#include "cbase.h"

#ifdef TF_RAID_MODE

#include "tf_projectile_rocket.h"
#include "player_vs_environment/boss_alpha/boss_alpha.h"
#include "player_vs_environment/boss_alpha/behavior/boss_alpha_launch_rockets.h"

ConVar tf_boss_alpha_dont_shoot( "tf_boss_alpha_dont_shoot", "0"/*, FCVAR_CHEAT*/ );


//---------------------------------------------------------------------------------------------
ActionResult< CBossAlpha >	CBossAlphaLaunchRockets::OnStart( CBossAlpha *me, Action< CBossAlpha > *priorAction )
{
	// start animation
	me->GetBodyInterface()->StartActivity( ACT_MP_STAND_SECONDARY );

	m_animLayer = me->AddLayeredSequence( me->LookupSequence( "taunt02" ), 0 );

	m_timer.Start( 1.0f );

	m_rocketsLeft = me->GetRocketLaunchCount();

	me->AddCondition( CBossAlpha::BUSY );
	me->LockAttackTarget();

	me->EmitSound( "RobotBoss.LaunchRockets" );

	if ( me->GetAttackTarget() == NULL )
	{
		return Done( "No target" );
	}

	m_target = me->GetAttackTarget();
	m_lastTargetPosition = m_target->WorldSpaceCenter();

	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CBossAlpha >	CBossAlphaLaunchRockets::Update( CBossAlpha *me, float interval )
{
	if ( m_target != NULL )
	{
		m_lastTargetPosition = m_target->WorldSpaceCenter();
	}

	me->GetLocomotionInterface()->FaceTowards( m_lastTargetPosition );

	if ( m_timer.IsElapsed() && m_launchTimer.IsElapsed() )
	{
		if ( !m_rocketsLeft )
		{
			return Done();
		}

		--m_rocketsLeft;
		m_launchTimer.Start( me->GetRocketInterval() );

		QAngle launchAngles = me->GetAbsAngles();

		if ( m_target == NULL )
		{
			Vector to = m_lastTargetPosition - me->WorldSpaceCenter();
			VectorAngles( to, launchAngles );
		}
		else
		{
			float range = me->GetRangeTo( m_target->EyePosition() );

			const float rocketSpeed = me->GetRocketAimError() * 1100.0f; // 2000.0f; // 1100.0f;  nerfing accuracy
			float flightTime = range / rocketSpeed;

			Vector aimSpot = m_target->EyePosition() + m_target->GetAbsVelocity() * flightTime;

			Vector to = aimSpot - me->WorldSpaceCenter();
			VectorAngles( to, launchAngles );
		}

		if ( !tf_boss_alpha_dont_shoot.GetBool() )
		{
			CTFProjectile_Rocket *pRocket = CTFProjectile_Rocket::Create( me, me->WorldSpaceCenter(), launchAngles, me, me );
			if ( pRocket )
			{
				if ( me->IsInCondition( CBossAlpha::ENRAGED ) )
				{
					pRocket->SetCritical( true );
					pRocket->EmitSound( "Weapon_RPG.SingleCrit" );
				}
				else
				{
					me->EmitSound( me->GetRocketSoundEffect() );
				}

				pRocket->SetDamage( me->GetRocketDamage() );
			}
		}
	}

	return Continue();
}


//---------------------------------------------------------------------------------------------
void CBossAlphaLaunchRockets::OnEnd( CBossAlpha *me, Action< CBossAlpha > *nextAction )
{
	me->RemoveCondition( CBossAlpha::ENRAGED );
	me->RemoveCondition( CBossAlpha::BUSY );
	me->FastRemoveLayer( m_animLayer );
	me->UnlockAttackTarget();
}


#endif // TF_RAID_MODE
