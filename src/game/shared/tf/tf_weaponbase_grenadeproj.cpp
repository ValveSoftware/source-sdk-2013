//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//
#include "cbase.h"
#include "tf_weaponbase_grenadeproj.h"
#include "tf_gamerules.h"

// Client specific.
#ifdef CLIENT_DLL
#include "c_tf_player.h"
// Server specific.
#else
#include "soundent.h"
#include "te_effect_dispatch.h"
#include "tf_player.h"
#include "func_break.h"
#include "func_nogrenades.h"
#include "Sprite.h"
#include "tf_fx.h"
#include "halloween/merasmus/merasmus.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


//=============================================================================
//
// TF Grenade projectile tables.
//

// Server specific.
#ifdef GAME_DLL
BEGIN_DATADESC( CTFWeaponBaseGrenadeProj )
DEFINE_THINKFUNC( DetonateThink ),
END_DATADESC()


extern void SendProxy_Origin( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID );
extern void SendProxy_Angles( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID );

#endif

IMPLEMENT_NETWORKCLASS_ALIASED( TFWeaponBaseGrenadeProj, DT_TFWeaponBaseGrenadeProj )

LINK_ENTITY_TO_CLASS( tf_weaponbase_grenade_proj, CTFWeaponBaseGrenadeProj );
PRECACHE_REGISTER( tf_weaponbase_grenade_proj );

BEGIN_NETWORK_TABLE( CTFWeaponBaseGrenadeProj, DT_TFWeaponBaseGrenadeProj )
#ifdef CLIENT_DLL
	RecvPropVector( RECVINFO( m_vInitialVelocity ) ),
	RecvPropBool( RECVINFO( m_bCritical ) ),
	RecvPropInt( RECVINFO( m_iDeflected ) ),

	RecvPropVector( RECVINFO_NAME( m_vecNetworkOrigin, m_vecOrigin ) ),
	RecvPropQAngles( RECVINFO_NAME( m_angNetworkAngles, m_angRotation ) ),
	RecvPropEHandle( RECVINFO( m_hDeflectOwner )),

#else
	SendPropVector( SENDINFO( m_vInitialVelocity ), 20 /*nbits*/, 0 /*flags*/, -3000 /*low value*/, 3000 /*high value*/	),
	SendPropBool( SENDINFO( m_bCritical ) ),

	SendPropExclude( "DT_BaseEntity", "m_vecOrigin" ),
	SendPropExclude( "DT_BaseEntity", "m_angRotation" ),

	SendPropVector	(SENDINFO(m_vecOrigin), -1,  SPROP_COORD_MP_INTEGRAL|SPROP_CHANGES_OFTEN, 0.0f, HIGH_DEFAULT, SendProxy_Origin ),
	SendPropQAngles	(SENDINFO(m_angRotation), 6, SPROP_CHANGES_OFTEN, SendProxy_Angles ),
	SendPropInt( SENDINFO( m_iDeflected ), 4, SPROP_UNSIGNED ),
	SendPropEHandle(SENDINFO( m_hDeflectOwner )),
#endif
END_NETWORK_TABLE()

