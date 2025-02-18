//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"

#include "tf_weapon_dragons_fury.h"

#ifdef CLIENT_DLL
	#include "c_tf_player.h"
	#include "dlight.h"
	#include "iefx.h"
	#include "tempent.h"
	#include "debugoverlay_shared.h"
#else
	#include "tf_player.h"
	#include "tf_fx.h"
	#include "tf_projectile_rocket.h"
	#include "tf_logic_robot_destruction.h"
	#include "tf_weapon_compound_bow.h"
	#include "tf_pumpkin_bomb.h"
	#include "halloween/merasmus/merasmus_trick_or_treat_prop.h"
	#include "tf_robot_destruction_robot.h"
	#include "tf_generic_bomb.h"
#endif

#ifdef CLIENT_DLL
	#define CTFProjectile_BallOfFire				C_TFProjectile_BallOfFire
#endif

ConVar tf_fireball_distance( "tf_fireball_distance", "500", FCVAR_REPLICATED | FCVAR_CHEAT ); // 375 = 3000 * 0.125, which is the speed and lifetime we tested with
ConVar tf_fireball_speed( "tf_fireball_speed", "3000", FCVAR_REPLICATED | FCVAR_CHEAT );
ConVar tf_fireball_damage( "tf_fireball_damage", "25", FCVAR_REPLICATED | FCVAR_CHEAT );
ConVar tf_fireball_burn_duration( "tf_fireball_burn_duration", "2", FCVAR_REPLICATED | FCVAR_CHEAT );
ConVar tf_fireball_radius( "tf_fireball_radius", "22.5", FCVAR_REPLICATED | FCVAR_CHEAT );
ConVar tf_fireball_draw_debug_radius( "tf_fireball_draw_debug_radius", "0", FCVAR_REPLICATED | FCVAR_CHEAT );
ConVar tf_fireball_burning_bonus( "tf_fireball_burning_bonus", "3", FCVAR_REPLICATED | FCVAR_CHEAT );
ConVar tf_fireball_max_lifetime( "tf_fireball_max_lifetime", "0.5", FCVAR_REPLICATED | FCVAR_CHEAT );


class CTFProjectile_BallOfFire : public CTFProjectile_Rocket
{
public:
	DECLARE_CLASS( CTFProjectile_BallOfFire, CTFProjectile_Rocket );
	DECLARE_NETWORKCLASS();

	CTFProjectile_BallOfFire() {}

	~CTFProjectile_BallOfFire()
	{
		if ( tf_fireball_draw_debug_radius.GetBool() )
		{
			NDebugOverlay::Sphere( GetAbsOrigin(), tf_fireball_radius.GetFloat() * 3.f, 255, 0, 255, false, 2.f );
		}
	}

	virtual void Precache() OVERRIDE
	{
		PrecacheModel( "models/empty.mdl" );
		PrecacheScriptSound( "Weapon_DragonsFury.Nearmiss" );
	}

	virtual void Spawn() OVERRIDE
	{
		BaseClass::Spawn();

#ifdef GAME_DLL
		SetRenderMode( kRenderNone );

		SetSolid( SOLID_BBOX );
		SetMoveType( MOVETYPE_FLY, MOVECOLLIDE_FLY_CUSTOM );
		SetSolidFlags( FSOLID_TRIGGER | FSOLID_NOT_SOLID );
		SetCollisionGroup( TFCOLLISION_GROUP_ROCKETS );
		// The fireball's collision is *much* larger than the actual fireball itself.  We don't want just expand the collision
		// bounds or else it'd hit walls and things when it seems like it shouldn't.  Using UseTriggerBounds allows us to get
		// touches with players in a radius larger than our collision bounds.
		const float flRadius = tf_fireball_radius.GetFloat();
		CollisionProp()->SetCollisionBounds( Vector( -1, -1, -1 ), Vector( 1, 1, 1 ) );
		CollisionProp()->UseTriggerBounds( true, flRadius, true );

		m_vecInitialVelocity = GetAbsVelocity().Normalized() * tf_fireball_speed.GetFloat();
		SetAbsVelocity( m_vecInitialVelocity );

		float flDamage = tf_fireball_damage.GetFloat();
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( GetLauncher(), flDamage, mult_dmg );
		SetDamage( flDamage );

		m_vecSpawnOrigin = GetAbsOrigin();
		// This will limit how far we can go
		SetContextThink( &CTFProjectile_BallOfFire::DistanceLimitThink, gpGlobals->curtime, "DistanceLimitThink" );

		// This will limit how long we're alive (in case we get stuck somewhere)
		SetContextThink( &CTFProjectile_BallOfFire::ExpireDelayThink, gpGlobals->curtime + tf_fireball_max_lifetime.GetFloat(), "ExpireDelayThink" );
#endif
	}

#ifdef GAME_DLL
	void ExpireDelayThink()
	{
		SetContextThink( &CBaseGrenade::SUB_Remove, gpGlobals->curtime, "RemoveThink" );
	}

