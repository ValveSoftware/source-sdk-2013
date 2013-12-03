//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "sdk_basegrenade_projectile.h"

float GetCurrentGravity( void );


#ifdef CLIENT_DLL

	#include "c_sdk_player.h"

#else

	#include "soundent.h"

	BEGIN_DATADESC( CBaseGrenadeProjectile )
		DEFINE_THINKFUNC( DangerSoundThink ),
	END_DATADESC()

#endif


IMPLEMENT_NETWORKCLASS_ALIASED( BaseGrenadeProjectile, DT_BaseGrenadeProjectile )

BEGIN_NETWORK_TABLE( CBaseGrenadeProjectile, DT_BaseGrenadeProjectile )
	#ifdef CLIENT_DLL
		RecvPropVector( RECVINFO( m_vInitialVelocity ) )
	#else
		SendPropVector( SENDINFO( m_vInitialVelocity ), 
			20,		// nbits
			0,		// flags
			-3000,	// low value
			3000	// high value
			)
	#endif
END_NETWORK_TABLE()


#ifdef CLIENT_DLL


	void CBaseGrenadeProjectile::PostDataUpdate( DataUpdateType_t type )
	{
		BaseClass::PostDataUpdate( type );

		if ( type == DATA_UPDATE_CREATED )
		{
			// Now stick our initial velocity into the interpolation history 
			CInterpolatedVar< Vector > &interpolator = GetOriginInterpolator();
			
			interpolator.ClearHistory();
			float changeTime = GetLastChangeTime( LATCH_SIMULATION_VAR );

			// Add a sample 1 second back.
			Vector vCurOrigin = GetLocalOrigin() - m_vInitialVelocity;
			interpolator.AddToHead( changeTime - 1.0, &vCurOrigin, false );

			// Add the current sample.
			vCurOrigin = GetLocalOrigin();
			interpolator.AddToHead( changeTime, &vCurOrigin, false );
		}
	}

	int CBaseGrenadeProjectile::DrawModel( int flags )
	{
		// During the first half-second of our life, don't draw ourselves if he's
		// still playing his throw animation.
		// (better yet, we could draw ourselves in his hand).
		if ( GetThrower() != C_BasePlayer::GetLocalPlayer() )
		{
			if ( gpGlobals->curtime - m_flSpawnTime < 0.5 )
			{
				C_SDKPlayer *pPlayer = dynamic_cast<C_SDKPlayer*>( GetThrower() );
				if ( pPlayer && pPlayer->m_PlayerAnimState->IsThrowingGrenade() )
				{
					return 0;
				}
			}
		}

		return BaseClass::DrawModel( flags );
	}

	void CBaseGrenadeProjectile::Spawn()
	{
		m_flSpawnTime = gpGlobals->curtime;
		BaseClass::Spawn();
	}

