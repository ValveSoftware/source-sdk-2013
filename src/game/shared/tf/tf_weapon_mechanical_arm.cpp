//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "tf_weapon_mechanical_arm.h"
#include "in_buttons.h"

#if !defined( CLIENT_DLL )
#include "tf_player.h"
#include "tf_gamestats.h"
#include "ilagcompensationmanager.h"
#include "particle_parse.h"
#include "tf_fx.h"
#include "tf_weapon_grenade_pipebomb.h"
#include "tf_team.h"
#include "tf_passtime_logic.h"
#include "tf_gamerules.h"
#else
#include "c_tf_player.h"
#endif



//=============================================================================
//
// tables.
//

IMPLEMENT_NETWORKCLASS_ALIASED( TFMechanicalArm, DT_TFMechanicalArm )

BEGIN_NETWORK_TABLE( CTFMechanicalArm, DT_TFMechanicalArm )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFMechanicalArm )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_mechanical_arm, CTFMechanicalArm );
PRECACHE_WEAPON_REGISTER( tf_weapon_mechanical_arm );


#define		AMMO_PER_PROJECTILE_SHOCK		5

const float tf_mecharm_orb_size = 100.f;
const float tf_mecharm_orb_speed = 700.f;
const int tf_mecharm_orb_cost = 65;
const int tf_mecharm_orb_zap_targets = 2;
const int tf_mecharm_orb_zap_damage = 15;
const float tf_mecharm_orb_lifetime = 1.2f;


//=============================================================================
//
// CTFMechanicalArm
//

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFMechanicalArm::CTFMechanicalArm()
{
#ifdef CLIENT_DLL
	m_pParticleBeamEffect = NULL;
	m_pParticleBeamSpark = NULL;
	m_pEffectOwner = NULL;
#endif // CLIENT_DLL
}