	// This gets called *BEFORE* the physics move, so we have a chance to update our velocity
	// so that physics won't move us beyond our distance limit
	void DistanceLimitThink()
	{
		if ( m_bFizzling )
			return;

		if ( UTIL_PointContents( GetAbsOrigin() ) & MASK_WATER )
		{
			StopAndFizzle();
			return;
		}

		const float flMaxDist = tf_fireball_distance.GetFloat();
		float flDistance = ( GetAbsOrigin() - m_vecSpawnOrigin ).Length();
		const float flDt = gpGlobals->frametime;

		// Get where our new position will be
		Vector vecVelThisFrame;
		Vector vecAbsVelocity = GetAbsVelocity();
		vecAbsVelocity += GetBaseVelocity();
		VectorScale( vecAbsVelocity, gpGlobals->frametime, vecVelThisFrame );
		Vector vecNewPos = GetAbsOrigin() + vecVelThisFrame;

		// Check if we're about to go too far, and clip our velocity so that we don't
		float flNewDistSqr = ( vecNewPos - m_vecSpawnOrigin ).LengthSqr();
		if ( flNewDistSqr > ( flMaxDist * flMaxDist ) )
		{
			float flDistToGo = flMaxDist - flDistance;
			
			// CloseEnough is close enough
			if ( CloseEnough( flDistToGo, 0.f, 1.f ) )
			{
				m_bFizzling = true;
				SetAbsVelocity( vec3_origin );

				// Put the projectile to sleep while we wait for the cl_interp window to expire (keeps the dlight effect in sync)
				CBaseEntity *pOwner = GetOwnerEntity();
				float flLerpAmount = Q_atof( engine->GetClientConVarValue( pOwner->entindex(), "cl_interp" ) );
				SetContextThink( &CTFProjectile_BallOfFire::ExpireDelayThink, gpGlobals->curtime + flLerpAmount, "ExpireDelayThink" );
			}
			else
			{
				Vector vecDirection = vecAbsVelocity.Normalized();
				SetAbsVelocity( vecDirection * ( flDistToGo / flDt ) );
			}
		}

		// Always do this think
		SetContextThink( &CTFProjectile_BallOfFire::DistanceLimitThink, gpGlobals->curtime, "DistanceLimitThink" );
	}

	void StopAndFizzle()
	{
		if ( m_bFizzling )
			return;

		SetAbsVelocity( vec3_origin );

		m_bFizzling = true;
		SetContextThink( &CBaseGrenade::SUB_Remove, gpGlobals->curtime + 0.1f, "RemoveThink" );
	}
	virtual const char *GetProjectileModelName( void ) { return "models/empty.mdl"; } // We dont have a model by default, and that's OK

	virtual float		GetDamageRadius()	const			{ return tf_fireball_radius.GetFloat(); }
	virtual int			GetCustomDamageType() const OVERRIDE { Assert( false ); return TF_DMG_CUSTOM_DRAGONS_FURY_IGNITE; }

