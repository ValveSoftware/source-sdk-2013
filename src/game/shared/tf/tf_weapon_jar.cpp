//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "tf_weapon_jar.h"
#include "decals.h"

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
#include "tf_team.h"
#include "tf_gamestats.h"
#include "tf_gamerules.h"
#include "particle_parse.h"
#include "bone_setup.h"
#include "tf_flame.h"
#endif

//=============================================================================
//
// Weapon Jar tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFJar, DT_TFWeaponJar )

BEGIN_NETWORK_TABLE( CTFJar, DT_TFWeaponJar )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFJar )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_jar, CTFJar );
PRECACHE_WEAPON_REGISTER( tf_weapon_jar );

IMPLEMENT_NETWORKCLASS_ALIASED( TFJarMilk, DT_TFWeaponJarMilk )

BEGIN_NETWORK_TABLE( CTFJarMilk, DT_TFWeaponJarMilk )
END_NETWORK_TABLE()

LINK_ENTITY_TO_CLASS( tf_weapon_jar_milk, CTFJarMilk );
PRECACHE_WEAPON_REGISTER( tf_weapon_jar_milk );

IMPLEMENT_NETWORKCLASS_ALIASED( TFCleaver, DT_TFWeaponCleaver )

BEGIN_NETWORK_TABLE( CTFCleaver, DT_TFWeaponCleaver )
END_NETWORK_TABLE()

LINK_ENTITY_TO_CLASS( tf_weapon_cleaver, CTFCleaver );
PRECACHE_WEAPON_REGISTER( tf_weapon_cleaver );

// Projectile tables.
IMPLEMENT_NETWORKCLASS_ALIASED( TFProjectile_Jar, DT_TFProjectile_Jar )
BEGIN_NETWORK_TABLE( CTFProjectile_Jar, DT_TFProjectile_Jar )
END_NETWORK_TABLE()

LINK_ENTITY_TO_CLASS( tf_projectile_jar, CTFProjectile_Jar );
PRECACHE_WEAPON_REGISTER( tf_projectile_jar );

IMPLEMENT_NETWORKCLASS_ALIASED( TFProjectile_JarMilk, DT_TFProjectile_JarMilk )
BEGIN_NETWORK_TABLE( CTFProjectile_JarMilk, DT_TFProjectile_JarMilk )
END_NETWORK_TABLE()

LINK_ENTITY_TO_CLASS( tf_projectile_jar_milk, CTFProjectile_JarMilk );
PRECACHE_WEAPON_REGISTER( tf_projectile_jar_milk );

IMPLEMENT_NETWORKCLASS_ALIASED( TFProjectile_Cleaver, DT_TFProjectile_Cleaver )
BEGIN_NETWORK_TABLE( CTFProjectile_Cleaver, DT_TFProjectile_Cleaver )
END_NETWORK_TABLE()

LINK_ENTITY_TO_CLASS( tf_projectile_cleaver, CTFProjectile_Cleaver );
PRECACHE_WEAPON_REGISTER( tf_projectile_cleaver );

#define TF_JAR_LAUNCH_SPEED		1000.f
#define TF_CLEAVER_LAUNCH_SPEED		7000.f
#define TF_WEAPON_PEEJAR_MODEL	"models/weapons/c_models/urinejar.mdl"
#define TF_WEAPON_FESTIVE_PEEJAR_MODEL	"models/weapons/c_models/c_xms_urinejar.mdl"
#define TF_WEAPON_MILKJAR_MODEL	"models/workshop/weapons/c_models/c_madmilk/c_madmilk.mdl"
#define TF_WEAPON_CLEAVER_MODEL	"models/workshop_partner/weapons/c_models/c_sd_cleaver/c_sd_cleaver.mdl"
#define TF_WEAPON_CLEAVER_IMPACT_FLESH_SOUND	"Cleaver.ImpactFlesh"
#define TF_WEAPON_CLEAVER_IMPACT_WORLD_SOUND	"Cleaver.ImpactWorld"


//=============================================================================
//
// Weapon Jar functions.
//
	
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFJar::CTFJar()
{
}