CTFMechanicalArm::~CTFMechanicalArm()
{
#ifdef CLIENT_DLL
	if ( m_pEffectOwner )
	{
		if ( m_pParticleBeamEffect )
		{
			m_pEffectOwner->ParticleProp()->StopEmissionAndDestroyImmediately( m_pParticleBeamEffect );
			m_pParticleBeamEffect = NULL;
		}

		if ( m_pParticleBeamSpark )
		{
			m_pEffectOwner->ParticleProp()->StopEmissionAndDestroyImmediately( m_pParticleBeamSpark );
			m_pParticleBeamSpark = NULL;
		}

		m_pEffectOwner = NULL;
	}
#endif // CLIENT_DLL
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMechanicalArm::Precache()
{
	BaseClass::Precache();

	PrecacheParticleSystem( "dxhr_arm_muzzleflash" );
	PrecacheParticleSystem( "dxhr_arm_muzzleflash2" );
	PrecacheParticleSystem( "dxhr_arm_impact" );
	PrecacheScriptSound( "Weapon_Upgrade.ExplosiveHeadshot" );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFMechanicalArm::ShockAttack( void )
{
	CTFPlayer *pOwner = ToTFPlayer( GetPlayerOwner() );
	if ( !pOwner )
		return false;

	if ( pOwner->GetWaterLevel() == WL_Eyes )
		return false;

	// Enough ammo to shock at least one target?
	if ( pOwner->GetAmmoCount( m_iPrimaryAmmoType ) < tf_mecharm_orb_cost )
		return false;

#ifdef GAME_DLL
	if ( pOwner->m_Shared.IsStealthed() )
	{
		pOwner->RemoveInvisibility();
	}

	// Remove the base cost for attempting to fire, regardless of what we hit
	pOwner->RemoveAmmo( tf_mecharm_orb_cost, m_iPrimaryAmmoType );
#endif

	return true;
}

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFMechanicalArm::IsValidVictim( CTFPlayer *pOwner, CBaseEntity *pTarget )
{
	if ( pTarget == NULL )
		return false;

	if ( pTarget == pOwner )
		return false;

	if ( pTarget->IsPlayer() && pTarget->GetTeamNumber() == TEAM_SPECTATOR )
		return false;

	if ( pTarget->GetTeamNumber() == pOwner->GetTeamNumber() )
		return false;

	if ( pTarget->IsPlayer() && !pTarget->IsAlive() )
		return false;

	if ( !pTarget->IsDeflectable() && !FClassnameIs( pTarget, "prop_physics" ) )
		return false;

	if ( pOwner->FVisible( pTarget, MASK_SOLID_BRUSHONLY ) == false )
		return false;

	if ( g_pPasstimeLogic && ( g_pPasstimeLogic->GetBall() == pTarget ) )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFMechanicalArm::ShockVictim( CTFPlayer *pOwner, CBaseEntity *pTarget )
{
	// Projectile
	if ( !pTarget->IsPlayer() )
	{
		pTarget->SetThink( &BaseClass::SUB_Remove );
		pTarget->SetNextThink( gpGlobals->curtime );
		pTarget->SetTouch( NULL );
		pTarget->AddEffects( EF_NODRAW );
		pTarget->RemoveFlag( FL_GRENADE );
	}
	
	// deal damage
	CTakeDamageInfo info;
	info.SetDamageType( DMG_SHOCK );
	info.SetAttacker( pOwner );
	info.SetInflictor( this );
	info.SetWeapon( this );
	info.SetDamage( 20 );
	info.SetDamagePosition( pTarget->WorldSpaceCenter() );
	pTarget->TakeDamage( info );

	// Achievement
	CTFGrenadePipebombProjectile *pPipebomb = dynamic_cast<CTFGrenadePipebombProjectile*>( pTarget );
	if ( pPipebomb && pPipebomb->HasStickyEffects() )
	{
		// If we are near a building, award achievement progress.
		CTFTeam *pTeam = pOwner->GetTFTeam();
		if ( pTeam )
		{
			for ( int j = 0; j < pTeam->GetNumObjects(); j++ )
			{
				CBaseObject *pTemp = pTeam->GetObject( j );
				if ( pTemp && ( pTemp->ObjectType() != OBJ_ATTACHMENT_SAPPER ) )
				{
					if ( ( pTemp->GetAbsOrigin().DistTo( pPipebomb->GetAbsOrigin() ) < 100 ) &&
						( pTemp->FVisible( pPipebomb, MASK_SOLID_BRUSHONLY ) ) )
					{
						pOwner->AwardAchievement( ACHIEVEMENT_TF_ENGINEER_DESTROY_STICKIES, 1 );
						break; // Only one award per sticky.
					}
				}
			}
		}
	}
}
#endif

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFMechanicalArm::SecondaryAttack( void )
{
	CTFPlayer *pOwner = ToTFPlayer( GetPlayerOwner() );
	if ( !pOwner )
		return;

	// Are we capable of firing again?
	if ( m_flNextSecondaryAttack > gpGlobals->curtime )
		return;

	if ( m_iPrimaryAmmoType == TF_AMMO_METAL )
	{
		if ( ( GetAmmoPerShot() > GetOwner()->GetAmmoCount( m_iPrimaryAmmoType ) ) ||
			( pOwner->GetWaterLevel() == WL_Eyes ) )
		{
			WeaponSound( EMPTY );
			m_flNextSecondaryAttack = gpGlobals->curtime + 0.67f;
			return;
		}
	}

	if ( !CanAttack() )
		return;

	if ( ShockAttack() )
	{
		WeaponSound( SPECIAL3 );
#ifdef GAME_DLL
		Vector vecForward, vecRight, vecUp;
		AngleVectors( pOwner->EyeAngles(), &vecForward, &vecRight, &vecUp );

		float fRight = 8.f;
		if ( IsViewModelFlipped() )
		{
			fRight *= -1;
		}
// 		Vector vecSrc = pOwner->Weapon_ShootPosition();
// 		vecSrc = vecSrc + ( vecUp * -9.0f ) + ( vecRight * 7.0f ) + ( vecForward * 3.0f );
		Vector vecSrc = pOwner->EyePosition()
			+ ( vecForward * 40.f )
			+ ( vecRight * 15.f )
			+ ( vecUp * -10.f );

		QAngle angForward = pOwner->EyeAngles();

		trace_t trace;
		Vector vecEye = pOwner->EyePosition();
		CTraceFilterSimple traceFilter( this, COLLISION_GROUP_PROJECTILE );
		UTIL_TraceHull( vecEye, vecSrc, -Vector( 8.f, 8.f, 8.f ), Vector( 8.f, 8.f, 8.f ), MASK_SOLID_BRUSHONLY, &traceFilter, &trace );
		if ( !trace.DidHit() )
		{
			CTFProjectile_MechanicalArmOrb *pOrb = static_cast< CTFProjectile_MechanicalArmOrb* >( CBaseEntity::CreateNoSpawn( "tf_projectile_mechanicalarmorb", vecSrc, angForward, pOwner ) );
			if ( pOrb )
			{
				pOrb->SetOwnerEntity( pOwner );
				pOrb->SetLauncher( this );

				Vector vForward;
				AngleVectors( angForward, &vForward, NULL, NULL );

				pOrb->SetAbsVelocity( vForward * tf_mecharm_orb_speed );

				pOrb->ChangeTeam( pOwner->GetTeamNumber() );
				pOrb->SetCritical( false );

				DispatchSpawn( pOrb );
			}
		}
#endif // GAME_DLL
	}
	else
	{
		WeaponSound( EMPTY );
	}

#ifdef CLIENT_DLL
	// Play an effect on the client so they have some kind of visual feedback that something happened
	int iParticleAttachment = LookupAttachment( "muzzle" );
	CNewParticleEffect* pEffect = ParticleProp()->Create( "dxhr_sniper_fizzle", PATTACH_POINT_FOLLOW, iParticleAttachment );
	ParticleProp()->AddControlPoint( pEffect, 1, this, PATTACH_POINT_FOLLOW, "muzzle" );
#endif

	SendWeaponAnim( ACT_VM_PRIMARYATTACK );

	pOwner->SetAnimation( PLAYER_ATTACK1 );

	m_flNextPrimaryAttack = gpGlobals->curtime + 0.67f;
	m_flNextSecondaryAttack = gpGlobals->curtime + 0.67f;
}

#ifdef CLIENT_DLL
void CTFMechanicalArm::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	UpdateParticleBeam();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFMechanicalArm::StopParticleBeam( void )
{
	if ( !m_pEffectOwner )
		return;

	// Different owners, kill the old effect
	if ( m_pParticleBeamEffect )
	{
		m_pEffectOwner->ParticleProp()->StopEmissionAndDestroyImmediately( m_pParticleBeamEffect );
		m_pParticleBeamEffect = NULL;
	}

	if ( m_pParticleBeamSpark )
	{
		m_pEffectOwner->ParticleProp()->StopEmissionAndDestroyImmediately( m_pParticleBeamSpark );
		m_pParticleBeamSpark = NULL;
	}

	m_pEffectOwner = NULL;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFMechanicalArm::UpdateParticleBeam()
{
	// Update Particle
	// If we are attacking, update the particle (make it render)
	CTFPlayer *pFiringPlayer = ToTFPlayer( GetOwnerEntity() );
	if ( !pFiringPlayer )
	{
		StopParticleBeam();
		return;
	}

	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	C_BaseEntity *pEffectOwner = this;
	if ( pLocalPlayer == pFiringPlayer )
	{
		pEffectOwner = pLocalPlayer->GetRenderedWeaponModel();
		if ( !pEffectOwner )
		{
			StopParticleBeam();
			return;
		}
	}

	if ( m_pEffectOwner && m_pEffectOwner != pEffectOwner )
	{
		StopParticleBeam();
		return;
	}

	if ( m_flNextSecondaryAttack > gpGlobals->curtime )
	{
		StopParticleBeam();
		return;
	}

	m_pEffectOwner = pEffectOwner;

	// Constantly perform the shock attack and update control points if attack is down and we've already fired
	if ( pFiringPlayer 
		&& pFiringPlayer->m_nButtons & IN_ATTACK 
		&& pFiringPlayer->GetActiveWeapon() == this 
		&& pFiringPlayer->GetWaterLevel() != WL_Eyes
		&& pFiringPlayer->m_flNextAttack < gpGlobals->curtime )
	{
		trace_t tr;
		Vector vecAiming;
		pFiringPlayer->EyeVectors( &vecAiming );
		Vector vecEnd = pFiringPlayer->EyePosition() + vecAiming * 256.0f;
		UTIL_TraceLine( pFiringPlayer->EyePosition(), vecEnd, ( MASK_SHOT & ~CONTENTS_HITBOX ), pFiringPlayer, DMG_GENERIC, &tr );

		// Line laser
		if ( !m_pParticleBeamEffect )
		{
			const char *pszEffectName = "dxhr_arm_muzzleflash2";
			m_pParticleBeamEffect = pEffectOwner->ParticleProp()->Create( pszEffectName, PATTACH_POINT_FOLLOW, "muzzle" );
		}
		if ( m_pParticleBeamEffect )
		{
			Vector vEndPos = tr.endpos - pFiringPlayer->GetAbsOrigin();
			pEffectOwner->ParticleProp()->AddControlPoint( m_pParticleBeamEffect, 1, pFiringPlayer, PATTACH_ABSORIGIN_FOLLOW, NULL, vEndPos );
		}

		// Spark
		if ( !m_pParticleBeamSpark && tr.m_pEnt && tr.m_pEnt->IsPlayer( ) )
		{
			m_pParticleBeamSpark = pEffectOwner->ParticleProp( )->Create( "dxhr_arm_impact", PATTACH_ABSORIGIN_FOLLOW );
		}
		else if ( m_pParticleBeamSpark && (!tr.m_pEnt || !tr.m_pEnt->IsPlayer() ) )
		{
			m_pEffectOwner->ParticleProp()->StopEmissionAndDestroyImmediately( m_pParticleBeamSpark );
			m_pParticleBeamSpark = NULL;
		}

		if ( m_pParticleBeamSpark )
		{
			Vector vEndPos = tr.endpos - pFiringPlayer->GetAbsOrigin();
			pEffectOwner->ParticleProp( )->AddControlPoint( m_pParticleBeamSpark, 1, pFiringPlayer, PATTACH_ABSORIGIN_FOLLOW, NULL, vEndPos );
		}
	}
	else if ( m_pEffectOwner )
	{
		StopParticleBeam( );
	}
}
#endif // CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFMechanicalArm::PrimaryAttack()
{
	CTFPlayer *pOwner = ToTFPlayer( GetPlayerOwner() );
	if ( !pOwner )
		return;

	float flFireDelay = ApplyFireDelay( m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_flTimeFireDelay );

	if ( m_iPrimaryAmmoType == TF_AMMO_METAL )
	{
		if ( ( GetAmmoPerShot() > GetOwner()->GetAmmoCount( m_iPrimaryAmmoType ) ) ||
			( pOwner->GetWaterLevel( ) == WL_Eyes ) )
		{
			WeaponSound( EMPTY );
			m_flNextPrimaryAttack = gpGlobals->curtime + flFireDelay;
			return;
		}
	}

	// Are we capable of firing again?
	if ( m_flNextPrimaryAttack > gpGlobals->curtime )
		return;

	// Get the player owning the weapon.
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( !pPlayer )
		return;

	if ( !CanAttack() )
		return;

#ifdef GAME_DLL
	CTF_GameStats.Event_PlayerFiredWeapon( pPlayer, false );

	int iAmmoPerShot = 0;
	CALL_ATTRIB_HOOK_INT( iAmmoPerShot, mod_ammo_per_shot );
	pOwner->RemoveAmmo( iAmmoPerShot, m_iPrimaryAmmoType );
	
	//int nAmmoToTake = bShocked ? 0 : GetAmmoPerShot();
	//pOwner->RemoveAmmo( nAmmoToTake, m_iPrimaryAmmoType );

	FireProjectile( pPlayer );
#endif

#ifdef CLIENT_DLL
	// Play an effect on the client so they have some kind of visual feedback that something happened
	int iParticleAttachment = LookupAttachment( "muzzle" );
	CNewParticleEffect* pEffect = ParticleProp()->Create( "dxhr_sniper_fizzle", PATTACH_POINT_FOLLOW, iParticleAttachment );
	ParticleProp()->AddControlPoint( pEffect, 1, this,  PATTACH_POINT_FOLLOW, "muzzle" );
#endif

	WeaponSound( SINGLE );

	// Set the weapon mode.
	m_iWeaponMode = TF_WEAPON_PRIMARY_MODE;

	SendWeaponAnim( ACT_VM_PRIMARYATTACK );

	pPlayer->SetAnimation( PLAYER_ATTACK1 );

	// Set next attack times.
	m_flNextPrimaryAttack = gpGlobals->curtime + flFireDelay;

	SetWeaponIdleTime( gpGlobals->curtime + SequenceDuration() );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int CTFMechanicalArm::GetAmmoPerShot( void )
{
	// Used by normal fire code, we only decrement ammo on ticks which uses an Attr
	return 0;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFMechanicalArm::UpdateBodygroups( CBaseCombatCharacter* pOwner, int iState )
{
	if ( !pOwner )
		return false;

	iState = pOwner->GetActiveWeapon() == this;

	bool res = BaseClass::UpdateBodygroups( pOwner, iState );

	CTFPlayer *pTFOwner = ToTFPlayer( pOwner );
	if ( pTFOwner )
	{
		CBaseViewModel *pVM = pTFOwner->GetViewModel();
		if ( pVM )
		{
			pVM->SetBodygroup( 1, iState );
		}
	}

	return res;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
IMPLEMENT_NETWORKCLASS_ALIASED( TFProjectile_MechanicalArmOrb, DT_TFProjectile_MechanicalArmOrb )
BEGIN_NETWORK_TABLE( CTFProjectile_MechanicalArmOrb, DT_TFProjectile_MechanicalArmOrb )
END_NETWORK_TABLE()

LINK_ENTITY_TO_CLASS( tf_projectile_mechanicalarmorb, CTFProjectile_MechanicalArmOrb );
PRECACHE_WEAPON_REGISTER( tf_projectile_mechanicalarmorb );

const char *pszParticleRed = "dxhr_lightningball_parent_red";
const char *pszParticleBlue = "dxhr_lightningball_parent_blue";

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFProjectile_MechanicalArmOrb::CTFProjectile_MechanicalArmOrb()
{
#ifdef CLIENT_DLL
	m_pTrailParticle = NULL;
#endif // CLIENT_DLL
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFProjectile_MechanicalArmOrb::~CTFProjectile_MechanicalArmOrb()
{
#ifdef CLIENT_DLL
	if ( m_pTrailParticle )
	{
		ParticleProp()->StopEmissionAndDestroyImmediately( m_pTrailParticle );
		m_pTrailParticle = NULL;
	}
#endif // CLIENT_DLL
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFProjectile_MechanicalArmOrb::Precache()
{
	BaseClass::Precache();
	
	PrecacheModel( "models/weapons/w_models/w_drg_ball.mdl" );
	PrecacheParticleSystem( pszParticleRed );
	PrecacheParticleSystem( pszParticleBlue );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFProjectile_MechanicalArmOrb::Spawn()
{
	BaseClass::Spawn();

	SetModel( "models/weapons/w_models/w_drg_ball.mdl" );

#ifdef GAME_DLL
	SetSolid( SOLID_BBOX );
	SetMoveType( MOVETYPE_FLY, MOVECOLLIDE_FLY_CUSTOM );
	SetSolidFlags( FSOLID_TRIGGER | FSOLID_NOT_SOLID );
	SetRenderMode( kRenderTransAlpha );
	SetRenderColorA( 1 );
 	CollisionProp()->SetCollisionBounds( Vector( -1.f, -1.f, -1.f ), Vector( 1.f, 1.f, 1.f ) );
	SetCollisionGroup( TFCOLLISION_GROUP_ROCKET_BUT_NOT_WITH_OTHER_ROCKETS );
	
	AddEFlags( EFL_NO_WATER_VELOCITY_CHANGE );
	AddEffects( EF_NOSHADOW );
	SetGravity( 0.f );
	SetTouch( &CTFBaseRocket::RocketTouch );
	AddFlag( FL_GRENADE );

	SetContextThink( &CTFProjectile_MechanicalArmOrb::OrbThink, gpGlobals->curtime, "OrbThink" );
	SetContextThink( &CTFProjectile_MechanicalArmOrb::ExplodeAndRemove, gpGlobals->curtime + tf_mecharm_orb_lifetime, "ExplodeAndRemoveThink" );
	m_flOrbNextAttackTime = -1.f;
#endif // GAME_DLL
}

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFProjectile_MechanicalArmOrb::ShouldProjectileIgnore( CBaseEntity *pOther )
{
	Assert( pOther );
	if ( !pOther )
		return true;

	if ( pOther->IsWorld() )
		return false;

	if ( GetOwnerEntity() == pOther )
		return true;

	if ( !pOther->IsSolid() || pOther->IsSolidFlagSet( FSOLID_VOLUME_CONTENTS ) )
		return true;

	if ( pOther->GetCollisionGroup() == TFCOLLISION_GROUP_RESPAWNROOMS )
		return true;

	if ( pOther->IsFuncLOD() )
		return true;

	const trace_t *pTrace = &CBaseEntity::GetTouchTrace();
	if ( pTrace->surface.flags & CONTENTS_LADDER )
		return true;

	if ( !ShouldTouchNonWorldSolid( pOther, pTrace ) )
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFProjectile_MechanicalArmOrb::RocketTouch( CBaseEntity *pOther )
{
	if ( pOther->IsPlayer() )
		return;

	const trace_t *pTrace = &CBaseEntity::GetTouchTrace();
	if ( pTrace->surface.flags & SURF_SKY )
	{
		UTIL_Remove( this );
		return;
	}

	if ( ShouldProjectileIgnore( pOther ) )
		return;

	// End if we run into something
	ExplodeAndRemove();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFProjectile_MechanicalArmOrb::ExplodeAndRemove( void )
{
	// Particle
	const char *pszParticleName = ( GetTeamNumber() == TF_TEAM_BLUE ) ? "drg_cow_explosioncore_normal_blue" : "drg_cow_explosioncore_normal";

	CPVSFilter filter( GetAbsOrigin() );
	TE_TFParticleEffect( filter, 0.f, pszParticleName, GetAbsOrigin(), vec3_angle );

	EmitSound( filter, entindex(), "Halloween.spell_lightning_impact" );

	// Go out with a bang
	CheckForPlayers( 16 );

#ifdef CLIENT_DLL
	if ( m_pTrailParticle )
	{
		ParticleProp()->StopEmissionAndDestroyImmediately( m_pTrailParticle );
		m_pTrailParticle = NULL;
	}
#endif // CLIENT_DLL

	SetContextThink( &CBaseGrenade::SUB_Remove, gpGlobals->curtime, "RemoveThink" );
	return;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFProjectile_MechanicalArmOrb::ZapPlayer( const CTakeDamageInfo &info, trace_t *pTrace, CTFPlayer *pTFPlayer )
{
	if ( !pTrace )
		return;

	if ( !pTFPlayer )
		return;

	// Shoot a beam at them
	CPVSFilter filter( pTFPlayer->WorldSpaceCenter() );
	Vector vStart = WorldSpaceCenter();
	Vector vEnd = pTFPlayer->EyePosition();
	const char *pszHitEffect = ( GetTeamNumber() == TF_TEAM_BLUE ) ? "dxhr_lightningball_hit_blue" : "dxhr_lightningball_hit_red";
	te_tf_particle_effects_control_point_t controlPoint = { PATTACH_ABSORIGIN, vEnd };
	TE_TFParticleEffectComplex( filter, 0.0f, pszHitEffect, vStart, QAngle( 0, 0, 0 ), NULL, &controlPoint, pTFPlayer, PATTACH_CUSTOMORIGIN );

	// Hurt 'em.
	Vector dir;
	AngleVectors( GetAbsAngles(), &dir );
	pTFPlayer->DispatchTraceAttack( info, dir, pTrace );
	ApplyMultiDamage();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFProjectile_MechanicalArmOrb::CheckForPlayers( int nNumToZap )
{
	CTFPlayer *pTFOwner = ToTFPlayer( GetOwnerEntity() );
	if ( !pTFOwner )
		return;

	CTakeDamageInfo info;
	info.SetAttacker( pTFOwner );
	info.SetInflictor( this );
	info.SetWeapon( GetLauncher() );
	info.SetDamage( tf_mecharm_orb_zap_damage );
	info.SetDamageCustom( TF_DMG_CUSTOM_PLASMA );
	info.SetDamagePosition( GetAbsOrigin() );
	info.SetDamageType( DMG_SHOCK );

	CBaseEntity *pListOfEntities[5];
	int iEntities = UTIL_EntitiesInSphere( pListOfEntities, 5, GetAbsOrigin(), tf_mecharm_orb_size, FL_CLIENT | FL_FAKECLIENT | FL_NPC );

	// Shuffle the list
	for ( int i = iEntities - 1; i > 0; --i )
	{
		V_swap( pListOfEntities[i], pListOfEntities[RandomInt( 0, i )] );
	}

	CTraceFilterIgnoreTeammates tracefilter( this, COLLISION_GROUP_NONE, GetTeamNumber() );

	// Zap as many targets as we're told to, if we can
	int nHits = 0;
	for ( int i = 0; i < iEntities && nHits < nNumToZap; ++i )
	{
		CBaseEntity* pTarget = pListOfEntities[i];
		if ( !pTarget )
			continue;

		if ( !pTarget->IsAlive() )
			continue;

		if ( pTFOwner->InSameTeam( pTarget ) )
			continue;

		if ( !FVisible( pTarget, MASK_OPAQUE ) )
			continue;

		CTFPlayer *pTFPlayer = ToTFPlayer( pTarget );
		if ( pTFPlayer )
		{
			if ( pTFPlayer->m_Shared.InCond( TF_COND_PHASE ) || pTFPlayer->m_Shared.InCond( TF_COND_PASSTIME_INTERCEPTION ) )
				continue;

			if ( pTFPlayer->m_Shared.IsInvulnerable() )
				continue;
		}

		trace_t trace;
		UTIL_TraceLine( GetAbsOrigin(), pTarget->GetAbsOrigin(), ( MASK_SHOT & ~( CONTENTS_HITBOX ) ), &tracefilter, &trace );
		if ( trace.DidHitWorld() )
			continue;

		ZapPlayer( info, &trace, pTFPlayer );
		++nHits;
	}

	// We zapped someone.  Play a sound
	if ( nHits > 0 )
	{
		EmitSound( "TFPlayer.MedicChargedDeath" );

		// If the owner is close, zap them too -- to punish shoot-the-floor patterns
		if ( ( pTFOwner->GetAbsOrigin() - GetAbsOrigin() ).LengthSqr() < Square( 80.f ) )
		{
			trace_t trace;
			UTIL_TraceLine( GetAbsOrigin(), pTFOwner->GetAbsOrigin(), ( MASK_SHOT & ~( CONTENTS_HITBOX ) ), &tracefilter, &trace );
			ZapPlayer( info, &trace, pTFOwner );
		}
	}

	m_flOrbNextAttackTime = gpGlobals->curtime + 0.15f;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFProjectile_MechanicalArmOrb::CheckForProjectiles( void )
{
	const int nMaxEnts = 16;
	const float flRadius = tf_mecharm_orb_size * 1.5f;	// Bloat it a little

	Vector vecPos = GetAbsOrigin();
	CBaseEntity	*pObjects[nMaxEnts];
	int nCount = UTIL_EntitiesInSphere( pObjects, nMaxEnts, vecPos, flRadius, FL_GRENADE );

	//NDebugOverlay::Sphere( vecPos, flRadius, 0, 255, 0, false, 0.35f );

	CTFPlayer *pTFOwner = ToTFPlayer( GetOwnerEntity() );

	bool bAnnihilate = false;

	// Destroy projectiles along the way
	for ( int i = 0; i < nCount; i++ )
	{
		if ( !pObjects[i] )
			continue;

		if ( pObjects[i] == this )
			continue;

		if ( pObjects[i]->InSameTeam( this ) )
			continue;

		if ( pObjects[i]->GetAbsOrigin().DistToSqr( GetAbsOrigin() ) > Square( tf_mecharm_orb_size ) )
			continue;

		if ( !FVisible( pObjects[i], MASK_SOLID ) )
			continue;

		CBaseProjectile *pProjectile = dynamic_cast< CBaseProjectile* >( pObjects[i] );
		if ( pProjectile && pProjectile->IsDestroyable( true ) )
		{
			CPVSFilter filter( WorldSpaceCenter() );
			const char *pszHitEffect = ( GetTeamNumber() == TF_TEAM_BLUE ) ? "dxhr_lightningball_hit_blue" : "dxhr_lightningball_hit_red";
			te_tf_particle_effects_control_point_t controlPoint = { PATTACH_ABSORIGIN, pObjects[i]->GetAbsOrigin() };
			TE_TFParticleEffectComplex( filter, 0.0f, pszHitEffect, WorldSpaceCenter(), QAngle( 0, 0, 0 ), NULL, &controlPoint, pProjectile, PATTACH_CUSTOMORIGIN );

			EmitSound( "Weapon_Upgrade.ExplosiveHeadshot" );

			// If we touch another orb, then we need to annihilate.  Destroy the other orb, and also destroy ourselves
			CTFProjectile_MechanicalArmOrb* pOtherOrb = dynamic_cast< CTFProjectile_MechanicalArmOrb* >( pProjectile );
			if ( pOtherOrb )
			{
				pOtherOrb->ExplodeAndRemove();
				bAnnihilate = true;
			}
			else
			{
				pProjectile->Destroy( true, false );
			}

			if ( pTFOwner )
			{
				CTF_GameStats.Event_PlayerAwardBonusPoints( pTFOwner, NULL, 2 );
			}
		}
	}

	// We hit another orb.  Destroy ourselves
	if ( bAnnihilate )
	{
		ExplodeAndRemove();
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFProjectile_MechanicalArmOrb::OrbThink( void )
{
	if ( gpGlobals->curtime >= m_flOrbNextAttackTime )
	{
		CheckForPlayers( tf_mecharm_orb_zap_targets );
	}

	CheckForProjectiles();

	SetContextThink( &CTFProjectile_MechanicalArmOrb::OrbThink, gpGlobals->curtime + 0.1f, "OrbThink" );
}
#endif

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFProjectile_MechanicalArmOrb::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	if ( updateType == DATA_UPDATE_CREATED )
	{
		m_iTeamNumPrev = GetTeamNumber();
		CreateTrails();
	}
	else if ( updateType == DATA_UPDATE_DATATABLE_CHANGED )
	{
		if ( m_iTeamNumPrev != GetTeamNumber() )
		{
			m_iTeamNumPrev = GetTeamNumber();

			if ( m_pTrailParticle )
			{
				ParticleProp()->StopEmissionAndDestroyImmediately( m_pTrailParticle );
				m_pTrailParticle = NULL;
			}
			CreateTrails();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFProjectile_MechanicalArmOrb::CreateTrails( void )
{
	BaseClass::CreateTrails();

	if ( !m_pTrailParticle )
	{
		m_pTrailParticle = ParticleProp()->Create( ( GetTeamNumber() == TF_TEAM_BLUE ? pszParticleBlue : pszParticleRed ), PATTACH_ABSORIGIN_FOLLOW );
	}
}
#endif // CLIENT_DLL