	virtual void RocketTouch( CBaseEntity *pOther ) OVERRIDE
	{
		if ( m_bFizzling )
			return;

		// Verify a correct "other."
		Assert( pOther );
		if ( !pOther->IsSolid() ||
			 pOther->IsSolidFlagSet( FSOLID_VOLUME_CONTENTS ) ||
			 pOther->IsSolidFlagSet( FSOLID_NOT_SOLID ) ||
			 ( pOther->GetCollisionGroup() == TFCOLLISION_GROUP_RESPAWNROOMS ) ||
			 pOther->IsFuncLOD() ||
			 pOther->IsBaseProjectile() )
		{
			return;
		}

		CBaseEntity *pOwner = GetOwnerEntity();
		// Don't shoot ourselves
		if ( pOwner == pOther )
			return;

		// Handle hitting skybox (disappear).
		const trace_t *pTrace = &CBaseEntity::GetTouchTrace();
		if ( pTrace->surface.flags & SURF_SKY )
		{
			UTIL_Remove( this );
			return;
		}

		// pass through ladders
		if ( pTrace->surface.flags & CONTENTS_LADDER )
			return;

		if ( !ShouldTouchNonWorldSolid( pOther, pTrace ) )
			return;

		// The stuff we collide with
		bool bCombatEntity = pOther->IsPlayer() ||
			pOther->IsBaseObject() ||
			pOther->IsCombatCharacter() ||
			pOther->IsCombatItem() ||
			pOther->IsProjectileCollisionTarget();

		if ( bCombatEntity )
		{
			Burn( pOther );
			return;
		}

		// Die if we hit a thing that's not a player, building, or projectile target
		StopAndFizzle();
	}

	void RefundAmmo()
	{
		if ( !m_bRefunded )
		{
			CTFWeaponFlameBall* pFlameLauncher = dynamic_cast< CTFWeaponFlameBall* >( GetLauncher() );
			if ( pFlameLauncher )
			{
				pFlameLauncher->RefundAmmo( 1 );
				m_bRefunded = true;
			}
		}
	}

	virtual const char *GetExplodeEffectSound()	const		{ return "Halloween.spell_fireball_impact"; }

	void OnCollideWithTeammate( CTFPlayer *pTFPlayer )
	{
		// Only care about Snipers
		if ( !pTFPlayer->IsPlayerClass( TF_CLASS_SNIPER ) )
			return;

		int nEntIndex = pTFPlayer->entindex();
		if ( m_vecHitPlayers.Find( nEntIndex ) != m_vecHitPlayers.InvalidIndex() )
			return;

		m_vecHitPlayers.AddToTail( nEntIndex );

		// Does he have the bow?
		CTFWeaponBase *pWpn = pTFPlayer->GetActiveTFWeapon();
		if ( pWpn && ( pWpn->GetWeaponID() == TF_WEAPON_COMPOUND_BOW ) )
		{
			CTFCompoundBow *pBow = static_cast< CTFCompoundBow* >( pWpn );
			pBow->SetArrowAlight( true );
		}
	}