#else

	void CBaseGrenadeProjectile::Spawn( void )
	{
		BaseClass::Spawn();

		SetSolidFlags( FSOLID_NOT_STANDABLE );
		SetMoveType( MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_CUSTOM );
		SetSolid( SOLID_BBOX );	// So it will collide with physics props!

		// smaller, cube bounding box so we rest on the ground
		SetSize( Vector ( -2, -2, -2 ), Vector ( 2, 2, 2 ) );
	}

	void CBaseGrenadeProjectile::DangerSoundThink( void )
	{
		if (!IsInWorld())
		{
			Remove( );
			return;
		}

		if( gpGlobals->curtime > m_flDetonateTime )
		{
			Detonate();
			return;
		}

		CSoundEnt::InsertSound ( SOUND_DANGER, GetAbsOrigin() + GetAbsVelocity() * 0.5, GetAbsVelocity().Length( ), 0.2 );

		SetNextThink( gpGlobals->curtime + 0.2 );

		if (GetWaterLevel() != 0)
		{
			SetAbsVelocity( GetAbsVelocity() * 0.5 );
		}
	}

	//Sets the time at which the grenade will explode
	void CBaseGrenadeProjectile::SetDetonateTimerLength( float timer )
	{
		m_flDetonateTime = gpGlobals->curtime + timer;
	}

	void CBaseGrenadeProjectile::ResolveFlyCollisionCustom( trace_t &trace, Vector &vecVelocity )
	{
		//Assume all surfaces have the same elasticity
		float flSurfaceElasticity = 1.0;

		//Don't bounce off of players with perfect elasticity
		if( trace.m_pEnt && trace.m_pEnt->IsPlayer() )
		{
			flSurfaceElasticity = 0.3;
		}

		// if its breakable glass and we kill it, don't bounce.
		// give some damage to the glass, and if it breaks, pass 
		// through it.
		bool breakthrough = false;

		if( trace.m_pEnt && FClassnameIs( trace.m_pEnt, "func_breakable" ) )
		{
			breakthrough = true;
		}

		if( trace.m_pEnt && FClassnameIs( trace.m_pEnt, "func_breakable_surf" ) )
		{
			breakthrough = true;
		}

		if (breakthrough)
		{
			CTakeDamageInfo info( this, this, 10, DMG_CLUB );
			trace.m_pEnt->DispatchTraceAttack( info, GetAbsVelocity(), &trace );

			ApplyMultiDamage();

			if( trace.m_pEnt->m_iHealth <= 0 )
			{
				// slow our flight a little bit
				Vector vel = GetAbsVelocity();

				vel *= 0.4;

				SetAbsVelocity( vel );
				return;
			}
		}
		
		float flTotalElasticity = GetElasticity() * flSurfaceElasticity;
		flTotalElasticity = clamp( flTotalElasticity, 0.0f, 0.9f );

		// NOTE: A backoff of 2.0f is a reflection
		Vector vecAbsVelocity;
		PhysicsClipVelocity( GetAbsVelocity(), trace.plane.normal, vecAbsVelocity, 2.0f );
		vecAbsVelocity *= flTotalElasticity;

		// Get the total velocity (player + conveyors, etc.)
		VectorAdd( vecAbsVelocity, GetBaseVelocity(), vecVelocity );
		float flSpeedSqr = DotProduct( vecVelocity, vecVelocity );

		// Stop if on ground.
		if ( trace.plane.normal.z > 0.7f )			// Floor
		{
			// Verify that we have an entity.
			CBaseEntity *pEntity = trace.m_pEnt;
			Assert( pEntity );

			SetAbsVelocity( vecAbsVelocity );

			if ( flSpeedSqr < ( 30 * 30 ) )
			{
				if ( pEntity->IsStandable() )
				{
					SetGroundEntity( pEntity );
				}

				// Reset velocities.
				SetAbsVelocity( vec3_origin );
				SetLocalAngularVelocity( vec3_angle );

				//align to the ground so we're not standing on end
				QAngle angle;
				VectorAngles( trace.plane.normal, angle );

				// rotate randomly in yaw
				angle[1] = random->RandomFloat( 0, 360 );

				// TODO: rotate around trace.plane.normal
				
				SetAbsAngles( angle );			
			}
			else
			{
				Vector vecDelta = GetBaseVelocity() - vecAbsVelocity;	
				Vector vecBaseDir = GetBaseVelocity();
				VectorNormalize( vecBaseDir );
				float flScale = vecDelta.Dot( vecBaseDir );

				VectorScale( vecAbsVelocity, ( 1.0f - trace.fraction ) * gpGlobals->frametime, vecVelocity ); 
				VectorMA( vecVelocity, ( 1.0f - trace.fraction ) * gpGlobals->frametime, GetBaseVelocity() * flScale, vecVelocity );
				PhysicsPushEntity( vecVelocity, &trace );
			}
		}
		else
		{
			// If we get *too* slow, we'll stick without ever coming to rest because
			// we'll get pushed down by gravity faster than we can escape from the wall.
			if ( flSpeedSqr < ( 30 * 30 ) )
			{
				// Reset velocities.
				SetAbsVelocity( vec3_origin );
				SetLocalAngularVelocity( vec3_angle );
			}
			else
			{
				SetAbsVelocity( vecAbsVelocity );
			}
		}
		
		BounceSound();
	}

	void CBaseGrenadeProjectile::SetupInitialTransmittedGrenadeVelocity( const Vector &velocity )
	{
		m_vInitialVelocity = velocity;
	}

#endif