float CTFJar::GetProjectileSpeed( void )
{
	return TF_JAR_LAUNCH_SPEED;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFJar::PrimaryAttack( void )
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return;

	int iJarCount = pPlayer->GetAmmoCount( m_iPrimaryAmmoType );
	if ( iJarCount == 0 )
		return;

	if ( ( pPlayer->GetWaterLevel() == WL_Eyes ) && !CanThrowUnderWater() )
		return;

	BaseClass::PrimaryAttack();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CBaseEntity *CTFJar::FireJar( CTFPlayer *pPlayer )
{
	StartEffectBarRegen();
	SetContextThink( &CTFJar::TossJarThink, gpGlobals->curtime + 0.1f, "TOSS_JAR_THINK" );

	return NULL;
}

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFProjectile_Jar *CTFJar::CreateJarProjectile( const Vector &position, const QAngle &angles, const Vector &velocity, 
												   const AngularImpulse &angVelocity, CBaseCombatCharacter *pOwner, const CTFWeaponInfo &weaponInfo )
{
	return CTFProjectile_Jar::Create( position, angles, velocity, angVelocity, pOwner, weaponInfo );
}
#endif

#ifdef GAME_DLL
Vector CTFJar::GetVelocityVector( const Vector &vecForward, const Vector &vecRight, const Vector &vecUp )
{
	return ( ( vecForward * GetProjectileSpeed() ) + ( vecUp * 200.0f ) + ( random->RandomFloat( -10.0f, 10.0f ) * vecRight ) +		
		( random->RandomFloat( -10.0f, 10.0f ) * vecUp ) );
}
#endif

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFJar::TossJarThink( void )
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return;

	PlayWeaponShootSound();

#ifdef GAME_DLL

	Vector vecForward, vecRight, vecUp;
	AngleVectors( pPlayer->EyeAngles(), &vecForward, &vecRight, &vecUp );

	float fRight = 8.f;
	if ( IsViewModelFlipped() )
	{
		fRight *= -1;
	}
	Vector vecSrc = pPlayer->Weapon_ShootPosition();
	vecSrc +=  vecForward * 16.0f + vecRight * fRight + vecUp * -6.0f;

	trace_t trace;	
	Vector vecEye = pPlayer->EyePosition();
	CTraceFilterSimple traceFilter( this, COLLISION_GROUP_NONE );
	UTIL_TraceHull( vecEye, vecSrc, -Vector(8,8,8), Vector(8,8,8), MASK_SOLID_BRUSHONLY, &traceFilter, &trace );

	// If we started in solid, don't let them fire at all
	if ( trace.startsolid )
		return;

	Vector vecVelocity = GetVelocityVector( vecForward, vecRight, vecUp );

	CTFProjectile_Jar *pProjectile = CreateJarProjectile( trace.endpos, pPlayer->EyeAngles(), vecVelocity, 
		GetAngularImpulse(), pPlayer, GetTFWpnData() );

	if ( pProjectile )
	{
		pProjectile->SetCritical( IsCurrentAttackACrit() );
		pProjectile->SetLauncher( this );
	}

	if ( ShouldSpeakWhenFiring() )
	{
		pPlayer->SpeakWeaponFire( MP_CONCEPT_JARATE_LAUNCH );
	}

#endif
}
//-----------------------------------------------------------------------------
void CTFJar::GetProjectileEntityName( CAttribute_String *attrProjectileEntityName )
{
	static CSchemaAttributeDefHandle pAttrDef_ProjectileEntityName( "projectile entity name" );
	CEconItemView *pItem = GetAttributeContainer()->GetItem();
	if ( pAttrDef_ProjectileEntityName && pItem )
	{
		//CAttribute_String attrProjectileEntityName;
		pItem->FindAttribute( pAttrDef_ProjectileEntityName, attrProjectileEntityName );
	}
}