//-----------------------------------------------------------------------------
// Purpose: Constructor.
//-----------------------------------------------------------------------------
CTFWeaponBaseGrenadeProj::CTFWeaponBaseGrenadeProj()
{
#ifndef CLIENT_DLL
	m_bUseImpactNormal = false;
	m_vecImpactNormal.Init();
	m_iDeflected = 0;
	m_flDestroyableTime = 0.0f;
	m_bIsMerasmusGrenade = false;
	m_iDestroyableHitCount = 0;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Destructor.
//-----------------------------------------------------------------------------
CTFWeaponBaseGrenadeProj::~CTFWeaponBaseGrenadeProj()
{}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CTFWeaponBaseGrenadeProj::GetDamageType() 
{ 
	int iDmgType = g_aWeaponDamageTypes[ GetWeaponID() ];
	if ( m_bCritical )
	{
		iDmgType |= DMG_CRITICAL;
	}

	return iDmgType;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFWeaponBaseGrenadeProj::GetDamageCustom()
{
	return 0;
}

const float GRENADE_COEFFICIENT_OF_RESTITUTION = 0.2f;

//-----------------------------------------------------------------------------
// Purpose: Bounce backwards
//-----------------------------------------------------------------------------
void CTFWeaponBaseGrenadeProj::BounceOff( IPhysicsObject *pPhysics )
{
	if ( !pPhysics )
		return;

	Vector vecVel;
	pPhysics->GetVelocity( &vecVel, NULL );
	vecVel *= -GRENADE_COEFFICIENT_OF_RESTITUTION;
	pPhysics->SetVelocity( &vecVel, NULL );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFWeaponBaseGrenadeProj::GetDamageRadius() 
{ 
	float flRadius = m_DmgRadius;
	CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( m_hLauncher, flRadius, mult_explosion_radius );
	return flRadius; 
}	

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWeaponBaseGrenadeProj::Precache( void )
{
	BaseClass::Precache();

#ifndef CLIENT_DLL
	PrecacheModel( NOGRENADE_SPRITE );
	PrecacheParticleSystem( "critical_grenade_blue" );
	PrecacheParticleSystem( "critical_grenade_red" );
	PrecacheParticleSystem( "ExplosionCore_Wall_Jumper" );
#endif
}

//=============================================================================
//
// Client specific functions.
//
#ifdef CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFWeaponBaseGrenadeProj::Spawn()
{
	m_flSpawnTime = gpGlobals->curtime;
	BaseClass::Spawn();

	AddFlag( FL_GRENADE );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFWeaponBaseGrenadeProj::OnDataChanged( DataUpdateType_t type )
{
	BaseClass::OnDataChanged( type );

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

//=============================================================================
//
// Server specific functions.
//
#else

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFWeaponBaseGrenadeProj *CTFWeaponBaseGrenadeProj::Create( const char *szName, const Vector &position, const QAngle &angles, 
													   const Vector &velocity, const AngularImpulse &angVelocity, 
													   CBaseCombatCharacter *pOwner, const CTFWeaponInfo &weaponInfo, int iFlags )
{
	CTFWeaponBaseGrenadeProj *pGrenade = static_cast<CTFWeaponBaseGrenadeProj*>( CBaseEntity::Create( szName, position, angles, pOwner ) );
	if ( pGrenade )
	{
		pGrenade->InitGrenade( velocity, angVelocity, pOwner, weaponInfo );
	}

	return pGrenade;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWeaponBaseGrenadeProj::InitGrenade( const Vector &velocity, const AngularImpulse &angVelocity, 
										   CBaseCombatCharacter *pOwner, const CTFWeaponInfo &weaponInfo )
{
	InitGrenade( velocity, angVelocity, pOwner, weaponInfo.GetWeaponData( TF_WEAPON_PRIMARY_MODE ).m_nDamage, weaponInfo.m_flDamageRadius );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWeaponBaseGrenadeProj::InitGrenade( const Vector &velocity, const AngularImpulse &angVelocity, 
									CBaseCombatCharacter *pOwner, const int iDamage, const float flRadius )
{
	// We can't use OwnerEntity for grenades, because then the owner can't shoot them with his hitscan weapons (due to collide rules)
	// Thrower is used to store the person who threw the grenade, for damage purposes.
	SetOwnerEntity( NULL );
	SetThrower( pOwner ); 

	SetupInitialTransmittedGrenadeVelocity( velocity );

	SetGravity( 0.4f/*BaseClass::GetGrenadeGravity()*/ );
	SetFriction( 0.2f/*BaseClass::GetGrenadeFriction()*/ );
	SetElasticity( 0.45f/*BaseClass::GetGrenadeElasticity()*/ );

	SetDamage( iDamage );
	SetDamageRadius( flRadius );
	ChangeTeam( pOwner ? pOwner->GetTeamNumber() : TEAM_UNASSIGNED );

	IPhysicsObject *pPhysicsObject = VPhysicsGetObject();
	if ( pPhysicsObject )
	{
		pPhysicsObject->AddVelocity( &velocity, &angVelocity );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFWeaponBaseGrenadeProj::Spawn( void )
{
	// Base class spawn.
	BaseClass::Spawn();

	// So it will collide with physics props!
	SetSolidFlags( FSOLID_NOT_STANDABLE );
	SetSolid( SOLID_BBOX );	

	AddEffects( EF_NOSHADOW );

	// Set the grenade size here.
	UTIL_SetSize( this, TF_GRENADE_PROJECTILE_MINS, TF_GRENADE_PROJECTILE_MAXS );

	// Set the movement type.
	SetCollisionGroup( TF_COLLISIONGROUP_GRENADES );

	VPhysicsInitNormal( SOLID_BBOX, 0, false );

	m_takedamage = DAMAGE_EVENTS_ONLY;

	// Set the team.
	ChangeTeam( GetThrower() ? GetThrower()->GetTeamNumber() : TEAM_UNASSIGNED );

	// Set skin based on team ( red = 1, blue = 2 )
	m_nSkin = ( GetTeamNumber() == TF_TEAM_BLUE ) ? 1 : 0;

	m_flDestroyableTime = gpGlobals->curtime + TF_GRENADE_DESTROYABLE_TIMER;

	// Setup the think and touch functions (see CBaseEntity).
	SetThink( &CTFWeaponBaseGrenadeProj::DetonateThink );
	SetNextThink( gpGlobals->curtime + 0.2 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
#define TF_GRENADE_JUMP_RADIUS	146
void CTFWeaponBaseGrenadeProj::Explode( trace_t *pTrace, int bitsDamageType )
{
	if ( ShouldNotDetonate() )
	{
		Destroy();
		return;
	}

	SetModelName( NULL_STRING );//invisible
	AddSolidFlags( FSOLID_NOT_SOLID );

	m_takedamage = DAMAGE_NO;

	// Pull out of the wall a bit
	if ( pTrace->fraction != 1.0 )
	{
		SetAbsOrigin( pTrace->endpos + ( pTrace->plane.normal * 1.0f ) );
	}

	CSoundEnt::InsertSound ( SOUND_COMBAT, GetAbsOrigin(), BASEGRENADE_EXPLOSION_VOLUME, 3.0 );

	// Explosion effect on client
	Vector vecOrigin = GetAbsOrigin();
	CPVSFilter filter( vecOrigin );


	item_definition_index_t ownerWeaponDefIndex = INVALID_ITEM_DEF_INDEX;
	CTFWeaponBase *pWeapon = dynamic_cast<CTFWeaponBase *>(GetOriginalLauncher());
	if (pWeapon)
	{
		ownerWeaponDefIndex = pWeapon->GetAttributeContainer()->GetItem()->GetItemDefIndex();
	}

	// Halloween Custom Spell Effect
	int iHalloweenSpell = 0;
	int iCustomParticleIndex = GetCustomParticleIndex();
	if ( TF_IsHolidayActive( kHoliday_HalloweenOrFullMoon ) )
	{
		CALL_ATTRIB_HOOK_INT_ON_OTHER( m_hLauncher, iHalloweenSpell, halloween_pumpkin_explosions );
		if ( iHalloweenSpell > 0 )
		{
			iCustomParticleIndex = GetParticleSystemIndex( "halloween_explosion" );
		}
	}

	int iNoSelfBlastDamage = 0;
	CALL_ATTRIB_HOOK_INT_ON_OTHER( m_hLauncher, iNoSelfBlastDamage, no_self_blast_dmg );
	if ( iNoSelfBlastDamage )
	{
		iCustomParticleIndex = GetParticleSystemIndex( "ExplosionCore_Wall_Jumper" );
	}

	int iLargeExplosion = 0;
	CALL_ATTRIB_HOOK_INT_ON_OTHER( m_hLauncher, iLargeExplosion, use_large_smoke_explosion );
	if ( iLargeExplosion > 0 )
	{
		DispatchParticleEffect( "explosionTrail_seeds_mvm", GetAbsOrigin(), GetAbsAngles() );
		DispatchParticleEffect( "fluidSmokeExpl_ring_mvm", GetAbsOrigin(), GetAbsAngles() );
	}

	if ( UseImpactNormal() )
	{
		if ( pTrace->m_pEnt && pTrace->m_pEnt->IsPlayer() )
		{
			TE_TFExplosion(filter, 0.0f, vecOrigin, GetImpactNormal(), GetWeaponID(), pTrace->m_pEnt->entindex(), ownerWeaponDefIndex, SPECIAL1, iCustomParticleIndex);
		}
		else
		{
			TE_TFExplosion(filter, 0.0f, vecOrigin, GetImpactNormal(), GetWeaponID(), kInvalidEHandleExplosion, ownerWeaponDefIndex, SPECIAL1, iCustomParticleIndex);
		}
	}
	else
	{
		if ( pTrace->m_pEnt && pTrace->m_pEnt->IsPlayer() )
		{
			TE_TFExplosion(filter, 0.0f, vecOrigin, pTrace->plane.normal, GetWeaponID(), pTrace->m_pEnt->entindex(), ownerWeaponDefIndex, SPECIAL1, iCustomParticleIndex);
		}
		else
		{
			TE_TFExplosion(filter, 0.0f, vecOrigin, pTrace->plane.normal, GetWeaponID(), kInvalidEHandleExplosion, ownerWeaponDefIndex, SPECIAL1, iCustomParticleIndex);
		}
	}

	// Use the thrower's position as the reported position
	Vector vecReported = GetThrower() ? GetThrower()->GetAbsOrigin() : vec3_origin;
	int nCustomDamage = GetDamageCustom();
	CTakeDamageInfo info( this, GetThrower(), m_hLauncher, GetBlastForce(), GetAbsOrigin(), m_flDamage, bitsDamageType, nCustomDamage, &vecReported );

	float flRadius = GetDamageRadius();

	CTFRadiusDamageInfo radiusinfo( &info, vecOrigin, flRadius, NULL, TF_GRENADE_JUMP_RADIUS );
	TFGameRules()->RadiusDamage( radiusinfo );

	// Don't decal players with scorch.
	if ( pTrace->m_pEnt && !pTrace->m_pEnt->IsPlayer() && ( iNoSelfBlastDamage == 0 ) )
	{
		UTIL_DecalTrace( pTrace, "Scorch" );
	}

	if ( GetEnemy() && GetThrower() )
	{
		CTFPlayer *pTarget = ToTFPlayer( GetEnemy() );
		if ( pTarget )
		{
			RecordEnemyPlayerHit( pTarget, true );
		}
	}

	SetThink( &CBaseGrenade::SUB_Remove );
	SetTouch( NULL );

	AddEffects( EF_NODRAW );
	SetAbsVelocity( vec3_origin );
	SetNextThink( gpGlobals->curtime );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFWeaponBaseGrenadeProj::OnTakeDamage( const CTakeDamageInfo &info )
{
	CTakeDamageInfo info2 = info;

	// Reduce explosion damage so that we don't get knocked too far
	if ( info.GetDamageType() & DMG_BLAST )
	{
		info2.ScaleDamageForce( 0.05 );
	}

	// We need to skip back to the base entity take damage, because
	// CBaseCombatCharacter doesn't, which prevents us from reacting
	// to physics impact damage.
	return CBaseEntity::OnTakeDamage( info2 );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFWeaponBaseGrenadeProj::DetonateThink( void )
{
	if ( !IsInWorld() )
	{
		Remove( );
		return;
	}

	if ( gpGlobals->curtime > m_flDetonateTime )
	{
		Detonate();
		return;
	}


	SetNextThink( gpGlobals->curtime + 0.2 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWeaponBaseGrenadeProj::Detonate( void )
{
	trace_t		tr;
	Vector		vecSpot;// trace starts here!

	SetThink( NULL );

	vecSpot = GetAbsOrigin() + Vector ( 0 , 0 , 8 );
	UTIL_TraceLine ( vecSpot, vecSpot + Vector ( 0, 0, -32 ), MASK_SHOT_HULL, this, COLLISION_GROUP_NONE, & tr);

	Explode( &tr, GetDamageType() );

	if ( GetShakeAmplitude() )
	{
		UTIL_ScreenShake( GetAbsOrigin(), GetShakeAmplitude(), 150.0, 1.0, GetShakeRadius(), SHAKE_START );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Sets the time at which the grenade will explode.
//-----------------------------------------------------------------------------
void CTFWeaponBaseGrenadeProj::SetDetonateTimerLength( float timer )
{
	float fFuseMult = 1.0f;
	if ( GetOwnerEntity() )
	{
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( GetOwnerEntity(), fFuseMult, fuse_mult );
	}
	
	m_flDetonateTime = gpGlobals->curtime + ( timer * fFuseMult );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFWeaponBaseGrenadeProj::ResolveFlyCollisionCustom( trace_t &trace, Vector &vecVelocity )
{
	//Assume all surfaces have the same elasticity
	float flSurfaceElasticity = 1.0;

	//Don't bounce off of players with perfect elasticity
	if( trace.m_pEnt && trace.m_pEnt->IsPlayer() )
	{
		flSurfaceElasticity = 0.3;
	}

#if 0
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
#endif

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

			// TFTODO: rotate around trace.plane.normal

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

#if 0
	// tell the bots a grenade has bounced
	CCSPlayer *player = ToCSPlayer(GetThrower());
	if ( player )
	{
		KeyValues *event = new KeyValues( "grenade_bounce" );
		event->SetInt( "userid", player->GetUserID() );
		gameeventmanager->FireEventServerOnly( event );
	}
#endif
}

bool CTFWeaponBaseGrenadeProj::ShouldNotDetonate( void )
{
	return InNoGrenadeZone( this );
}

void CTFWeaponBaseGrenadeProj::Destroy( bool bBlinkOut, bool bBreak )
{
	if ( bBreak )
	{
		CPVSFilter filter( GetAbsOrigin() );
		UserMessageBegin( filter, "BreakModelRocketDud" );
		WRITE_SHORT( GetModelIndex() );
		WRITE_VEC3COORD( GetAbsOrigin() );
		WRITE_ANGLES( GetAbsAngles() );
		MessageEnd();
	}

	// Kill it
	SetThink( &BaseClass::SUB_Remove );
	SetNextThink( gpGlobals->curtime );
	SetTouch( NULL );
	AddEffects( EF_NODRAW );

	if ( bBlinkOut )
	{
		// Sprite flash
		CSprite *pGlowSprite = CSprite::SpriteCreate( NOGRENADE_SPRITE, GetAbsOrigin(), false );
		if ( pGlowSprite )
		{
			pGlowSprite->SetTransparency( kRenderGlow, 255, 255, 255, 255, kRenderFxFadeFast );
			pGlowSprite->SetThink( &CSprite::SUB_Remove );
			pGlowSprite->SetNextThink( gpGlobals->curtime + 1.0 );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: This will hit only things that are in newCollisionGroup, but NOT in collisionGroupAlreadyChecked
//			Always ignores other grenade projectiles.
//-----------------------------------------------------------------------------
class CTraceFilterCollisionGrenades : public CTraceFilterEntitiesOnly
{
public:
	// It does have a base, but we'll never network anything below here..
	DECLARE_CLASS_NOBASE( CTraceFilterCollisionGrenades );

	CTraceFilterCollisionGrenades( const IHandleEntity *passentity, const IHandleEntity *passentity2 )
		: m_pPassEnt(passentity), m_pPassEnt2(passentity2)
	{
	}

	virtual bool ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask )
	{
		if ( !PassServerEntityFilter( pHandleEntity, m_pPassEnt ) )
			return false;
		CBaseEntity *pEntity = EntityFromEntityHandle( pHandleEntity );
		if ( pEntity )
		{
			if ( pEntity == m_pPassEnt2 )
				return false;
			if ( pEntity->GetCollisionGroup() == TF_COLLISIONGROUP_GRENADES )
				return false;
			if ( pEntity->GetCollisionGroup() == TFCOLLISION_GROUP_ROCKETS )
				return false;
			if ( pEntity->GetCollisionGroup() == COLLISION_GROUP_DEBRIS )
				return false;
			if ( pEntity->GetCollisionGroup() == TFCOLLISION_GROUP_RESPAWNROOMS )
				return false;
			if ( pEntity->GetCollisionGroup() == COLLISION_GROUP_NONE )
				return false;

			return true;
		}

		return true;
	}

protected:
	const IHandleEntity *m_pPassEnt;
	const IHandleEntity *m_pPassEnt2;
};


//-----------------------------------------------------------------------------
// Purpose: Grenades aren't solid to players, so players don't get stuck on
//			them when they're lying on the ground. We still want thrown grenades
//			to bounce of players though, so manually trace ahead and see if we'd
//			hit something that we'd like the grenade to "collide" with.
//-----------------------------------------------------------------------------
void CTFWeaponBaseGrenadeProj::VPhysicsUpdate( IPhysicsObject *pPhysics )
{
	BaseClass::VPhysicsUpdate( pPhysics );

	Vector vel;
	AngularImpulse angVel;
	pPhysics->GetVelocity( &vel, &angVel );

	Vector start = GetAbsOrigin();

	// find all entities that my collision group wouldn't hit, but COLLISION_GROUP_NONE would and bounce off of them as a ray cast
	CTraceFilterCollisionGrenades filter( this, GetThrower() );
	
	ITraceFilter *pFilterChain = NULL;
	CTraceFilterIgnoreFriendlyCombatItems filterCombatItems( this, COLLISION_GROUP_NONE, GetTeamNumber(), true );
	if ( TFGameRules() && TFGameRules()->GameModeUsesUpgrades() )
	{
		pFilterChain = &filterCombatItems;
	}
	CTraceFilterChain filterChain( &filter, pFilterChain );

	trace_t tr;
	UTIL_TraceLine( start, start + vel * gpGlobals->frametime, CONTENTS_HITBOX|CONTENTS_MONSTER|CONTENTS_SOLID, &filterChain, &tr );

	bool bHitEnemy = tr.m_pEnt && tr.m_pEnt->GetTeamNumber() == GetEnemyTeam( GetTeamNumber() );
	bool bHitFriendly = tr.m_pEnt && tr.m_pEnt->GetTeamNumber() == GetTeamNumber() && CanCollideWithTeammates();

	// Combat items are solid to enemy projectiles and bullets
	if ( bHitEnemy && tr.m_pEnt->IsCombatItem() )
	{
		if ( IsAllowedToExplode() )
		{
			Explode( &tr, GetDamageType() );
		}
		else
		{
			BounceOff( pPhysics );
		}
		return;
	}

	if ( tr.startsolid )
	{
		if ( bHitEnemy )
		{
			Touch( tr.m_pEnt );
		}
		else if ( !m_bInSolid && bHitFriendly )
		{
			BounceOff( pPhysics );
		}
		m_bInSolid = true;

		return;
	}

	m_bInSolid = false;

	if ( tr.DidHit() )
	{
		Touch( tr.m_pEnt );
		
		if ( bHitFriendly || bHitEnemy )
		{
			// reflect velocity around normal
			vel = -2.0f * tr.plane.normal * DotProduct(vel,tr.plane.normal) + vel;

			// absorb 80% in impact
			vel *= GetElasticity();

			if ( bHitEnemy == true )
			{
				vel *= 0.5f;
			}

			angVel *= -0.5f;
			pPhysics->SetVelocity( &vel, &angVel );
		}
	}
}


#endif
