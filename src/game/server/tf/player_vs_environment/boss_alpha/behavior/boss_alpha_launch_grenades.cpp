//========= Copyright Valve Corporation, All rights reserved. ============//
// boss_alpha_launch_grenades.cpp
// Michael Booth, November 2010

#include "cbase.h"

#ifdef TF_RAID_MODE

#include "player_vs_environment/boss_alpha/boss_alpha.h"
#include "player_vs_environment/boss_alpha/behavior/boss_alpha_launch_grenades.h"

ConVar tf_boss_alpha_grenade_ring_min_horiz_vel( "tf_boss_alpha_grenade_ring_min_horiz_vel", "100"/*, FCVAR_CHEAT*/ );
ConVar tf_boss_alpha_grenade_ring_max_horiz_vel( "tf_boss_alpha_grenade_ring_max_horiz_vel", "350"/*, FCVAR_CHEAT*/ );
ConVar tf_boss_alpha_grenade_vert_vel( "tf_boss_alpha_grenade_vert_vel", "750"/*, FCVAR_CHEAT*/ );
ConVar tf_boss_alpha_grenade_det_time( "tf_boss_alpha_grenade_det_time", "3"/*, FCVAR_CHEAT*/ );
ConVar tf_boss_alpha_grenade_damage( "tf_boss_alpha_grenade_damage", "25"/*, FCVAR_CHEAT*/ );


//---------------------------------------------------------------------------------------------
ActionResult< CBossAlpha >	CBossAlphaLaunchGrenades::OnStart( CBossAlpha *me, Action< CBossAlpha > *priorAction )
{
	me->GetBodyInterface()->StartActivity( ACT_MP_STAND_SECONDARY );
	m_animLayer = me->AddLayeredSequence( me->LookupSequence( "gesture_melee_cheer" ), 0 );

	m_timer.Start( 1.0f );
	m_detonateTimer.Invalidate();
	me->AddCondition( CBossAlpha::BUSY );
	me->GetGrenadeTimer()->Start( me->GetGrenadeInterval() );

	me->EmitSound( "RobotBoss.LaunchGrenades" );

	return Continue();
}


//---------------------------------------------------------------------------------------------
void CBossAlphaLaunchGrenades::LaunchGrenade( CBossAlpha *me, const Vector &launchVel, CTFWeaponInfo *weaponInfo )
{
	CTFGrenadePipebombProjectile *pProjectile = CTFGrenadePipebombProjectile::Create( me->WorldSpaceCenter() + Vector( 0, 0, 100 ), vec3_angle, launchVel, 
																					  AngularImpulse( 600, random->RandomInt( -1200, 1200 ), 0 ), 
																					  me, *weaponInfo, TF_PROJECTILE_PIPEBOMB_REMOTE, 1 );
	if ( pProjectile )
	{
		pProjectile->SetLauncher( me );
		pProjectile->SetDamage( tf_boss_alpha_grenade_damage.GetFloat() );

		if ( me->IsInCondition( CBossAlpha::ENRAGED ) )
		{
			pProjectile->SetCritical( true );
		}

		m_grenadeVector.AddToTail( pProjectile );
	}
}


//---------------------------------------------------------------------------------------------
void CBossAlphaLaunchGrenades::LaunchGrenadeRings( CBossAlpha *me )
{
	const char *weaponAlias = WeaponIdToAlias( TF_WEAPON_GRENADELAUNCHER );
	if ( !weaponAlias )
		return;

	WEAPON_FILE_INFO_HANDLE	weaponInfoHandle = LookupWeaponInfoSlot( weaponAlias );
	if ( weaponInfoHandle == GetInvalidWeaponInfoHandle() )
		return;

	CTFWeaponInfo *weaponInfo = static_cast< CTFWeaponInfo * >( GetFileWeaponInfoFromHandle( weaponInfoHandle ) );

	QAngle myAngles = me->EyeAngles();

	// create rings of stickies
	float deltaVel = tf_boss_alpha_grenade_ring_max_horiz_vel.GetFloat() - tf_boss_alpha_grenade_ring_min_horiz_vel.GetFloat();
	const int ringCount = 2;
	for( int r=0; r<ringCount; ++r )
	{
		float u = (float)r/(float)(ringCount-1);

		float horizVel = tf_boss_alpha_grenade_ring_min_horiz_vel.GetFloat() + u * deltaVel;

		float angleDelta = 10.0f + 20.0f * ( 1.0f - u );

		for( float angle=0.0f; angle<360.0f; angle += angleDelta )
		{
			Vector forward;
			AngleVectors( myAngles, &forward );

			Vector vecVelocity( horizVel * forward.x, horizVel * forward.y, tf_boss_alpha_grenade_vert_vel.GetFloat() );

			LaunchGrenade( me, vecVelocity, weaponInfo );

			myAngles.y += angleDelta;
		}
	}
}