	void Burn( CBaseEntity* pTarget )
	{
		CBaseEntity *pOwner = GetOwnerEntity();
		CTFPlayer* pTFOwner = ToTFPlayer( pOwner );
		CTFPlayer *pTFPlayer = ToTFPlayer( pTarget );
		
		if ( pOwner->InSameTeam( pTarget ) )
		{
			if ( pTFPlayer )
			{
				OnCollideWithTeammate( pTFPlayer );
			}
			return;
		}
		
		int nEntIndex = pTarget->entindex();
		if ( m_vecHitPlayers.Find( nEntIndex ) != m_vecHitPlayers.InvalidIndex() )
			return;

		m_vecHitPlayers.AddToTail( nEntIndex );

		if ( !pOwner )
			return;

		if ( !pTarget->IsAlive() )
			return;

		if ( !IsEntityVisible( pTarget ) )
			return;

		CTakeDamageInfo info;
		info.SetAttacker( pOwner );
		info.SetInflictor( this ); 
		info.SetWeapon( GetLauncher() );
		info.SetDamagePosition( GetAbsOrigin() );
		info.SetDamageType( DMG_IGNITE | DMG_USEDISTANCEMOD | DMG_NOCLOSEDISTANCEMOD );

		// CRITS!
		if ( IsCritical() )
		{
			info.SetDamageType( info.GetDamageType() | DMG_CRITICAL );
		}

		CTraceFilterIgnoreTeammates tracefilter( this, COLLISION_GROUP_NONE, GetTeamNumber() );
		trace_t trace;
		UTIL_TraceLine( GetAbsOrigin(), pTarget->GetAbsOrigin(), ( MASK_SHOT & ~( CONTENTS_HITBOX ) ), &tracefilter, &trace );
		if ( trace.DidHitWorld() )
			return;

		// Show a little burn particle
		CPVSFilter filter( pTarget->WorldSpaceCenter() );
		Vector vStart = WorldSpaceCenter();
		Vector vEnd = vStart + ( pTarget->WorldSpaceCenter() - vStart ).Normalized() * GetDamageRadius();
		const char *pszHitEffect = "torch_player_burn";
		te_tf_particle_effects_control_point_t controlPoint = { PATTACH_ABSORIGIN, vEnd };
		TE_TFParticleEffectComplex( filter, 0.0f, pszHitEffect, vEnd, QAngle( 0, 0, 0 ), NULL, &controlPoint, pTarget, PATTACH_CUSTOMORIGIN );

		bool bBonusDamage = false;

		if ( pTFPlayer )
		{
			if ( pTFPlayer->m_Shared.InCond( TF_COND_PHASE ) || pTFPlayer->m_Shared.InCond( TF_COND_PASSTIME_INTERCEPTION ) )
				return;

			if ( pTFPlayer->m_Shared.IsInvulnerable() )
				return;

			// Trace forward and see if we would touch a hitbox if we kept going
			CTraceFilterCollisionArrows filterHitBox( this, GetOwnerEntity() );
			trace_t trForward;
			UTIL_TraceLine( GetAbsOrigin(), GetAbsOrigin() + GetAbsVelocity() * gpGlobals->frametime * 1.5f, ( MASK_SHOT & ~( CONTENTS_HITBOX ) ), this, COLLISION_GROUP_PLAYER, &trForward );
			bool bHitBBox = trForward.DidHit() && trForward.m_pEnt && trForward.m_pEnt == pTFPlayer;
			//NDebugOverlay::Line( GetAbsOrigin(), GetAbsOrigin() + GetAbsVelocity() * gpGlobals->frametime, 255.f, 0.f, 0.f, false, 2.5f );
			//NDebugOverlay::Cross3D( trForward.endpos, 32.f, 0.f, 255.f, 0.f, false, 2.5f );

			bBonusDamage = ( pTFPlayer->m_Shared.InCond( TF_COND_BURNING ) && bHitBBox );
			float flDamageBonusScale = ( bBonusDamage ) ? tf_fireball_burning_bonus.GetFloat() : 1.f;
			info.SetDamage( GetDamage() * flDamageBonusScale );
			info.SetDamageCustom( bBonusDamage ? TF_DMG_CUSTOM_DRAGONS_FURY_BONUS_BURNING : TF_DMG_CUSTOM_DRAGONS_FURY_IGNITE );

			float flBurnDuration = tf_fireball_burn_duration.GetFloat();

			// This burn affects pyros, too, but only half as long
			if ( pTFPlayer->IsPlayerClass( TF_CLASS_PYRO ) )
			{
				pTFPlayer->m_Shared.AddCond( TF_COND_BURNING_PYRO, ( flBurnDuration * 0.5f ), pOwner );
			}

			// Ignite them AFTER we figure out the damage.  We do extra damage to burning players.
			pTFPlayer->m_Shared.Burn( ToTFPlayer( pOwner ), (CTFWeaponBase*)GetLauncher(), flBurnDuration );
		}
		else
		{
			// This weapon sucks against non-players since it can't light them on fire, but if you've managed
			// to get in range as a Pyro you deserve to destroy it.  We're going to give the bonus
			// damage always against these targets to help with this.
			bBonusDamage = true;
			info.SetDamage( GetDamage() * tf_fireball_burning_bonus.GetFloat() );
			info.SetDamageCustom( TF_DMG_CUSTOM_DRAGONS_FURY_BONUS_BURNING );
		}


		RefundAmmo();
		
		// Hurt 'em.
		Vector dir;
		AngleVectors( GetAbsAngles(), &dir );
		pTarget->DispatchTraceAttack( info, dir, &trace );
		ApplyMultiDamage();

		// Impact sound.  We only play it once per projectile.  It's so fast, nobody will notice
		// there's only one, but their ears will thank us that there is only one.
		if ( !m_bImpactSoundPlayed )
		{
			m_bImpactSoundPlayed = true;

			if ( bBonusDamage )
			{
				CRecipientFilter filterImpact;
				filterImpact.AddAllPlayers();
				// When we're doing bonus damage, don't play this sound to
				// the victim and the attacker.  They're going to get special
				// sounds in their ears.
				if ( pTFPlayer )
				{
					filterImpact.RemoveRecipient( pTFPlayer );
				}
				
				if ( pTFOwner )
				{
					filterImpact.RemoveRecipient( pTFOwner );
				}

				// Bonus damage hit sound
				{
					EmitSound_t params;
					params.m_flSoundTime = 0;
					params.m_pflSoundDuration = 0;
					params.m_pSoundName = "Weapon_DragonsFury.BonusDamage";
					pTarget->EmitSound( filterImpact, pTarget->entindex(), params );
				}

				// Special pain sound for bonus damage
				if ( pTFPlayer )
				{
					EmitSound_t params;
					params.m_flSoundTime = 0;
					params.m_pflSoundDuration = 0;
					params.m_pSoundName = "Weapon_DragonsFury.BonusDamagePain";

					CSingleUserRecipientFilter filterVictim( pTFPlayer );
					EmitSound( filterVictim, pTFPlayer->entindex(), params );
				}
			}
		}

		// Special sound in the shooter's ears to let them know they got the Bonus Damage
		if ( bBonusDamage && !m_bBonusSoundPlayed )
		{
			m_bBonusSoundPlayed = true;
			// sound effects
			EmitSound_t params;
			params.m_flSoundTime = 0;
			params.m_pflSoundDuration = 0;
			params.m_pSoundName = "Weapon_DragonsFury.BonusDamageHit";

			CSingleUserRecipientFilter filterShooter( pTFOwner );
			EmitSound( filterShooter, pTFOwner->entindex(), params );
		}
	}