#ifdef GAME_DLL

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFProjectile_Jar::CTFProjectile_Jar()
{
	m_vCollisionVelocity = Vector( 0,0,0 );
	m_iProjectileType = TF_PROJECTILE_JAR;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFProjectile_Jar::Precache()
{
	PrecacheModel( TF_WEAPON_PEEJAR_MODEL );
	PrecacheModel( TF_WEAPON_FESTIVE_PEEJAR_MODEL );
	PrecacheModel( "models/weapons/c_models/c_breadmonster/c_breadmonster.mdl" );

	PrecacheScriptSound( TF_WEAPON_PEEJAR_EXPLODE_SOUND );
	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFProjectile_Jar::SetCustomPipebombModel()
{
	// Check for Model Override
	int iProjectile = 0;
	CTFPlayer *pThrower = ToTFPlayer( GetThrower() );
	if ( pThrower && pThrower->GetActiveWeapon() )
	{
		CALL_ATTRIB_HOOK_INT_ON_OTHER( pThrower->GetActiveWeapon(), iProjectile, override_projectile_type );
		switch ( iProjectile )
		{
		case TF_PROJECTILE_FESTIVE_JAR :
			m_iProjectileType = iProjectile;
			SetModel( TF_WEAPON_FESTIVE_PEEJAR_MODEL );
			return;
		case TF_PROJECTILE_BREADMONSTER_JARATE:
		case TF_PROJECTILE_BREADMONSTER_MADMILK:
			m_iProjectileType = iProjectile;
			SetModel( "models/weapons/c_models/c_breadmonster/c_breadmonster.mdl" );
			return;
		}
	}
	
	SetModel( TF_WEAPON_PEEJAR_MODEL );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFProjectile_Jar* CTFProjectile_Jar::Create( const Vector &position, const QAngle &angles, 
												const Vector &velocity, const AngularImpulse &angVelocity, 
												CBaseCombatCharacter *pOwner, const CTFWeaponInfo &weaponInfo )
{
	CTFProjectile_Jar *pGrenade = static_cast<CTFProjectile_Jar*>( CBaseEntity::CreateNoSpawn( "tf_projectile_jar", position, angles, pOwner ) );
	if ( pGrenade )
	{
		// Set the pipebomb mode before calling spawn, so the model & associated vphysics get setup properly.
		pGrenade->SetPipebombMode();
		DispatchSpawn( pGrenade );

		pGrenade->InitGrenade( velocity, angVelocity, pOwner, weaponInfo );

#ifdef _X360 
		if ( pGrenade->m_iType != TF_GL_MODE_REMOTE_DETONATE )
		{
			pGrenade->SetDamage( TF_WEAPON_GRENADE_XBOX_DAMAGE );
		}
#endif
		pGrenade->m_flFullDamage = 0;

		pGrenade->ApplyLocalAngularVelocityImpulse( angVelocity );
	}

	return pGrenade;
}

extern void ExtinguishPlayer( CEconEntity *pExtinguisher, CTFPlayer *pOwner, CTFPlayer *pTarget, const char *pExtinguisherName );

void JarExplode( int iEntIndex, CTFPlayer *pAttacker, CBaseEntity *pOriginalWeapon, CBaseEntity *pWeapon, const Vector& vContactPoint, int iTeam, float flRadius, ETFCond cond, float flDuration, const char *pszImpactEffect, const char *pszExplodeSound )
{
	// Splash!
	CPVSFilter particleFilter( vContactPoint );
	TE_TFParticleEffect( particleFilter, 0.0, pszImpactEffect, vContactPoint, vec3_angle );

	// Explosion effect.
	CBroadcastRecipientFilter soundFilter;
	Vector vecOrigin = vContactPoint;
	CBaseEntity::EmitSound( soundFilter, iEntIndex, pszExplodeSound, &vecOrigin );

	// Treat this trace exactly like radius damage
	CTraceFilterIgnorePlayers traceFilter( pAttacker, COLLISION_GROUP_PROJECTILE );

	// Splash pee on everyone nearby.
	CBaseEntity *pListOfEntities[MAX_PLAYERS_ARRAY_SAFE];
	int iEntities = UTIL_EntitiesInSphere( pListOfEntities, ARRAYSIZE( pListOfEntities ), vContactPoint, flRadius, FL_CLIENT | FL_NPC );
	for ( int i = 0; i < iEntities; ++i )
	{
		CTFPlayer *pPlayer = ToTFPlayer( pListOfEntities[i] );
		if ( pPlayer )
		{
			if ( !pPlayer->IsAlive() )
				continue;

			// Do a quick trace to see if there's any geometry in the way.
			// Pee isn't stopped by other entities. Splishy splashy.
			trace_t trace;
			UTIL_TraceLine( vContactPoint, pPlayer->GetAbsOrigin(), ( MASK_SHOT & ~( CONTENTS_HITBOX ) ), &traceFilter, &trace );
			if ( trace.DidHitWorld() )
				continue;

			// Drench the target.
			if ( pPlayer->GetTeamNumber() != iTeam )
			{
				if ( TFGameRules() && TFGameRules()->IsTruceActive() )
					continue;

				if ( pPlayer->m_Shared.IsInvulnerable() )
					continue;

				if ( pPlayer->m_Shared.InCond( TF_COND_PHASE ) || pPlayer->m_Shared.InCond( TF_COND_PASSTIME_INTERCEPTION ) )
					continue;

				if ( !pPlayer->CanGetWet() )
					continue;

				pPlayer->m_Shared.AddCond( cond, flDuration, pAttacker );
				pPlayer->m_Shared.SetPeeAttacker( pAttacker );
				pPlayer->SpeakConceptIfAllowed( MP_CONCEPT_JARATE_HIT );

				if ( pAttacker )
				{
					if ( pPlayer->IsPlayerClass( TF_CLASS_SPY ) && pPlayer->m_Shared.GetPercentInvisible() == 1.0f )
					{
						pAttacker->AwardAchievement( ACHIEVEMENT_TF_SNIPER_JARATE_REVEAL_SPY );
					}

					float flStun = 1.0f;
					CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pAttacker, flStun, applies_snare_effect );
					if ( flStun != 1.0f )
					{
						pPlayer->m_Shared.StunPlayer( flDuration, flStun, TF_STUN_MOVEMENT, pAttacker );
					}

					// Stats tracking?
					if ( cond == TF_COND_URINE || cond == TF_COND_MAD_MILK || cond == TF_COND_GAS )
					{
						if ( TFGameRules() && TFGameRules()->IsPVEModeActive() )
						{
							// These if statements are intentionally split to avoid falling through to the normal kKillEaterEvent_PeeVictims event if we're in
							// IsPVEModeActive() but not a robot, or don't have the stun.
							if ( pPlayer->GetTeamNumber() == TF_TEAM_PVE_INVADERS && flStun != 1.0f )
							{
								EconEntity_OnOwnerKillEaterEvent( dynamic_cast<CEconEntity *>( pWeapon ), pAttacker, pPlayer, kKillEaterEvent_RobotsSlowed );
							}
						}
						else
						{
							EconEntity_OnOwnerKillEaterEvent( dynamic_cast<CEconEntity *>( pWeapon ), pAttacker, pPlayer, kKillEaterEvent_PeeVictims );
						}
					}

					// Tell the clients involved in the jarate
					CRecipientFilter involved_filter;
					involved_filter.AddRecipient( pPlayer );
					involved_filter.AddRecipient( pAttacker );
					UserMessageBegin( involved_filter, "PlayerJarated" );
						WRITE_BYTE( pAttacker->entindex() );
						WRITE_BYTE( pPlayer->entindex() );
					MessageEnd();

					const char *pszEvent = NULL;
					switch( cond )
					{
					case TF_COND_URINE:
						pszEvent = "jarate_attack";
						break;
					case TF_COND_MAD_MILK:
						pszEvent = "milk_attack";
						break;
					case TF_COND_GAS:
						pszEvent = "gas_attack";
						break;
					}

					if ( pszEvent && pszEvent[0] )
					{
						UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered \"%s\" against \"%s<%i><%s><%s>\" with \"%s\" (attacker_position \"%d %d %d\") (victim_position \"%d %d %d\")\n",    
							pAttacker->GetPlayerName(),
							pAttacker->GetUserID(),
							pAttacker->GetNetworkIDString(),
							pAttacker->GetTeam()->GetName(),
							pszEvent,
							pPlayer->GetPlayerName(),
							pPlayer->GetUserID(),
							pPlayer->GetNetworkIDString(),
							pPlayer->GetTeam()->GetName(),
							"tf_weapon_jar",
							(int)pAttacker->GetAbsOrigin().x, 
							(int)pAttacker->GetAbsOrigin().y,
							(int)pAttacker->GetAbsOrigin().z,
							(int)pPlayer->GetAbsOrigin().x, 
							(int)pPlayer->GetAbsOrigin().y,
							(int)pPlayer->GetAbsOrigin().z );
					}
				}
			}
			else
			{
				if ( pAttacker && pPlayer->m_Shared.InCond( TF_COND_BURNING ) )
				{
					ExtinguishPlayer( dynamic_cast<CEconEntity *>( pWeapon ), pAttacker, pPlayer, "tf_weapon_jar" );

					// Return some percentage of the jar to the thrown weapon if extinguishing an ally
					auto pLauncher = dynamic_cast< CTFWeaponBase* >( pOriginalWeapon );
					if ( pLauncher && pAttacker != pPlayer && pLauncher->HasEffectBarRegeneration() )
					{
						float fCooldown = 1.0f;
						CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pLauncher, fCooldown, extinguish_reduces_cooldown );
						fCooldown = 1.0f - fCooldown;
						if ( fCooldown > 0 )
						{
							if ( pLauncher->GetEffectBarProgress() < fCooldown )
							{
								float fDuration = pLauncher->GetEffectBarRechargeTime();
								float fIncrement = fDuration * fCooldown;
								pLauncher->DecrementBarRegenTime( fIncrement );
							}
						}
					}
				}
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFProjectile_Jar::Explode( trace_t *pTrace, int bitsDamageType )
{
	SetModelName( NULL_STRING );//invisible
	AddSolidFlags( FSOLID_NOT_SOLID );

	m_takedamage = DAMAGE_NO;

	// Pull out of the wall a bit.
	if ( pTrace->fraction != 1.0 )
	{
		SetAbsOrigin( pTrace->endpos + ( pTrace->plane.normal * 1.0f ) );
	}

	CTFPlayer *pThrower = ToTFPlayer( GetThrower() );
	JarExplode( entindex(), pThrower, GetOriginalLauncher(), GetLauncher(), GetAbsOrigin(), GetTeamNumber(), GetDamageRadius(), GetEffectCondition(), 10.f, GetImpactEffect(), GetExplodeSound() );

	// Debug radius draw.
	//DrawRadius( GetDamageRadius() );

	SetContextThink( &CBaseGrenade::SUB_Remove, gpGlobals->curtime, "RemoveThink" );
	SetTouch( NULL );

	AddEffects( EF_NODRAW );
	SetAbsVelocity( vec3_origin );
}	

//-----------------------------------------------------------------------------
void CTFProjectile_Jar::PipebombTouch( CBaseEntity *pOther )
{
	if ( pOther == GetThrower() )
		return;

	if ( !pOther->IsSolid() || pOther->IsSolidFlagSet( FSOLID_VOLUME_CONTENTS ) )
		return;

	if ( !pOther->IsWorld() && !pOther->IsPlayer() )
		return;

	// Don't collide with teammate if we're still in the grace period.
	if ( pOther->IsPlayer() && pOther->GetTeamNumber() == GetTeamNumber() && !CanCollideWithTeammates() )
	{
		// Exception to this rule - if we're a jar or milk, and our potential victim is on fire, then allow collision after all.
		// If we're a jar or milk, then still allow collision if our potential victim is on fire.
		if (m_iProjectileType == TF_PROJECTILE_JAR || m_iProjectileType == TF_PROJECTILE_JAR_MILK)
		{
			auto victim = ToTFPlayer(pOther);
			if (!victim->m_Shared.InCond(TF_COND_BURNING))
			{
				return;
			}
		}
		else
		{
			return;
		}
	}

	// Handle hitting skybox (disappear).
	trace_t pTrace;
	Vector velDir = GetAbsVelocity();
	if ( velDir.IsZero() && pOther && pOther->IsPlayer() )
	{
		velDir = pOther->WorldSpaceCenter() - GetAbsOrigin();
	}

	VectorNormalize( velDir );
	Vector vecSpot = GetAbsOrigin() - velDir * 32;
	UTIL_TraceLine( vecSpot, vecSpot + velDir * 64, MASK_SOLID, this, COLLISION_GROUP_NONE, &pTrace );

	if ( pTrace.fraction < 1.0 && pTrace.surface.flags & SURF_SKY )
	{
		UTIL_Remove( this );
		return;
	}

	// If we already touched a surface then we're not exploding on contact anymore.
	if ( m_bTouched == true )
		return;

	OnHit( pOther );
	if ( m_iProjectileType == TF_PROJECTILE_BREADMONSTER_JARATE || m_iProjectileType == TF_PROJECTILE_BREADMONSTER_MADMILK )
	{
		OnBreadMonsterHit( pOther, &pTrace );
	}

	if ( ExplodesOnHit() )
	{
		// Save this entity as enemy, they will take 100% damage if applicable
		m_hEnemy = pOther;
		Explode( &pTrace, GetDamageType() );
	}
}

//-----------------------------------------------------------------------------
void CTFProjectile_Jar::OnBreadMonsterHit( CBaseEntity *pOther, trace_t *pTrace )
{
	if ( m_iProjectileType != TF_PROJECTILE_BREADMONSTER_JARATE && m_iProjectileType != TF_PROJECTILE_BREADMONSTER_MADMILK )
		return;

	CTFPlayer *pVictim = ToTFPlayer( pOther );
	if ( !pVictim || pVictim->GetTeamNumber() == GetTeamNumber() )
		return;

	// This is a player on the other team, attach a breadmonster
	
	CTFPlayer *pOwner = ToTFPlayer( GetThrower() );

	// Attach Breadmonster to Victim
	CreateStickyAttachmentToTarget( pOwner, pVictim, pTrace );
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFProjectile_Jar::VPhysicsCollision( int index, gamevcollisionevent_t *pEvent )
{
	BaseClass::VPhysicsCollision( index, pEvent );

	int otherIndex = !index;
	CBaseEntity *pHitEntity = pEvent->pEntities[otherIndex];

	if ( !pHitEntity )
		return;

	if ( pHitEntity->IsWorld() )
	{
		OnHitWorld();
	}

	// Break if we hit the world.
	bool bIsDynamicProp = ( NULL != dynamic_cast<CDynamicProp *>( pHitEntity ) );
	if ( ExplodesOnHit() && pHitEntity && ( pHitEntity->IsWorld() || bIsDynamicProp ) )
	{
		// Explode immediately next frame. (Can't explode in the collision callback.)
		m_vCollisionVelocity = pEvent->preVelocity[index];
		SetContextThink( &CTFProjectile_Jar::VPhysicsCollisionThink, gpGlobals->curtime, "JarCollisionThink" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Handles exploding after a vphysics collision has happened.
// This prevents changing collision properties during the vphysics callback itself.
//-----------------------------------------------------------------------------
void CTFProjectile_Jar::VPhysicsCollisionThink( void )
{
	if ( !ExplodesOnHit() )
		return;

	trace_t pTrace;
	Vector velDir = m_vCollisionVelocity;
	VectorNormalize( velDir );
	Vector vecSpot = GetAbsOrigin() - velDir * 16;
	UTIL_TraceLine( vecSpot, vecSpot + velDir * 32, MASK_SOLID, this, COLLISION_GROUP_NONE, &pTrace );

	Explode( &pTrace, GetDamageType() );
}


//-----------------------------------------------------------------------------
bool CTFProjectile_Jar::PositionArrowOnBone( mstudiobbox_t *pBox, CBaseAnimating *pOtherAnim )
{
	CStudioHdr *pStudioHdr = pOtherAnim->GetModelPtr();
	if ( !pStudioHdr )
		return false;

	mstudiohitboxset_t *set = pStudioHdr->pHitboxSet( pOtherAnim->GetHitboxSet() );
	if ( !set )
		return false;
	if ( !set->numhitboxes )			// Target must have hit boxes.
		return false;

	if ( pBox->bone < 0 || pBox->bone >= pStudioHdr->numbones() )	// Bone index must be valid.
		return false;

	CBoneCache *pCache = pOtherAnim->GetBoneCache();
	if ( !pCache )
		return false;

	matrix3x4_t *bone_matrix = pCache->GetCachedBone( pBox->bone );
	if ( !bone_matrix )
		return false;

	Vector vecBoxAbsMins, vecBoxAbsMaxs;
	TransformAABB( *bone_matrix, pBox->bbmin, pBox->bbmax, vecBoxAbsMins, vecBoxAbsMaxs );

	// Adjust the arrow so it isn't exactly in the center of the box.
	Vector position;
	Vector vecDelta = vecBoxAbsMaxs - vecBoxAbsMins;
	float frand = (float)rand() / VALVE_RAND_MAX;
	position.x = vecBoxAbsMins.x + vecDelta.x*0.6f - vecDelta.x*frand*0.2f;
	frand = (float)rand() / VALVE_RAND_MAX;
	position.y = vecBoxAbsMins.y + vecDelta.y*0.6f - vecDelta.y*frand*0.2f;
	frand = (float)rand() / VALVE_RAND_MAX;
	position.z = vecBoxAbsMins.z + vecDelta.z*0.6f - vecDelta.z*frand*0.2f;
	SetAbsOrigin( position );

	return true;
}

//-----------------------------------------------------------------------------
void CTFProjectile_Jar::GetBoneAttachmentInfo( mstudiobbox_t *pBox, CBaseAnimating *pOtherAnim, Vector &bonePosition, QAngle &boneAngles, int &boneIndexAttached, int &physicsBoneIndex )
{
	// Find a bone to stick to.
	matrix3x4_t arrowWorldSpace;
	MatrixCopy( EntityToWorldTransform(), arrowWorldSpace );

	// Get the bone info so we can follow the bone.
	boneIndexAttached = pBox->bone;
	physicsBoneIndex = pOtherAnim->GetPhysicsBone( boneIndexAttached );
	matrix3x4_t boneToWorld;
	pOtherAnim->GetBoneTransform( boneIndexAttached, boneToWorld );

	Vector attachedBonePos;
	QAngle attachedBoneAngles;
	pOtherAnim->GetBonePosition( boneIndexAttached, attachedBonePos, attachedBoneAngles );

	// Transform my current position/orientation into the hit bone's space.
	matrix3x4_t worldToBone, localMatrix;
	MatrixInvert( boneToWorld, worldToBone );
	ConcatTransforms( worldToBone, arrowWorldSpace, localMatrix );
	MatrixAngles( localMatrix, boneAngles, bonePosition );
}

//-----------------------------------------------------------------------------
void CTFProjectile_Jar::CreateStickyAttachmentToTarget( CTFPlayer *pOwner, CTFPlayer *pVictim, trace_t *trace )
{
	// Dont stick to the sky!
	if ( trace->surface.flags & SURF_SKY )
	{
		return;
	}

	// If I hit a player, remove the jar and replace with the face eater version
	CStudioHdr *pStudioHdr = NULL;
	mstudiohitboxset_t *set = NULL;
	pStudioHdr = pVictim->GetModelPtr();
	if ( pStudioHdr )
	{
		set = pStudioHdr->pHitboxSet( pVictim->GetHitboxSet() );
	}

	// Look for nearest hitbox
	mstudiobbox_t *closest_box = NULL;
	if ( trace->m_pEnt && trace->m_pEnt->GetTeamNumber() != GetTeamNumber() )
	{
		closest_box = set->pHitbox( trace->hitbox );
	}

	if ( closest_box )
	{
		if ( !PositionArrowOnBone( closest_box, pVictim ) )
			return;

		// See if we're supposed to stick in the target.
		Vector bonePosition = vec3_origin;
		QAngle boneAngles = QAngle( 0, 0, 0 );
		int boneIndexAttached = -1;
		int physicsBoneIndex = -1;

		GetBoneAttachmentInfo( closest_box, pVictim, bonePosition, boneAngles, boneIndexAttached, physicsBoneIndex );

		IGameEvent * event = gameeventmanager->CreateEvent( "arrow_impact" );
		if ( event )
		{
			event->SetInt( "attachedEntity", pVictim->entindex() );
			event->SetInt( "shooter", pOwner->entindex() );
			event->SetInt( "attachedEntity", pVictim->entindex() );
			event->SetInt( "boneIndexAttached", boneIndexAttached );
			event->SetFloat( "bonePositionX", bonePosition.x );
			event->SetFloat( "bonePositionY", bonePosition.y );
			event->SetFloat( "bonePositionZ", bonePosition.z );
			event->SetFloat( "boneAnglesX", boneAngles.x );
			event->SetFloat( "boneAnglesY", boneAngles.y );
			event->SetFloat( "boneAnglesZ", boneAngles.z );
			event->SetInt( "projectileType", GetProjectileType() );
			event->SetBool( "isCrit", IsCritical() );
			gameeventmanager->FireEvent( event );
		}
	}
}

#endif

#ifdef CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CTFProjectile_Jar::GetTrailParticleName( void )
{
	if ( GetTeamNumber() == TF_TEAM_BLUE )
	{
		return "peejar_trail_blu";
	}
	else
	{
		return "peejar_trail_red";
	}
}

#endif

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFProjectile_Jar *CTFJarMilk::CreateJarProjectile( const Vector &position, const QAngle &angles, const Vector &velocity, 
												 const AngularImpulse &angVelocity, CBaseCombatCharacter *pOwner, const CTFWeaponInfo &weaponInfo )
{
	return CTFProjectile_JarMilk::Create( position, angles, velocity, angVelocity, pOwner, weaponInfo );
}
#endif

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFProjectile_JarMilk::Precache()
{
	PrecacheModel( TF_WEAPON_MILKJAR_MODEL );

	BaseClass::Precache();
}
#ifdef GAME_DLL

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFProjectile_JarMilk* CTFProjectile_JarMilk::Create( const Vector &position, const QAngle &angles, 
											 const Vector &velocity, const AngularImpulse &angVelocity, 
											 CBaseCombatCharacter *pOwner, const CTFWeaponInfo &weaponInfo )
{
	CTFProjectile_JarMilk *pGrenade = static_cast<CTFProjectile_JarMilk*>( CBaseEntity::CreateNoSpawn( "tf_projectile_jar_milk", position, angles, pOwner ) );
	if ( pGrenade )
	{
		// Set the pipebomb mode before calling spawn, so the model & associated vphysics get setup properly.
		pGrenade->SetPipebombMode();
		DispatchSpawn( pGrenade );

		pGrenade->InitGrenade( velocity, angVelocity, pOwner, weaponInfo );

#ifdef _X360 
		if ( pGrenade->m_iType != TF_GL_MODE_REMOTE_DETONATE )
		{
			pGrenade->SetDamage( TF_WEAPON_GRENADE_XBOX_DAMAGE );
		}
#endif
		pGrenade->m_flFullDamage = 0;

		pGrenade->ApplyLocalAngularVelocityImpulse( angVelocity );
	}

	return pGrenade;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFProjectile_JarMilk::SetCustomPipebombModel()
{
	// Check for Model Override
	int iProjectile = 0;
	CTFPlayer *pThrower = ToTFPlayer( GetThrower() );
	if ( pThrower && pThrower->GetActiveWeapon() )
	{
		CALL_ATTRIB_HOOK_INT_ON_OTHER( pThrower->GetActiveWeapon(), iProjectile, override_projectile_type );
		switch ( iProjectile )
		{
		case TF_PROJECTILE_BREADMONSTER_JARATE:
		case TF_PROJECTILE_BREADMONSTER_MADMILK:
			m_iProjectileType = iProjectile;
			SetModel( "models/weapons/c_models/c_breadmonster/c_breadmonster_milk.mdl" );
			return;
		}
	}

	SetModel( TF_WEAPON_MILKJAR_MODEL );
}
#endif

#ifdef CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
const char* CTFJarMilk::ModifyEventParticles( const char* token )
{
	if ( FStrEq( token, "energydrink_splash") )
	{
		CEconItemView *pItem = m_AttributeManager.GetItem();
		int iSystems = pItem->GetStaticData()->GetNumAttachedParticles( GetTeamNumber() );
		for ( int i = 0; i < iSystems; i++ )
		{
			attachedparticlesystem_t *pSystem = pItem->GetStaticData()->GetAttachedParticleData( GetTeamNumber(),i );
			if ( pSystem->iCustomType == 1 )
			{
				return pSystem->pszSystemName;
			}
		}
	}

	return BaseClass::ModifyEventParticles( token );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFJarMilk::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	CTFPlayer *pOwner = ToTFPlayer( GetPlayerOwner() );
	if ( pOwner && pOwner->IsLocalPlayer() )
	{
		C_BaseEntity *pParticleEnt = pOwner->GetViewModel(0);
		if ( pParticleEnt )
		{
			pOwner->StopViewModelParticles( pParticleEnt );
		}
	}

	return BaseClass::Holster( pSwitchingTo );
}

#endif

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
Vector CTFCleaver::GetVelocityVector( const Vector &vecForward, const Vector &vecRight, const Vector &vecUp )
{
	Vector vecVelocity;

	// Calculate the initial impulse on the item.
	vecVelocity = vecForward * 10 + vecUp;
	VectorNormalize( vecVelocity );
	vecVelocity *= 3000;

	return vecVelocity;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFProjectile_Jar *CTFCleaver::CreateJarProjectile( const Vector &position, const QAngle &angles, const Vector &velocity, 
	const AngularImpulse &angVelocity, CBaseCombatCharacter *pOwner, const CTFWeaponInfo &weaponInfo )
{
	return CTFProjectile_Cleaver::Create( position, angles, velocity, angVelocity, pOwner, weaponInfo, GetSkin() );
}
#endif

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
float CTFCleaver::GetProjectileSpeed( void )
{
	return TF_CLEAVER_LAUNCH_SPEED;
}

//-----------------------------------------------------------------------------
void CTFCleaver::SecondaryAttack( void )
{
	PrimaryAttack();
}

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
const char* CTFCleaver::ModifyEventParticles( const char* token )
{
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFCleaver::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	return BaseClass::Holster( pSwitchingTo );
}
#endif

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFProjectile_Cleaver::Precache()
{
	PrecacheModel( TF_WEAPON_CLEAVER_MODEL );
	PrecacheScriptSound( TF_WEAPON_CLEAVER_IMPACT_FLESH_SOUND );
	PrecacheScriptSound( TF_WEAPON_CLEAVER_IMPACT_WORLD_SOUND );

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFProjectile_Cleaver::SetCustomPipebombModel()
{
	SetModel( TF_WEAPON_CLEAVER_MODEL );
}

CTFProjectile_Cleaver::CTFProjectile_Cleaver()
{
#ifdef GAME_DLL
	m_bHitPlayer = false;
	m_bSoundPlayed = false;
#endif
}

#ifdef GAME_DLL
#define FLIGHT_TIME_TO_REDUCE_COOLDOWN	0.5f
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFProjectile_Cleaver::OnHit( CBaseEntity *pOther )
{
	SetModelName( NULL_STRING );//invisible
	AddSolidFlags( FSOLID_NOT_SOLID );

	CTFPlayer *pOwner = ToTFPlayer( GetThrower() );
	if ( !pOwner )
		return;

	if ( !pOther || !pOther->IsPlayer() )
		return;

	CTFPlayer *pPlayer = ToTFPlayer( pOther );
	if ( !pPlayer )
		return;

	// Can't bleed an invul player.
	if ( pPlayer->m_Shared.IsInvulnerable() || pPlayer->m_Shared.InCond( TF_COND_INVULNERABLE_WEARINGOFF ) )
		return;

	if ( pPlayer->GetTeamNumber() == pOwner->GetTeamNumber() )
		return;

	if ( TFGameRules() && TFGameRules()->IsTruceActive() && pOwner->IsTruceValidForEnt() )
	{
		RemoveCleaver();
		return;
	}

	CBaseEntity *pInflictor = GetLauncher();

	float flLifeTime = gpGlobals->curtime - m_flCreationTime;
	if ( flLifeTime >= FLIGHT_TIME_TO_REDUCE_COOLDOWN )
	{
		auto pLauncher = dynamic_cast<CTFWeaponBase*>( pInflictor );
		if ( pLauncher && pOwner != pPlayer && pLauncher->HasEffectBarRegeneration() )
		{
			pLauncher->DecrementBarRegenTime( 1.5f );
		}
	}

	// just do the bleed effect directly since the bleed
	// attribute comes from the inflictor, which is the cleaver.
	pPlayer->m_Shared.MakeBleed( pOwner, (CTFCleaver *)GetLauncher(), 5.f );

	// Give 'em a love tap.
	const trace_t *pTrace = &CBaseEntity::GetTouchTrace();
	trace_t *pNewTrace = const_cast<trace_t*>( pTrace );

	CTakeDamageInfo info;
	info.SetAttacker( pOwner );
	info.SetInflictor( pInflictor ); 
	info.SetWeapon( pInflictor );
	info.SetDamage( GetDamage() );
	info.SetDamageCustom( TF_DMG_CUSTOM_CLEAVER );
	info.SetDamagePosition( GetAbsOrigin() );
	int iDamageType = GetDamageType();
	if ( IsCritical() )
	{
		iDamageType |= DMG_CRITICAL;
	}
	info.SetDamageType( iDamageType );

	// Hurt 'em.
	Vector dir;
	AngleVectors( GetAbsAngles(), &dir );
	pPlayer->DispatchTraceAttack( info, dir, pNewTrace );
	ApplyMultiDamage();

	// sound effects
	EmitSound_t params;
	params.m_flSoundTime = 0;
	params.m_pflSoundDuration = 0;
	params.m_pSoundName = TF_WEAPON_CLEAVER_IMPACT_FLESH_SOUND;

	CPASFilter filter( GetAbsOrigin() );
	filter.RemoveRecipient( pOwner );
	EmitSound( filter, entindex(), params );

	CSingleUserRecipientFilter attackerFilter( pOwner );
	EmitSound( attackerFilter, pOwner->entindex(), params );

	RemoveCleaver();

	m_bHitPlayer = true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFProjectile_Cleaver::Explode( trace_t *pTrace, int bitsDamageType )
{
	if ( !m_bHitPlayer )
	{
		if ( !m_bSoundPlayed )
		{
			EmitSound( TF_WEAPON_CLEAVER_IMPACT_WORLD_SOUND );
			m_bSoundPlayed = true;
		}

		SetContextThink( &CBaseGrenade::SUB_Remove, gpGlobals->curtime + 2, "RemoveThink" );
		SetTouch( NULL );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFProjectile_Cleaver::Detonate( void )
{
	trace_t		tr;
	Vector		vecSpot;// trace starts here!

	SetThink( NULL );

	vecSpot = GetAbsOrigin() + Vector ( 0 , 0 , 8 );
	UTIL_TraceLine ( vecSpot, vecSpot + Vector ( 0, 0, -32 ), MASK_SHOT_HULL, this, COLLISION_GROUP_NONE, & tr);

	Explode( &tr, GetDamageType() );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFProjectile_Cleaver* CTFProjectile_Cleaver::Create( const Vector &position, const QAngle &angles, 
	const Vector &velocity, const AngularImpulse &angVelocity, 
	CBaseCombatCharacter *pOwner, const CTFWeaponInfo &weaponInfo, int nSkin )
{
	CTFProjectile_Cleaver *pGrenade = static_cast<CTFProjectile_Cleaver*>( CBaseEntity::CreateNoSpawn( "tf_projectile_cleaver", position, angles, pOwner ) );
	if ( pGrenade )
	{
		// Set the pipebomb mode before calling spawn, so the model & associated vphysics get setup properly.
		pGrenade->SetPipebombMode();
		DispatchSpawn( pGrenade );

		pGrenade->m_nSkin = nSkin;

		pGrenade->InitGrenade( velocity, angVelocity, pOwner, weaponInfo );

		pGrenade->m_flFullDamage = 0;

		pGrenade->ApplyLocalAngularVelocityImpulse( angVelocity );
	}

	return pGrenade;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFProjectile_Cleaver::RemoveCleaver( void )
{
	AddEffects( EF_NODRAW );
	SetAbsVelocity( vec3_origin );
	SetContextThink( &CBaseGrenade::SUB_Remove, gpGlobals->curtime + 2, "RemoveThink" );
	SetTouch( NULL );
}

#else
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CTFProjectile_Cleaver::GetTrailParticleName( void )
{
	if ( GetTeamNumber() == TF_TEAM_BLUE )
	{
		return "peejar_trail_blu_glow";
	}
	else
	{
		return "peejar_trail_red_glow";
	}
}

#endif // GAME_DLL
