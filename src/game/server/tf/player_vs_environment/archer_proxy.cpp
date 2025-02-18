//========= Copyright Valve Corporation, All rights reserved. ============//
#include "cbase.h"

#include "tf_player.h"
#include "tf_projectile_arrow.h"
#include "tf_weapon_grenade_pipebomb.h"
#include "archer_proxy.h"

LINK_ENTITY_TO_CLASS( archer_proxy, CTFArcherProxy );


ConVar tf_archer_proxy_fire_rate( "tf_archer_proxy_fire_rate", "1", FCVAR_CHEAT );


//--------------------------------------------------------------------------------------
void CTFArcherProxy::Precache( void )
{
	// don't need to precache, since player does this for us
	BaseClass::Precache();
}


//--------------------------------------------------------------------------------------
void CTFArcherProxy::Spawn( void )
{
	BaseClass::Spawn();

	SetThink( &CTFArcherProxy::Update );
	SetNextThink( gpGlobals->curtime + RandomFloat( 0.1, 1.0f ) * tf_archer_proxy_fire_rate.GetFloat() );

	m_state = HIDDEN;
	m_timer.Invalidate();
	AddEffects( EF_NODRAW );
	m_homePos = GetAbsOrigin();

	SetModel( "models/weapons/w_models/w_arrow.mdl" );
}


//---------------------------------------------------------------------------------------------
CTFPlayer *CTFArcherProxy::SelectTarget( void )
{
	// collect everyone
	CUtlVector< CTFPlayer * > playerVector;
	CollectPlayers( &playerVector, TF_TEAM_RED, COLLECT_ONLY_LIVING_PLAYERS );
	CollectPlayers( &playerVector, TF_TEAM_BLUE, COLLECT_ONLY_LIVING_PLAYERS, APPEND_PLAYERS );

	CTFPlayer *newVictim = NULL;
	float victimRangeSq = FLT_MAX;
	trace_t result;

	for( int i=0; i<playerVector.Count(); ++i )
	{
		float rangeSq = ( playerVector[i]->GetAbsOrigin() - GetAbsOrigin() ).LengthSqr();
		if ( rangeSq < victimRangeSq )
		{
			UTIL_TraceLine( GetAbsOrigin(), playerVector[i]->EyePosition(), MASK_SOLID, this, COLLISION_GROUP_DEBRIS, &result );			

			if ( !result.DidHit() )
			{
				newVictim = playerVector[i];
				victimRangeSq = rangeSq;
			}
		}
	}

	return newVictim;
}


//--------------------------------------------------------------------------------------
void CTFArcherProxy::Update( void )
{
	SetNextThink( gpGlobals->curtime );

	switch( m_state )
	{
	case HIDDEN:
		m_state = EMERGE;
		RemoveEffects( EF_NODRAW );
		m_timer.Start( 1.0f );
		break;

	case EMERGE:
		m_state = AIM_AND_FIRE;
		m_timer.Start( 1.0f );
		break;

	case AIM_AND_FIRE:
		{
			CTFPlayer *target = SelectTarget();
			if ( target )
			{
				Vector to = target->GetAbsOrigin() - GetAbsOrigin();
				QAngle angles;
				VectorAngles( to, angles );

				SetAbsAngles( angles );

				if ( m_timer.IsElapsed() )
				{
					ShootArrowAt( target );
					// ShootGrenadeAt( target );

					m_state = HIDE;
					m_timer.Start( 1.0f );
				}
			}
			break;
		}

	case HIDE:
		if ( m_timer.IsElapsed() )
		{
			m_state = HIDDEN;
			AddEffects( EF_NODRAW );
			m_timer.Start( 1.0f );
		}
		break;
	}

}


//--------------------------------------------------------------------------------------
void CTFArcherProxy::ShootArrowAt( CBaseEntity *target )
{
	if ( !target )
	{
		return;
	}

	Vector to = target->EyePosition() - GetAbsOrigin();
	to.NormalizeInPlace();

	QAngle angles;
	VectorAngles( to, angles );

	const float arrowSpeed = 2600.0f;
	const float arrowGravity = 0.1f;
	CTFProjectile_Arrow *arrow = CTFProjectile_Arrow::Create( GetAbsOrigin() + 20.0f * to, angles, arrowSpeed, arrowGravity, TF_PROJECTILE_ARROW, this, this );
	if ( arrow )
	{
		arrow->SetLauncher( this );
		arrow->SetCritical( false );
		arrow->SetDamage( 100.0f );

		EmitSound( "Weapon_CompoundBow.Single" );
	}
}


//--------------------------------------------------------------------------------------
void CTFArcherProxy::ShootGrenadeAt( CBaseEntity *target )
{
	if ( !target )
	{
		return;
	}

	Vector to = target->EyePosition() - GetAbsOrigin();
	to.NormalizeInPlace();

	QAngle angles;
	VectorAngles( to, angles );


	float launchSpeed = 1000.0f;
	Vector velocity = ( to * launchSpeed ) + ( Vector( 0, 0, 1.0f ) * 200.0f );
	AngularImpulse angVelocity = AngularImpulse( 600, random->RandomInt( -1200, 1200 ), 0 );

	CTFGrenadePipebombProjectile *grenade = static_cast< CTFGrenadePipebombProjectile * >( CBaseEntity::CreateNoSpawn( "tf_projectile_pipe", GetAbsOrigin(), angles, NULL ) );
	if ( grenade )
	{
		// Set the pipebomb mode before calling spawn, so the model & associated vphysics get setup properly
		grenade->SetPipebombMode();
		DispatchSpawn( grenade );

		const int damage = 100;
		const float radius = 100.0f;

		grenade->InitGrenade( velocity, angVelocity, NULL, damage, radius );
		grenade->m_flFullDamage = grenade->GetDamage();
		grenade->ApplyLocalAngularVelocityImpulse( angVelocity );
		grenade->SetCritical( false );
		grenade->SetLauncher( this );
	}
}