	bool IsEntityVisible( CBaseEntity *pEntity )
	{
		const trace_t *pTrace = &CBaseEntity::GetTouchTrace();
		
		trace_t trace;
		UTIL_TraceLine( WorldSpaceCenter(), pTrace->endpos, MASK_OPAQUE, this, COLLISION_GROUP_NONE, &trace );
		if ( trace.fraction < 1.f )
			return false;

		return true;
	}

	CUtlVector< int > m_vecHitPlayers;
#endif

#ifdef CLIENT_DLL
	
	// Do nothing
	virtual void CreateTrails( void ) OVERRIDE
	{}

	const char* GetParticle( bool bCrit ) const { return bCrit ? ( GetTeamNumber()==TF_TEAM_BLUE ? "projectile_fireball_crit_blue" : "projectile_fireball_crit_red" ) : "projectile_fireball"; }

	virtual void OnDataChanged( DataUpdateType_t updateType ) OVERRIDE
	{
		BaseClass::OnDataChanged( updateType );

		if ( updateType == DATA_UPDATE_CREATED )
		{
			SetNextClientThink(CLIENT_THINK_ALWAYS);

			// Create the particle on the empty attachment
			int iAttachment = LookupAttachment( "empty" );
			if ( iAttachment == INVALID_PARTICLE_ATTACHMENT )
				return;

			CTFPlayer* pTFPlayerOwner = ToTFPlayer( GetOwnerEntity() );
			if ( !pTFPlayerOwner )
				return;

			// We're going to emit a local temp ent that has the particle
			// attached to it, just as we learn about the real entity being
			// created on the client.  The reason being we don't lerp the deletion
			// of entities on the client, which causes the particle effect to
			// end in very strange/unexpected ways to the shooter.  This way,
			// the projectile always travels as it should.
			m_flTempProjCreationTime = gpGlobals->curtime + GetInterpolationAmount( 0 );
		}
	}

