//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================
#include "cbase.h"
#include "tf_weapon_minigun.h"
#include "decals.h"
#include "in_buttons.h"
#include "tf_fx_shared.h"
#include "debugoverlay_shared.h"
#include "tf_gamerules.h"

// Client specific.
#ifdef CLIENT_DLL
#include "c_tf_player.h"
#include "soundenvelope.h"
#include "achievementmgr.h"
#include "baseachievement.h"
#include "achievements_tf.h"
#include "prediction.h"
#include "clientmode_tf.h"
#include "bone_setup.h"
// NVNT haptics system interface
#include "haptics/ihaptics.h"
// Server specific.
#else
#include "tf_player.h"
#include "particle_parse.h"
#include "tf_gamestats.h"
#include "baseprojectile.h"
#endif

#define MAX_BARREL_SPIN_VELOCITY	20
#define TF_MINIGUN_SPINUP_TIME 0.75f
#define TF_MINIGUN_PENALTY_PERIOD 1.f

//=============================================================================
//
// Weapon Minigun tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFMinigun, DT_WeaponMinigun )

BEGIN_NETWORK_TABLE( CTFMinigun, DT_WeaponMinigun )
// Client specific.
#ifdef CLIENT_DLL
	RecvPropInt( RECVINFO( m_iWeaponState ) ),
	RecvPropBool( RECVINFO( m_bCritShot ) )
// Server specific.
#else
	SendPropInt( SENDINFO( m_iWeaponState ), 4, SPROP_UNSIGNED | SPROP_CHANGES_OFTEN ),
	SendPropBool( SENDINFO( m_bCritShot ) )
#endif
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CTFMinigun )
	DEFINE_FIELD(  m_iWeaponState, FIELD_INTEGER ),
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS( tf_weapon_minigun, CTFMinigun );
PRECACHE_WEAPON_REGISTER( tf_weapon_minigun );


// Server specific.
#ifndef CLIENT_DLL
BEGIN_DATADESC( CTFMinigun )
END_DATADESC()
#endif

//=============================================================================
//
// Weapon Minigun functions.
//

//-----------------------------------------------------------------------------
// Purpose: Constructor.
//-----------------------------------------------------------------------------
CTFMinigun::CTFMinigun()
{
#ifdef CLIENT_DLL
	m_pSoundCur = NULL;

	m_hEjectBrassWeapon = NULL;
	m_pEjectBrassEffect = NULL;
	m_iEjectBrassAttachment = -1;

	m_hMuzzleEffectWeapon = NULL;
	m_pMuzzleEffect = NULL;
	m_iMuzzleAttachment = -1;

	m_nShotsFired = 0;

	ListenForGameEvent( "teamplay_round_active" );
	ListenForGameEvent( "localplayer_respawn" );

	m_bRageDraining = false;
	m_bPrevRageDraining = false;
#endif
	m_bAttack3Down = false;

	WeaponReset();
}