ConVar tf_boss_alpha_grenade_spoke_angle( "tf_boss_alpha_grenade_spoke_angle", "45"/*, FCVAR_CHEAT*/ );
ConVar tf_boss_alpha_grenade_spoke_count( "tf_boss_alpha_grenade_spoke_count", "15"/*, FCVAR_CHEAT*/ );
ConVar tf_boss_alpha_grenade_spoke_min_horiz_vel( "tf_boss_alpha_grenade_spoke_min_horiz_vel", "100"/*, FCVAR_CHEAT*/ );
ConVar tf_boss_alpha_grenade_spoke_max_horiz_vel( "tf_boss_alpha_grenade_spoke_max_horiz_vel", "750"/*, FCVAR_CHEAT*/ );


//---------------------------------------------------------------------------------------------
void CBossAlphaLaunchGrenades::LaunchGrenadeSpokes( CBossAlpha *me )
{
	const char *weaponAlias = WeaponIdToAlias( TF_WEAPON_GRENADELAUNCHER );
	if ( !weaponAlias )
		return;

	WEAPON_FILE_INFO_HANDLE	weaponInfoHandle = LookupWeaponInfoSlot( weaponAlias );
	if ( weaponInfoHandle == GetInvalidWeaponInfoHandle() )
		return;

	CTFWeaponInfo *weaponInfo = static_cast< CTFWeaponInfo * >( GetFileWeaponInfoFromHandle( weaponInfoHandle ) );

	// create spokes of stickies
	float deltaVel = tf_boss_alpha_grenade_spoke_max_horiz_vel.GetFloat() - tf_boss_alpha_grenade_spoke_min_horiz_vel.GetFloat();
	float angleDelta = tf_boss_alpha_grenade_spoke_angle.GetFloat();
	QAngle myAngles = me->EyeAngles();

	for( float angle=0.0f; angle<360.0f; angle += angleDelta )
	{
		Vector forward;
		AngleVectors( myAngles, &forward );

		int spokeCount = tf_boss_alpha_grenade_spoke_count.GetInt();

		for( int i=0; i<spokeCount; ++i )
		{
			float u = (float)i/(float)(spokeCount-1);

			float horizVel = tf_boss_alpha_grenade_spoke_min_horiz_vel.GetFloat() + u * deltaVel;

			Vector vecVelocity( horizVel * forward.x, horizVel * forward.y, tf_boss_alpha_grenade_vert_vel.GetFloat() );

			LaunchGrenade( me, vecVelocity, weaponInfo );
		}

		myAngles.y += angleDelta;
	}
}


//---------------------------------------------------------------------------------------------
ActionResult< CBossAlpha >	CBossAlphaLaunchGrenades::Update( CBossAlpha *me, float interval )
{
	QAngle myAngles = me->EyeAngles();

	if ( m_timer.HasStarted() && m_timer.IsElapsed() )
	{
		m_timer.Invalidate();

		if ( RandomInt( 0, 100 ) < 50 )
		{
			LaunchGrenadeRings( me );
		}
		else
		{
			LaunchGrenadeSpokes( me );
		}

		me->EmitSound( "Weapon_Grenade_Normal.Single" );

		m_detonateTimer.Start( tf_boss_alpha_grenade_det_time.GetFloat() );
	}

	if ( m_detonateTimer.HasStarted() && m_detonateTimer.IsElapsed() )
	{
		// detonate the stickies
		for( int i=0; i<m_grenadeVector.Count(); ++i )
		{
			if ( m_grenadeVector[i] )
			{
				m_grenadeVector[i]->Detonate();
			}
		}

		return Done();
	}

	return Continue();
}


//---------------------------------------------------------------------------------------------
void CBossAlphaLaunchGrenades::OnEnd( CBossAlpha *me, Action< CBossAlpha > *nextAction )
{
	// fizzle any outstanding stickies
	for( int i=0; i<m_grenadeVector.Count(); ++i )
	{
		if ( m_grenadeVector[i] )
		{
			m_grenadeVector[i]->Fizzle();
			m_grenadeVector[i]->Detonate();
		}
	}

	me->RemoveCondition( CBossAlpha::ENRAGED );
	me->RemoveCondition( CBossAlpha::BUSY );
	me->FastRemoveLayer( m_animLayer );
}

#endif // TF_RAID_MODE