	virtual void ClientThink() OVERRIDE
	{
		BaseClass::ClientThink();

		SetNextClientThink( CLIENT_THINK_ALWAYS );

		if ( gpGlobals->curtime >= m_flTempProjCreationTime && !m_bTempProjCreated )
		{
			m_bTempProjCreated = true;

			int nModelIndex = modelinfo->GetModelIndex( "models/empty.mdl" );
			C_LocalTempEntity* pClientFireball = tempents->ClientProjectile( m_vecSpawnOrigin, m_vecInitialVelocity, vec3_origin, nModelIndex, 2.f, GetOwnerEntity(), NULL, GetParticle( IsCritical() ) );
			Assert( pClientFireball );
			if ( pClientFireball )
			{
				pClientFireball->SetAbsAngles( GetAbsAngles() );
				pClientFireball->flags |= FTENT_COLLISIONGROUP;
				pClientFireball->SetCollisionGroup( COLLISION_GROUP_DEBRIS );
			}
		}

		if ( tf_fireball_draw_debug_radius.GetBool() )
		{
			NDebugOverlay::Sphere( GetAbsOrigin(), tf_fireball_radius.GetFloat() * 3.f, 0, 255, 255, false, 0.f );
		}

		if ( GetOwnerEntity() == C_BasePlayer::GetLocalPlayer() )
		{
			// Deal with the environment light
			if ( !m_pDynamicLight || (m_pDynamicLight->key != index) )
			{
				m_pDynamicLight = effects->CL_AllocDlight( index );
				assert (m_pDynamicLight);
			}

			ColorRGBExp32 color;
			color.r	= 255;
			color.g	= 100;
			color.b	= 30;
			color.exponent = 8;

			//m_pDynamicLight->flags = DLIGHT_NO_MODEL_ILLUMINATION;
			m_pDynamicLight->radius		= 100.f;
			m_pDynamicLight->origin		= GetAbsOrigin() + Vector( 0, 0, 0 );
			m_pDynamicLight->die		= gpGlobals->curtime + 0.05f;
			m_pDynamicLight->color		= color;
		}
		else if ( !m_bNearMiss && ( gpGlobals->curtime - m_flLastNearMissCheck >= 0.05f ) )
		{
			m_bNearMiss = UTIL_BPerformNearMiss( this, "Weapon_DragonsFury.Nearmiss", 200.f );
			m_flLastNearMissCheck = gpGlobals->curtime;
		}
	}
#endif // CLIENT_DLL

protected:
	virtual float GetFireballScale() const { return 1.f; }

private:
#ifdef CLIENT_DLL
	dlight_t*			m_pDynamicLight = NULL;
	bool				m_bNearMiss = false;
	float				m_flLastNearMissCheck = 0.f;
	float				m_flTempProjCreationTime = 0.f;
	bool				m_bTempProjCreated = false;
#endif // CLIENT_DLL

	CNetworkVector( m_vecSpawnOrigin );
	CNetworkVector( m_vecInitialVelocity );

#ifdef GAME_DLL
	bool m_bRefunded = false;
	bool m_bFizzling = false;
	bool m_bImpactSoundPlayed = false;
	bool m_bBonusSoundPlayed = false;
#endif
};

// Lightning ball
IMPLEMENT_NETWORKCLASS_ALIASED( TFProjectile_BallOfFire, DT_TFProjectile_BallOfFire )
BEGIN_NETWORK_TABLE( CTFProjectile_BallOfFire, DT_TFProjectile_BallOfFire )
#if !defined( CLIENT_DLL )
SendPropVector( SENDINFO( m_vecInitialVelocity ), 0, SPROP_NOSCALE ), 
SendPropVector( SENDINFO( m_vecSpawnOrigin ), 0, SPROP_NOSCALE ), 
#else
RecvPropVector( RECVINFO(m_vecInitialVelocity), 0 ),
RecvPropVector( RECVINFO(m_vecSpawnOrigin), 0 ),
#endif
END_NETWORK_TABLE()

LINK_ENTITY_TO_CLASS( tf_projectile_balloffire, CTFProjectile_BallOfFire );
PRECACHE_WEAPON_REGISTER( tf_projectile_balloffire );