//-----------------------------------------------------------------------------
// Purpose: Destructor.
//-----------------------------------------------------------------------------
CTFMinigun::~CTFMinigun()
{
	WeaponReset();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMinigun::WeaponReset( void )
{
	BaseClass::WeaponReset();

	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( pPlayer )
	{
		pPlayer->m_Shared.RemoveCond( TF_COND_AIMING );
		pPlayer->TeamFortress_SetSpeed();

#ifdef GAME_DLL
		pPlayer->ClearWeaponFireScene();
		m_flAegisCheckTime = 0.0f;
#endif

		m_flNextRingOfFireAttackTime = 0.0f;
		m_flLastAmmoDrainTime = gpGlobals->curtime;
		m_flAccumulatedAmmoDrain = 0.0f;
	}

	SetWeaponState( AC_STATE_IDLE );
	m_iWeaponMode = TF_WEAPON_PRIMARY_MODE;
	m_bCritShot = false;
	m_flStartedFiringAt = -1.0f;
	m_flStartedWindUpAt = -1.f;
	m_flNextFiringSpeech = 0.0f;

	m_flBarrelAngle = 0.0f;

	m_flBarrelCurrentVelocity = 0.0f;
	m_flBarrelTargetVelocity = 0.0f;

#ifdef CLIENT_DLL
	if ( m_pSoundCur )
	{
		CSoundEnvelopeController::GetController().SoundDestroy( m_pSoundCur );
		m_pSoundCur = NULL;
	}

	m_iMinigunSoundCur = -1;
	m_flMinigunSoundCurrentPitch = 1.0f;

	StopMuzzleEffect();
	StopBrassEffect();
#endif
}

#ifdef GAME_DLL
int CTFMinigun::UpdateTransmitState( void )
{
	// ALWAYS transmit to all clients.
	return SetTransmitState( FL_EDICT_ALWAYS );
}
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMinigun::Precache( void )
{
	PrecacheScriptSound( "Halloween.HeadlessBossAxeHitWorld" );

	// FIXME: Do we still need these??
	PrecacheScriptSound( "MVM.GiantHeavyGunWindUp" );
	PrecacheScriptSound( "MVM.GiantHeavyGunWindDown" );
	PrecacheScriptSound( "MVM.GiantHeavyGunFire" );
	PrecacheScriptSound( "MVM.GiantHeavyGunSpin" );

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMinigun::ItemPostFrame( void )
{
	// Prevent base code from ever playing empty sounds, minigun handles them manually.
	m_flNextEmptySoundTime = gpGlobals->curtime + 1.0;

#ifdef GAME_DLL
	CBasePlayer *pOwner = GetPlayerOwner();
	if ( pOwner )
	{
		if ( ( pOwner->m_nButtons & IN_ATTACK3 ) && !m_bAttack3Down )
		{
			ActivatePushBackAttackMode();
			m_bAttack3Down = true;
		}
		else if ( !( pOwner->m_nButtons & IN_ATTACK3 ) && m_bAttack3Down )
		{
			m_bAttack3Down = false;
		}
	}
#endif // GAME_DLL

	BaseClass::ItemPostFrame();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFMinigun::PrimaryAttack()
{
	SharedAttack();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMinigun::SharedAttack()
{
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( !pPlayer )
		return;

	if ( !CanAttack() )
	{
		WeaponIdle();
		return;
	}

#ifdef CLIENT_DLL
	m_bRageDraining = pPlayer->m_Shared.IsRageDraining();
#endif // CLIENT_DLL

	if ( pPlayer->m_nButtons & IN_ATTACK )
	{
		m_iWeaponMode = TF_WEAPON_PRIMARY_MODE;
	}
	else if ( pPlayer->m_nButtons & IN_ATTACK2 )
	{
		m_iWeaponMode = TF_WEAPON_SECONDARY_MODE;
	}

	switch ( m_iWeaponState )
	{
	default:
	case AC_STATE_IDLE:
		{
			// Removed the need for cells to powerup the AC
			WindUp();

			float flSpinUpTime = TF_MINIGUN_SPINUP_TIME;
			CALL_ATTRIB_HOOK_FLOAT( flSpinUpTime, mult_minigun_spinup_time );

			float flSpinTimeMultiplier = Max( flSpinUpTime, 0.00001f );
			if ( pPlayer->GetViewModel( 0 ) )
			{
				pPlayer->GetViewModel( 0 )->SetPlaybackRate( TF_MINIGUN_SPINUP_TIME / flSpinTimeMultiplier );
			}
			if ( pPlayer->GetViewModel( 1 ) )
			{
				pPlayer->GetViewModel( 1 )->SetPlaybackRate( TF_MINIGUN_SPINUP_TIME / flSpinTimeMultiplier );
			}

			m_flNextPrimaryAttack = gpGlobals->curtime + flSpinUpTime;
			m_flNextSecondaryAttack = gpGlobals->curtime + flSpinUpTime;
			m_flTimeWeaponIdle = gpGlobals->curtime + flSpinUpTime;
			m_flStartedFiringAt = -1.f;
			pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRE );
			break;
		}
	case AC_STATE_STARTFIRING:
		{
			// Start playing the looping fire sound
			if ( m_flNextPrimaryAttack <= gpGlobals->curtime )
			{
				if ( m_iWeaponMode == TF_WEAPON_SECONDARY_MODE )
				{
					SetWeaponState( AC_STATE_SPINNING );
				}
				else
				{
					SetWeaponState( AC_STATE_FIRING );
				}

#ifdef GAME_DLL
				if ( m_iWeaponState == AC_STATE_SPINNING )
				{
					pPlayer->SpeakWeaponFire( MP_CONCEPT_WINDMINIGUN );
				}
				else
				{
					pPlayer->SpeakWeaponFire( MP_CONCEPT_FIREMINIGUN );
				}
#endif

				m_flNextSecondaryAttack = m_flNextPrimaryAttack = m_flTimeWeaponIdle = gpGlobals->curtime + 0.1;
			}
			break;
		}
	case AC_STATE_FIRING:
		{
			if ( m_iWeaponMode == TF_WEAPON_SECONDARY_MODE )
			{
				SetWeaponState( AC_STATE_SPINNING );
			}

			if ( m_iWeaponState == AC_STATE_SPINNING )
			{
#ifdef GAME_DLL
				pPlayer->ClearWeaponFireScene();
				pPlayer->SpeakWeaponFire( MP_CONCEPT_WINDMINIGUN );
#endif
				m_flNextSecondaryAttack = m_flNextPrimaryAttack = m_flTimeWeaponIdle = gpGlobals->curtime + 0.1;

			}
			else if ( pPlayer->GetAmmoCount(m_iPrimaryAmmoType) <= 0 )
			{
				SetWeaponState( AC_STATE_DRYFIRE );
			}
			else
			{
				if ( m_flStartedFiringAt < 0 )
				{
					m_flStartedFiringAt = gpGlobals->curtime;
				}

#ifdef GAME_DLL
				if ( m_flNextFiringSpeech < gpGlobals->curtime )
				{
					m_flNextFiringSpeech = gpGlobals->curtime + 5.0;
					pPlayer->SpeakConceptIfAllowed( MP_CONCEPT_MINIGUN_FIREWEAPON );
				}
#endif

#ifdef CLIENT_DLL
				int nAmmo = 0;
				if ( prediction->IsFirstTimePredicted() && 
					 C_BasePlayer::GetLocalPlayer() == pPlayer )
				{
					nAmmo = pPlayer->GetAmmoCount( m_iPrimaryAmmoType );
				}
#endif

				// Only fire if we're actually shooting
				BaseClass::PrimaryAttack();		// fire and do timers
				
#ifdef CLIENT_DLL
				if ( prediction->IsFirstTimePredicted() && 
					 C_BasePlayer::GetLocalPlayer() == pPlayer &&
					 nAmmo != pPlayer->GetAmmoCount( m_iPrimaryAmmoType ) ) // did PrimaryAttack() fire a shot? (checking our ammo to find out)
				{
					m_nShotsFired++;
					if ( m_nShotsFired == 1000 ) // == and not >= so we don't keep awarding this every shot after it's achieved
					{
						g_AchievementMgrTF.OnAchievementEvent( ACHIEVEMENT_TF_HEAVY_FIRE_LOTS );
					}
					// NVNT the local player fired a shot. notify the haptics system.
					if ( haptics )
						haptics->ProcessHapticEvent(2,"Weapons","minigun_fire");
				}
#endif
				CalcIsAttackCritical();
				m_bCritShot = IsCurrentAttackACrit();
				pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );

#ifdef GAME_DLL

				int iAttackProjectiles = 0;
				CALL_ATTRIB_HOOK_INT( iAttackProjectiles, attack_projectiles );

#ifdef TF_RAID_MODE
				if ( TFGameRules()->IsBossBattleMode() )
				{
					iAttackProjectiles = 1;
				}
#endif // TF_RAID_MODE

				if ( iAttackProjectiles )
				{
					AttackEnemyProjectiles();
				}

#endif // GAME_DLL

				m_flTimeWeaponIdle = gpGlobals->curtime + 0.2;
			}
			break;
		}
	case AC_STATE_DRYFIRE:
		{
			m_flStartedFiringAt = -1.f;
			m_flStartedWindUpAt = -1.f;

			if ( pPlayer->GetAmmoCount(m_iPrimaryAmmoType) > 0 )
			{
				SetWeaponState( AC_STATE_FIRING );
			}
			else if ( m_iWeaponMode == TF_WEAPON_SECONDARY_MODE )
			{
				SetWeaponState( AC_STATE_SPINNING );
			}
			SendWeaponAnim( ACT_VM_SECONDARYATTACK );
			break;
		}
	case AC_STATE_SPINNING:
		{
			m_flStartedFiringAt = -1.f;

			if ( m_iWeaponMode == TF_WEAPON_PRIMARY_MODE )
			{
				if ( pPlayer->GetAmmoCount(m_iPrimaryAmmoType) > 0 )
				{
#ifdef GAME_DLL
					pPlayer->ClearWeaponFireScene();
					pPlayer->SpeakWeaponFire( MP_CONCEPT_FIREMINIGUN );
#endif
					SetWeaponState( AC_STATE_FIRING );
				}
				else
				{
					SetWeaponState( AC_STATE_DRYFIRE );
				}
			}

			SendWeaponAnim( ACT_VM_SECONDARYATTACK );
			break;
		}
	}

	if ( pPlayer->GetAmmoCount( m_iPrimaryAmmoType ) > 0 )
	{
		if ( m_iWeaponState > AC_STATE_STARTFIRING )
		{
			int nRingOfFireWhileAiming = 0;
			CALL_ATTRIB_HOOK_INT( nRingOfFireWhileAiming, ring_of_fire_while_aiming );
			if ( nRingOfFireWhileAiming != 0 )
			{
				RingOfFireAttack( nRingOfFireWhileAiming );
			}
		}

		if ( m_iWeaponState == AC_STATE_SPINNING || m_iWeaponState == AC_STATE_FIRING )
		{
			int nUsesAmmoWhileAiming = 0;
			CALL_ATTRIB_HOOK_INT( nUsesAmmoWhileAiming, uses_ammo_while_aiming );
			if ( nUsesAmmoWhileAiming > 0 )
			{
				m_flAccumulatedAmmoDrain += nUsesAmmoWhileAiming * ( gpGlobals->curtime - m_flLastAmmoDrainTime );
				m_flLastAmmoDrainTime = gpGlobals->curtime;

				if ( m_flAccumulatedAmmoDrain > 1.0f )
				{
					int nAmmoRemoved = m_flAccumulatedAmmoDrain;
					pPlayer->RemoveAmmo( nAmmoRemoved, m_iPrimaryAmmoType );

					m_flAccumulatedAmmoDrain -= nAmmoRemoved;
				}
			}
		}
	}
}

void CTFMinigun::SetWeaponState( MinigunState_t nState )
{
	if ( m_iWeaponState != nState )
	{
		if ( m_iWeaponState == AC_STATE_IDLE || m_iWeaponState == AC_STATE_STARTFIRING || m_iWeaponState == AC_STATE_DRYFIRE )
		{
			// Transitioning from non firing or non fully spinning states resets when our drain start point and when the ring of fire can start
			m_flLastAmmoDrainTime = gpGlobals->curtime;
			m_flNextRingOfFireAttackTime = gpGlobals->curtime + 0.5f;
		}
		
		m_iWeaponState = nState;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Fall through to Primary Attack
//-----------------------------------------------------------------------------
void CTFMinigun::SecondaryAttack( void )
{
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( !pPlayer )
		return;

	SharedAttack();
}

void CTFMinigun::RingOfFireAttack( int nDamage )
{
	if ( m_flNextRingOfFireAttackTime == 0.0f || m_flNextRingOfFireAttackTime > gpGlobals->curtime )
		return;

	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( !pPlayer )
		return;

#ifdef GAME_DLL

	Vector vOrigin = pPlayer->GetAbsOrigin();
	const float flFireRadius = 135.0f;
	const float flFireRadiusSqr = flFireRadius * flFireRadius;

	CBaseEntity *pEntity = NULL;
	for ( CEntitySphereQuery sphere( vOrigin, flFireRadius ); (pEntity = sphere.GetCurrentEntity()) != NULL; sphere.NextEntity() )
	{
		// Skip players on the same team or who are invuln
		CTFPlayer *pVictim = ToTFPlayer( pEntity );
		if ( !pVictim || InSameTeam( pVictim ) || pVictim->m_Shared.InCond( TF_COND_INVULNERABLE ) )
			continue;

		// Make sure their bounding box is near our ground plane
		Vector vMins = pVictim->GetPlayerMins();
		Vector vMaxs = pVictim->GetPlayerMaxs();
		if ( !( vOrigin.z > pVictim->GetAbsOrigin().z + vMins.z - 32.0f && vOrigin.z < pVictim->GetAbsOrigin().z + vMaxs.z ) )
		{
			continue;
		}

		// CEntitySphereQuery actually does a box test. So we need to make sure the distance is less than the radius first.
		Vector vecPos;
		pEntity->CollisionProp()->CalcNearestPoint( vOrigin, &vecPos );
		if ( ( vOrigin - vecPos ).LengthSqr() > flFireRadiusSqr )
			continue;

		// Finally LOS test
		trace_t	tr;
		Vector vecSrc = WorldSpaceCenter();
		Vector vecSpot = pEntity->WorldSpaceCenter();
		CTraceFilterSimple filter( this, COLLISION_GROUP_PROJECTILE );
		UTIL_TraceLine( vecSrc, vecSpot, MASK_SOLID_BRUSHONLY, &filter, &tr );

		// If we don't trace the whole way to the target, and we didn't hit the target entity, we're blocked
		if ( tr.fraction != 1.0 && tr.m_pEnt != pEntity )
			continue;

		pVictim->TakeDamage( CTakeDamageInfo( pPlayer, pPlayer, this, vec3_origin, vOrigin, nDamage, DMG_PLASMA, 0, &vOrigin ) );
	}

	DispatchParticleEffect( "heavy_ring_of_fire", pPlayer->GetAbsOrigin(), vec3_angle );

#else

	DispatchParticleEffect( "heavy_ring_of_fire_fp", pPlayer->GetAbsOrigin(), vec3_angle );

#endif // #ifdef GAME_DLL

	m_flNextRingOfFireAttackTime = gpGlobals->curtime + 0.5f;
}

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose:  Scans along a line for rockets and grenades to destroy
//-----------------------------------------------------------------------------
void CTFMinigun::AttackEnemyProjectiles( void )
{
	if ( gpGlobals->curtime < m_flAegisCheckTime )
		return;

	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( !pPlayer )
		return;

	// Parameters
	const int nSweepDist = 300;	// How far out
	const int nHitDist = ( pPlayer->IsMiniBoss() ) ? 56 : 38;	// How far from the center line (radial)
	float flRechargeTime = 0.1f;

	// Pos
	const Vector &vecGunPos = ( pPlayer->IsMiniBoss() ) ? pPlayer->Weapon_ShootPosition() : pPlayer->EyePosition();
	Vector vecForward;
	AngleVectors( GetAbsAngles(), &vecForward );
	Vector vecGunAimEnd = vecGunPos + vecForward * (float)nSweepDist;

	bool bDebug = false;
	if ( bDebug )
	{
		// NDebugOverlay::Sphere( vecGunPos + vecForward * nSweepDist, nSweepDist, 0, 255, 0, 40, 5 );
		NDebugOverlay::Box( vecGunPos, -Vector( 5, 5, 5 ), Vector( 5, 5, 5 ), 255, 0, 0, 40, 5 );
		NDebugOverlay::Box( vecGunAimEnd, -Vector( 5, 5, 5 ), Vector( 5, 5, 5 ), 255, 0, 0, 40, 5 );
		NDebugOverlay::Line( vecGunPos, vecGunAimEnd, 255, 255, 255, true, 5 );
	}

	// Iterate through each grenade/rocket in the sphere
	const int nMaxEnts = 32;
	CBaseEntity	*pObjects[ nMaxEnts ];
	int nCount = UTIL_EntitiesInSphere( pObjects, nMaxEnts, vecGunPos, nSweepDist, FL_GRENADE );
	for ( int i = 0; i < nCount; i++ )
	{
		if ( InSameTeam( pObjects[i] ) )
			continue;

		// Hit?
		const Vector &vecGrenadePos = pObjects[i]->GetAbsOrigin();
		float flDistToLine = CalcDistanceToLineSegment( vecGrenadePos, vecGunPos, vecGunAimEnd );
		if ( flDistToLine <= nHitDist )
		{
			if ( pPlayer->FVisible( pObjects[i], MASK_SOLID ) == false )
				continue;

			if ( ( pObjects[i]->GetFlags() & FL_ONGROUND ) )
				continue;
				
			if ( !pObjects[i]->IsDeflectable() )
				continue;

			CBaseProjectile *pProjectile = dynamic_cast< CBaseProjectile* >( pObjects[i] );
			if ( pProjectile && pProjectile->IsDestroyable() )
			{
				pProjectile->IncrementDestroyableHitCount();

				if ( bDebug )
				{
					NDebugOverlay::Box( vecGrenadePos, -Vector( 5, 5, 5 ), Vector( 5, 5, 5 ), 255, 0, 255, 40, 5 );
				}

				// Did we destroy it?
				int iAttackProjectiles = 0;
				CALL_ATTRIB_HOOK_INT( iAttackProjectiles, attack_projectiles );
				int nHitsRequired = m_bCritShot ? 1 : (int)RemapValClamped( iAttackProjectiles, 1, 2, 2, 1 );
				if ( pProjectile->GetDestroyableHitCount() >= nHitsRequired )
				{
					pProjectile->Destroy( false, true );

					EmitSound( "Halloween.HeadlessBossAxeHitWorld" );

					CTF_GameStats.Event_PlayerAwardBonusPoints( pPlayer, NULL, 2 );

					// Weaker version has a longer cooldown
					if ( iAttackProjectiles < 2 )
					{
						flRechargeTime = 0.3f;
					}
				}
				else
				{
					// Nicked it
					pObjects[i]->EmitSound( "FX_RicochetSound.Ricochet" );
				}
			}
		}
	}

	m_flAegisCheckTime = gpGlobals->curtime + flRechargeTime;
}

//-----------------------------------------------------------------------------
// Purpose:  Reduces damage and adds extra knockback.
//-----------------------------------------------------------------------------
void CTFMinigun::ActivatePushBackAttackMode( void )
{
	CTFPlayer *pOwner = ToTFPlayer( GetPlayerOwner() );
	if ( !pOwner )
		return;

	int iRage = 0;
	CALL_ATTRIB_HOOK_INT_ON_OTHER( pOwner, iRage, generate_rage_on_dmg );
	if ( !iRage )
		return;

	if ( pOwner->m_Shared.IsRageDraining() )
		return;

	if ( pOwner->m_Shared.GetRageMeter() < 100.f )
	{
		pOwner->EmitSound( "Player.DenyWeaponSelection" );
		return;
	}

	pOwner->m_Shared.StartRageDrain();
	EmitSound( "Heavy.Battlecry03" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFMinigun::GetInitialAfterburnDuration() const
{
	int nRingOfFireWhileAiming = 0;
	CALL_ATTRIB_HOOK_INT( nRingOfFireWhileAiming, ring_of_fire_while_aiming );
	if ( nRingOfFireWhileAiming != 0 )
	{
		return 8.f;
	}

	return BaseClass::GetInitialAfterburnDuration();
}
#endif // GAME_DLL

//-----------------------------------------------------------------------------
// Purpose: UI Progress (same as GetProgress() without the division by 100.0f)
//-----------------------------------------------------------------------------
bool CTFMinigun::IsRageFull( void )
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return false;

	return ( pPlayer->m_Shared.GetRageMeter() >= 100.0f );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFMinigun::EffectMeterShouldFlash( void )
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return false;

	if ( pPlayer && ( IsRageFull() || pPlayer->m_Shared.IsRageDraining() ) )
		return true;
	else
		return false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFMinigun::CanInspect() const
{
	return BaseClass::CanInspect() && CanHolster();
}

//-----------------------------------------------------------------------------
// Purpose: UI Progress
//-----------------------------------------------------------------------------
float CTFMinigun::GetProgress( void )
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return 0.f;

	return pPlayer->m_Shared.GetRageMeter() / 100.0f;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFMinigun::WindUp( void )
{
	// Get the player owning the weapon.
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( !pPlayer )
		return;

	// Play wind-up animation and sound (SPECIAL1).
	SendWeaponAnim( ACT_MP_ATTACK_STAND_PREFIRE );

	// Set the appropriate firing state.
	SetWeaponState( AC_STATE_STARTFIRING );
	pPlayer->m_Shared.AddCond( TF_COND_AIMING );

#ifndef CLIENT_DLL
	pPlayer->StopRandomExpressions();
#endif

#ifdef CLIENT_DLL 
	WeaponSoundUpdate();
#endif

	// Update player's speed
	pPlayer->TeamFortress_SetSpeed();

	if ( m_flStartedWindUpAt == -1.f )
	{
		m_flStartedWindUpAt = gpGlobals->curtime;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFMinigun::CanHolster( void ) const
{
	bool bCanHolster = CanHolsterWhileSpinning();

	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if( pPlayer )
	{
		// PASSTIME need to be able to immediately holster when you catch the ball
		if ( pPlayer->m_Shared.HasPasstimeBall() )
			return true;

		// TF_COND_MELEE_ONLY need to be able to immediately holster and switch to melee weapon
		if ( pPlayer->m_Shared.InCond( TF_COND_MELEE_ONLY ) )
			return true;
	}

	if ( bCanHolster )
	{
		if ( m_iWeaponState == AC_STATE_STARTFIRING || m_iWeaponState == AC_STATE_FIRING )
			return false;
	}
	else
	{
		if ( m_iWeaponState > AC_STATE_IDLE )
			return false;

		if ( GetActivity() == ACT_MP_ATTACK_STAND_POSTFIRE || GetActivity() == ACT_PRIMARY_ATTACK_STAND_POSTFIRE )
		{
			if ( !IsViewModelSequenceFinished() )
				return false;
		}
	}

	return BaseClass::CanHolster();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFMinigun::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	if ( m_iWeaponState > AC_STATE_IDLE )
	{
		WindDown();
	}

	return BaseClass::Holster( pSwitchingTo );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFMinigun::Lower( void )
{
	if ( m_iWeaponState > AC_STATE_IDLE )
	{
		WindDown();
	}

	return BaseClass::Lower();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFMinigun::WindDown( void )
{
	// Get the player owning the weapon.
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( !pPlayer )
		return;

	SendWeaponAnim( ACT_MP_ATTACK_STAND_POSTFIRE );

#ifdef CLIENT_DLL
	if ( !HasSpinSounds() && m_iWeaponState == AC_STATE_FIRING )
	{
		PlayStopFiringSound();
	}
#endif

	// Set the appropriate firing state.
	SetWeaponState( AC_STATE_IDLE );
	pPlayer->m_Shared.RemoveCond( TF_COND_AIMING );
#ifdef CLIENT_DLL
	WeaponSoundUpdate();
#else
	pPlayer->ClearWeaponFireScene();
#endif

	// Time to weapon idle.
	m_flTimeWeaponIdle = gpGlobals->curtime + 2.0;

	// Update player's speed
	pPlayer->TeamFortress_SetSpeed();

#ifdef CLIENT_DLL
	m_flBarrelTargetVelocity = 0;

	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( pLocalPlayer && GetOwner() == pLocalPlayer )
	{
		IGameEvent *event = gameeventmanager->CreateEvent( "localplayer_winddown" );
		if ( event )
		{
			gameeventmanager->FireEventClientSide( event );
		}
	}
#endif

	m_flStartedWindUpAt = -1.f;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFMinigun::WeaponIdle()
{
	if ( gpGlobals->curtime < m_flTimeWeaponIdle )
		return;

	// Always wind down if we've hit here, because it only happens when the player has stopped firing/spinning
	if ( m_iWeaponState != AC_STATE_IDLE )
	{
		CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
		if ( pPlayer )
		{
			pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_POST );
		}

		WindDown();
		return;
	}

	BaseClass::WeaponIdle();

	m_flTimeWeaponIdle = gpGlobals->curtime + 12.5;// how long till we do this again.
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFMinigun::FireGameEvent( IGameEvent * event )
{
#ifdef CLIENT_DLL
	C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
	if ( pLocalPlayer && GetOwner() == pLocalPlayer )
	{
		if ( FStrEq( event->GetName(), "teamplay_round_active" ) ||
			 FStrEq( event->GetName(), "localplayer_respawn" ) )
		{
			m_nShotsFired = 0;
		}
	}

	BaseClass::FireGameEvent( event );
#endif
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFMinigun::SendWeaponAnim( int iActivity )
{
#ifdef CLIENT_DLL
	// Client procedurally animates the barrel bone
	if ( iActivity == ACT_MP_ATTACK_STAND_PRIMARYFIRE || iActivity == ACT_MP_ATTACK_STAND_PREFIRE )
	{
		m_flBarrelTargetVelocity = MAX_BARREL_SPIN_VELOCITY;
	}
	else if ( iActivity == ACT_MP_ATTACK_STAND_POSTFIRE )
	{
		m_flBarrelTargetVelocity = 0;
	}

#endif


	// When we start firing, play the startup firing anim first
	if ( iActivity == ACT_VM_PRIMARYATTACK )
	{
		// If we're already playing the fire anim, let it continue. It loops.
		if ( GetActivity() == ACT_VM_PRIMARYATTACK )
			return true;

		// Otherwise, play the start it
		return BaseClass::SendWeaponAnim( ACT_VM_PRIMARYATTACK );		
	}

	return BaseClass::SendWeaponAnim( iActivity );
}

//-----------------------------------------------------------------------------
// Purpose: This will force the minigun to turn off the firing sound and play the spinning sound
//-----------------------------------------------------------------------------
void CTFMinigun::HandleFireOnEmpty( void )
{
	if ( m_iWeaponState == AC_STATE_FIRING || m_iWeaponState == AC_STATE_SPINNING )
	{
		 SetWeaponState( AC_STATE_DRYFIRE );

		 SendWeaponAnim( ACT_VM_SECONDARYATTACK );

		 if ( m_iWeaponMode == TF_WEAPON_SECONDARY_MODE )
		 {
			SetWeaponState ( AC_STATE_SPINNING );
		 }
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFMinigun::GetProjectileDamage( void )
{
	float flDamage = BaseClass::GetProjectileDamage();

	// How long have we been spun up - sans the min period required to fire
	float flPreFireWindUp = GetWindUpDuration() - TF_MINIGUN_SPINUP_TIME;
	float flSpinTime = Max( flPreFireWindUp, GetFiringDuration() );
	// DevMsg( "PreFireTime: %.2f\n", flPreFireWindUp );

	if ( flSpinTime < TF_MINIGUN_PENALTY_PERIOD )
	{
		float flMod = 1.f;
		flMod = RemapValClamped( flSpinTime, 0.2f, TF_MINIGUN_PENALTY_PERIOD, 0.5f, 1.f );
		flDamage *= flMod;
		//DevMsg( "DmgMod: %.2f\n", flMod );
	}
	
	return flDamage;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFMinigun::GetWeaponSpread( void )
{
	float flSpread = BaseClass::GetWeaponSpread();

	// How long have we been spun up - sans the min period required to fire
	float flPreFireWindUp = GetWindUpDuration() - TF_MINIGUN_SPINUP_TIME;
	float flSpinTime = Max( flPreFireWindUp, GetFiringDuration() );
	//DevMsg( "PreFireTime: %.2f\n", flPreFireWindUp );

	if ( flSpinTime < TF_MINIGUN_PENALTY_PERIOD )
	{
		const float flMaxSpread = 1.5f;
		float flMod = RemapValClamped( flSpinTime, 0.f, TF_MINIGUN_PENALTY_PERIOD, flMaxSpread, 1.f );
		//DevMsg( "SpreadMod: %.2f\n", flMod );

		flSpread *= flMod;
	}
	
	return flSpread;
}


#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CStudioHdr *CTFMinigun::OnNewModel( void )
{
	CStudioHdr *hdr = BaseClass::OnNewModel();

	m_iBarrelBone = LookupBone( "barrel" );

	// skip resetting this while recording in the tool
	// we change the weapon to the worldmodel and back to the viewmodel when recording
	// which causes the minigun to not spin while recording
	if ( !IsToolRecording() )
	{
		m_flBarrelAngle = 0;

		m_flBarrelCurrentVelocity = 0;
		m_flBarrelTargetVelocity = 0;
	}

	return hdr;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMinigun::StandardBlendingRules( CStudioHdr *hdr, Vector pos[], Quaternion q[], float currentTime, int boneMask )
{
	BaseClass::StandardBlendingRules( hdr, pos, q, currentTime, boneMask );

	if (m_iBarrelBone != -1)
	{
		UpdateBarrelMovement();

		// Weapon happens to be aligned to (0,0,0)
		// If that changes, use this code block instead to
		// modify the angles

		/*
		RadianEuler a;
		QuaternionAngles( q[iBarrelBone], a );

		a.x = m_flBarrelAngle;

		AngleQuaternion( a, q[iBarrelBone] );
		*/

		AngleQuaternion( RadianEuler( 0, 0, m_flBarrelAngle ), q[m_iBarrelBone] );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Updates the velocity and position of the rotating barrel
//-----------------------------------------------------------------------------
void CTFMinigun::UpdateBarrelMovement()
{
	if ( m_flBarrelCurrentVelocity != m_flBarrelTargetVelocity )
	{
		float flBarrelAcceleration = CanHolsterWhileSpinning() ? 0.5f : 0.1f;

		// update barrel velocity to bring it up to speed or to rest
		m_flBarrelCurrentVelocity = Approach( m_flBarrelTargetVelocity, m_flBarrelCurrentVelocity, flBarrelAcceleration );

		if ( 0 == m_flBarrelCurrentVelocity )
		{	
			// if we've stopped rotating, turn off the wind-down sound
			WeaponSoundUpdate();
		}
	}

	// update the barrel rotation based on current velocity
	m_flBarrelAngle += m_flBarrelCurrentVelocity * gpGlobals->frametime;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMinigun::OnDataChanged( DataUpdateType_t updateType )
{
	// Brass ejection and muzzle flash.
	HandleBrassEffect();
	HandleMuzzleEffect();

	BaseClass::OnDataChanged( updateType );

	WeaponSoundUpdate();
	
	// Turn off the firing sound here for the Tomislav
	if( m_iPrevMinigunState == AC_STATE_FIRING && 
		( m_iWeaponState == AC_STATE_SPINNING || m_iWeaponState == AC_STATE_IDLE ) )
	{
		if ( !HasSpinSounds() )
		{
			PlayStopFiringSound();
		}
	}

	m_iPrevMinigunState = m_iWeaponState;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMinigun::UpdateOnRemove( void )
{
	if ( m_pSoundCur )
	{
		CSoundEnvelopeController::GetController().SoundDestroy( m_pSoundCur );
		m_pSoundCur = NULL;
	}

	// Force the particle system off.
	StopMuzzleEffect();
	StopBrassEffect();

	BaseClass::UpdateOnRemove();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMinigun::SetDormant( bool bDormant )
{
	// If I'm going from active to dormant and I'm carried by another player, stop our firing sound.
	if ( !IsCarriedByLocalPlayer() )
	{
		// Am I firing? Stop the firing sound.
		if ( !IsDormant() && bDormant && m_iWeaponState >= AC_STATE_FIRING )
		{
			WeaponSoundUpdate();
		}

		// If firing and going dormant - stop the brass effect.
		if ( !IsDormant() && bDormant && m_iWeaponState != AC_STATE_IDLE )
		{
			StopMuzzleEffect();
			StopBrassEffect();
		}
	}

	// Deliberately skip base combat weapon
	C_BaseEntity::SetDormant( bDormant );
}


//-----------------------------------------------------------------------------
// Purpose: 
// won't be called for w_ version of the model, so this isn't getting updated twice
//-----------------------------------------------------------------------------
void CTFMinigun::ItemPreFrame( void )
{
	UpdateBarrelMovement();
	BaseClass::ItemPreFrame();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMinigun::StartBrassEffect()
{
	StopBrassEffect();

	m_hEjectBrassWeapon = GetWeaponForEffect();
	if ( !m_hEjectBrassWeapon )
		return;

	if ( UsingViewModel() && !g_pClientMode->ShouldDrawViewModel() )
	{
		// Prevent effects when the ViewModel is hidden with r_drawviewmodel=0
		return;
	}

	// Try and setup the attachment point if it doesn't already exist.
	// This caching will mess up if we go third person from first - we only do this in taunts and don't fire so we should
	// be okay for now.
	if ( m_iEjectBrassAttachment == -1 )
	{
		m_iEjectBrassAttachment = m_hEjectBrassWeapon->LookupAttachment( "eject_brass" );
	}

	// Start the brass ejection, if a system hasn't already been started.
	if ( m_iEjectBrassAttachment > 0 && m_pEjectBrassEffect == NULL )
	{
		m_pEjectBrassEffect = m_hEjectBrassWeapon->ParticleProp()->Create( "eject_minigunbrass", PATTACH_POINT_FOLLOW, m_iEjectBrassAttachment );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMinigun::StartMuzzleEffect()
{
	StopMuzzleEffect();

	m_hMuzzleEffectWeapon = GetWeaponForEffect();
	if ( !m_hMuzzleEffectWeapon )
		return;

	if ( UsingViewModel() && !g_pClientMode->ShouldDrawViewModel() )
	{
		// Prevent effects when the ViewModel is hidden with r_drawviewmodel=0
		return;
	}

	// Try and setup the attachment point if it doesn't already exist.
	// This caching will mess up if we go third person from first - we only do this in taunts and don't fire so we should
	// be okay for now.
	if ( m_iMuzzleAttachment <= 0 )
	{
		m_iMuzzleAttachment = m_hMuzzleEffectWeapon->LookupAttachment( "muzzle" );
	}

	// Start the muzzle flash, if a system hasn't already been started.
	if ( m_iMuzzleAttachment > 0 && m_pMuzzleEffect == NULL )
	{
		m_pMuzzleEffect = m_hMuzzleEffectWeapon->ParticleProp()->Create( "muzzle_minigun_constant", PATTACH_POINT_FOLLOW, m_iMuzzleAttachment );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMinigun::StopBrassEffect()
{
	if ( !m_hEjectBrassWeapon )
		return;

	// Stop the brass ejection.
	if ( m_pEjectBrassEffect )
	{
		m_hEjectBrassWeapon->ParticleProp()->StopEmission( m_pEjectBrassEffect );
		m_hEjectBrassWeapon = NULL;
		m_pEjectBrassEffect = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMinigun::StopMuzzleEffect()
{
	if ( !m_hMuzzleEffectWeapon )
		return;

	// Stop the muzzle flash.
	if ( m_pMuzzleEffect )
	{
		m_hMuzzleEffectWeapon->ParticleProp()->StopEmission( m_pMuzzleEffect );
		m_hMuzzleEffectWeapon = NULL;
		m_pMuzzleEffect		  = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMinigun::HandleBrassEffect()
{
	if ( m_iWeaponState == AC_STATE_FIRING && m_pEjectBrassEffect == NULL )
	{
		StartBrassEffect();
	}
	else if ( m_iWeaponState != AC_STATE_FIRING && m_pEjectBrassEffect )
	{
		StopBrassEffect();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMinigun::HandleMuzzleEffect()
{
	if ( m_iWeaponState == AC_STATE_FIRING && m_pMuzzleEffect == NULL )
	{
		StartMuzzleEffect();
	}
	else if ( m_iWeaponState != AC_STATE_FIRING && m_pMuzzleEffect )
	{
		StopMuzzleEffect();
	}
}

//-----------------------------------------------------------------------------
// Purpose: View model barrel rotation angle. Calculated here, implemented in 
// tf_viewmodel.cpp
//-----------------------------------------------------------------------------
float CTFMinigun::GetBarrelRotation( void )
{
	return m_flBarrelAngle;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMinigun::ViewModelAttachmentBlending( CStudioHdr *hdr, Vector pos[], Quaternion q[], float currentTime, int boneMask )
{
	int iBarrelBone = Studio_BoneIndexByName( hdr, "barrel" );

	// Assert( iBarrelBone != -1 );

	if ( iBarrelBone != -1 )
	{
		if ( hdr->boneFlags( iBarrelBone ) & boneMask )
		{
			RadianEuler a;
			QuaternionAngles( q[iBarrelBone], a );

			a.z = GetBarrelRotation();

			AngleQuaternion( a, q[iBarrelBone] );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFMinigun::CreateMove( float flInputSampleTime, CUserCmd *pCmd, const QAngle &vecOldViewAngles )
{
	// Prevent jumping while firing
	if ( m_iWeaponState != AC_STATE_IDLE )
	{
		pCmd->buttons &= ~IN_JUMP;
	}

	BaseClass::CreateMove( flInputSampleTime, pCmd, vecOldViewAngles );
}

//-----------------------------------------------------------------------------
// Purpose: Ensures the correct sound (including silence) is playing for 
//			current weapon state.
//-----------------------------------------------------------------------------
void CTFMinigun::WeaponSoundUpdate()
{
	// determine the desired sound for our current state
	int iSound = -1;
	switch ( m_iWeaponState )
	{
	case AC_STATE_IDLE:
		if ( !HasSpinSounds() && m_iMinigunSoundCur == SPECIAL2 )
		{
			// Don't turn off SPECIAL2 (stop firing sound) for non spinning miniguns.
			// We don't have a wind-down sound.
			return;
		}
		else if ( HasSpinSounds() && m_flBarrelCurrentVelocity > 0 )
		{
			iSound = SPECIAL2;	// wind down sound

			if ( m_flBarrelTargetVelocity > 0 )
			{
				m_flBarrelTargetVelocity = 0;
			}
		}
		else
		{
			iSound = -1;
		}
		break;
	case AC_STATE_STARTFIRING:
			iSound = SPECIAL1;	// wind up sound
		break;
	case AC_STATE_FIRING:
		{
			if ( m_bCritShot == true ) 
			{
				iSound = BURST;	// Crit sound
			}
			else
			{
				iSound = WPN_DOUBLE; // firing sound
			}
		}
		break;
	case AC_STATE_SPINNING:
		if ( HasSpinSounds() )
			iSound = SPECIAL3;	// spinning sound
		else
			return;
		break;
	case AC_STATE_DRYFIRE:
		iSound = EMPTY;		// out of ammo, still trying to fire
		break;
	default:
		Assert( false );
		break;
	}

	// Get the pitch we should play at
	float flPitch = 1.0f;

	float flSpeed = ApplyFireDelay( 1.0f );
	if ( flSpeed != 1.0f )
	{
		flPitch = RemapValClamped( flSpeed, 1.5f, 0.5f, 80.f, 120.f );
	}

	if ( m_bRageDraining )
	{
		flPitch /= 1.65;
	}

	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();

	// if we're already playing the desired sound, nothing to do
	if ( m_iMinigunSoundCur == iSound && m_bPrevRageDraining == m_bRageDraining )
	{
		// If the pitch is different we need to modify it
		if ( m_flMinigunSoundCurrentPitch != flPitch )
		{
			m_flMinigunSoundCurrentPitch = flPitch;

			if ( m_pSoundCur )
			{
				controller.SoundChangePitch( m_pSoundCur, m_flMinigunSoundCurrentPitch, 0.3f );
			}
		}
		return;
	}

	// if we're playing some other sound, stop it
	if ( m_pSoundCur )
	{
		// Stop the previous sound immediately
		CSoundEnvelopeController::GetController().SoundDestroy( m_pSoundCur );
		m_pSoundCur = NULL;
	}
	m_iMinigunSoundCur = iSound;
	// if there's no sound to play for current state, we're done
	if ( -1 == iSound )
		return;

	m_flMinigunSoundCurrentPitch = flPitch;

	// play the appropriate sound
	const char *shootsound = GetShootSound( iSound );
	CLocalPlayerFilter filter;
	m_pSoundCur = controller.SoundCreate( filter, entindex(), shootsound );
	controller.Play( m_pSoundCur, 1.0, 100 );
	controller.SoundChangeVolume( m_pSoundCur, 1.0, 0.1 );

	if ( m_flMinigunSoundCurrentPitch != 1.0f )
	{
		controller.SoundChangePitch( m_pSoundCur, m_flMinigunSoundCurrentPitch, 0.0 );
	}

	m_bPrevRageDraining = m_bRageDraining;
}

void CTFMinigun::PlayStopFiringSound()
{
	if ( m_pSoundCur )
	{
		CSoundEnvelopeController::GetController().SoundDestroy( m_pSoundCur );
		m_pSoundCur = NULL;
	}

	m_iMinigunSoundCur = SPECIAL2;

	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
	const char *shootsound = GetShootSound( SPECIAL2 );
	CLocalPlayerFilter filter;
	m_pSoundCur = controller.SoundCreate( filter, entindex(), shootsound );
	controller.Play( m_pSoundCur, 1.0, 100 );
	controller.SoundChangeVolume( m_pSoundCur, 1.0, 0.1 );
}

#endif
