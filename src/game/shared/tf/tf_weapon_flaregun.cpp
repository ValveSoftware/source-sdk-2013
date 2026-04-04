//========= Copyright Valve Corporation, All rights reserved. ============//
//
//
//=============================================================================
#include "cbase.h"
#include "tf_weapon_flaregun.h"
#include "tf_fx_shared.h"
#include "in_buttons.h"

// Client specific.
#ifdef CLIENT_DLL
#include "c_tf_player.h"
#include "soundenvelope.h"
// Server specific.
#else
#include "tf_gamestats.h"
#include "tf_player.h"
#endif

//=============================================================================
//
// Weapon Flare Gun tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFFlareGun, DT_WeaponFlareGun )

BEGIN_NETWORK_TABLE( CTFFlareGun, DT_WeaponFlareGun )
#ifdef CLIENT_DLL
	RecvPropFloat( RECVINFO( m_flChargeBeginTime ) ),
#else
	SendPropFloat( SENDINFO( m_flChargeBeginTime ) ),
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFFlareGun )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_flaregun, CTFFlareGun );
PRECACHE_WEAPON_REGISTER( tf_weapon_flaregun );

// Server specific.
#ifndef CLIENT_DLL
BEGIN_DATADESC( CTFFlareGun )
END_DATADESC()
#endif


//============================
IMPLEMENT_NETWORKCLASS_ALIASED( TFFlareGun_Revenge, DT_WeaponFlareGun_Revenge )

BEGIN_NETWORK_TABLE( CTFFlareGun_Revenge, DT_WeaponFlareGun_Revenge )
#ifdef CLIENT_DLL
RecvPropFloat( RECVINFO( m_fLastExtinguishTime ) ),
#else
SendPropFloat( SENDINFO( m_fLastExtinguishTime ) ),
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFFlareGun_Revenge )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_flaregun_revenge, CTFFlareGun_Revenge );
PRECACHE_WEAPON_REGISTER( tf_weapon_flaregun_revenge );

#ifdef GAME_DLL
const float tf_flaregun_afterburn_rate = 7.5f;
#endif // GAME_DLL


//=============================================================================
//
// Weapon Flare Gun functions.
//

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFFlareGun::CTFFlareGun()
{
	m_bEffectsThinking = false;
	m_flLastDenySoundTime = 0.0f;

#ifdef CLIENT_DLL
	m_bReadyToFire = false;
#endif

	StopCharge();
}

CTFFlareGun::~CTFFlareGun()
{
	DestroySounds();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFFlareGun::Precache()
{
	BaseClass::Precache();

	PrecacheParticleSystem( "stickybombtrail_blue" );
	PrecacheParticleSystem( "stickybombtrail_red" );
	PrecacheParticleSystem( "critical_grenade_blue" );
	PrecacheParticleSystem( "critical_grenade_red" );
}

void CTFFlareGun::DestroySounds( void )
{
	StopCharge();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFFlareGun::PrimaryAttack( void )
{
	// Get the player owning the weapon.
	CTFPlayer *pOwner = ToTFPlayer( GetPlayerOwner() );
	if ( !pOwner )
		return;

	if ( m_flChargeBeginTime > 0.0f )
		return;

	// Don't attack if we're underwater
	if ( pOwner->GetWaterLevel() != WL_Eyes )
	{
		BaseClass::PrimaryAttack();
	}
	else
	{
		if ( gpGlobals->curtime > m_flLastDenySoundTime )
		{
			WeaponSound( SPECIAL2 );
			m_flLastDenySoundTime = gpGlobals->curtime + 1.0f;
		}
	}

#ifdef CLIENT_DLL
	m_bReadyToFire = false;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Detonate flare
//-----------------------------------------------------------------------------
void CTFFlareGun::SecondaryAttack( void )
{
	if ( GetFlareGunType() != FLAREGUN_DETONATE )
		return;

	if ( !CanAttack() )
		return;

	CTFPlayer *pPlayer = ToTFPlayer( GetOwner() );
	if ( !pPlayer )
		return;

#ifdef GAME_DLL
	if ( m_iFlareCount )
	{
		int iCount = m_Flares.Count();
		for ( int i = 0; i < iCount; i++ )
		{
			CTFProjectile_Flare *pTemp = m_Flares[i];
			if ( pTemp )
			{
				pTemp->Detonate();
			}
		}
	}
#endif // GAME_DLL
}

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFFlareGun::AddFlare( CTFProjectile_Flare *pFlare )
{
	FlareHandle hHandle;
	hHandle = pFlare;
	m_Flares.AddToTail( hHandle );

	m_iFlareCount = m_Flares.Count();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFFlareGun::DeathNotice( CBaseEntity *pVictim )
{
	Assert( dynamic_cast<CTFProjectile_Flare*>( pVictim ) );

	FlareHandle hHandle;
	hHandle = (CTFProjectile_Flare*)pVictim;
	m_Flares.FindAndRemove( hHandle );

	m_iFlareCount = m_Flares.Count();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFFlareGun::GetAfterburnRateOnHit() const
{
	return tf_flaregun_afterburn_rate;
}
#endif // GAME_DLL

bool CTFFlareGun::Holster( CBaseCombatWeapon *pSwitchingTo )
{
#ifdef CLIENT_DLL
	m_bEffectsThinking = false;
	StopCharge();

	m_bReadyToFire = false;
#endif

	return BaseClass::Holster( pSwitchingTo );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFFlareGun::Deploy( void )
{
#ifdef CLIENT_DLL
	m_bEffectsThinking = true;
	SetContextThink( &CTFFlareGun::ClientEffectsThink, gpGlobals->curtime + 0.25f, "EFFECTS_THINK" );

	m_bReadyToFire = false;
#endif

	return BaseClass::Deploy();
}

void CTFFlareGun::WeaponReset( void )
{
	BaseClass::WeaponReset();

#if defined( CLIENT_DLL )
	StopCharge();
#endif
}

void CTFFlareGun::ItemPostFrame( void )
{
	BaseClass::ItemPostFrame();

	if ( m_flChargeBeginTime > 0.0f )
	{
		CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
		if ( !pPlayer )
			return;

		// If we're not holding down the attack button
		if ( !(pPlayer->m_nButtons & IN_ATTACK2) )
		{
			StopCharge();
		}
		else
		{
			ChargePostFrame();
		}
	}

#ifdef CLIENT_DLL
	if ( !m_bEffectsThinking )
	{
		m_bEffectsThinking = true;
		SetContextThink( &CTFFlareGun::ClientEffectsThink, gpGlobals->curtime + 0.25f, "EFFECTS_THINK" );
	}
#endif
}

#ifdef CLIENT_DLL
void CTFFlareGun::DispatchMuzzleFlash( const char* effectName, C_BaseEntity* pAttachEnt )
{
	DispatchParticleEffect( effectName, PATTACH_POINT_FOLLOW, pAttachEnt, "muzzle", GetParticleColor( 1 ), GetParticleColor( 2 ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFlareGun::ClientEffectsThink( void )
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return;

	if ( !pPlayer->IsLocalPlayer() )
		return;

	if ( !pPlayer->GetViewModel() )
		return;

	if ( !m_bEffectsThinking )
		return;

	if ( !GetOwner() || GetOwner()->GetActiveWeapon() != this )
	{
		m_bEffectsThinking = false;
	}
	else
	{
		SetContextThink( &CTFFlareGun::ClientEffectsThink, gpGlobals->curtime + 0.25f, "EFFECTS_THINK" );
	}

	if ( GetFlareGunType() == FLAREGUN_GRORDBORT && m_flNextPrimaryAttack <= gpGlobals->curtime )
	{
		ParticleProp()->Init( this );
		CNewParticleEffect* pEffect = ParticleProp()->Create( "drg_bison_idle", PATTACH_POINT_FOLLOW, "muzzle" );
		if ( pEffect )
		{
			pEffect->SetControlPoint( CUSTOM_COLOR_CP1, GetParticleColor( 1 ) );
			pEffect->SetControlPoint( CUSTOM_COLOR_CP2, GetParticleColor( 2 ) );
		}

		ParticleProp()->Create( "drg_manmelter_idle", PATTACH_POINT_FOLLOW, "muzzle" );

		if ( !m_bReadyToFire )
		{
			m_bReadyToFire = true;

			EmitSound( "Weapon_SniperRailgun.NonScoped" );
		}
	}
}

void CTFFlareGun::StartChargeEffects()
{
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( pPlayer )
	{
		DispatchParticleEffect( GetChargeEffect(), PATTACH_POINT_FOLLOW, GetAppropriateWorldOrViewModel(), "muzzle", GetParticleColor( 1 ), GetParticleColor( 2 ) );
	}
}

void CTFFlareGun::StopChargeEffects()
{
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( pPlayer )
	{
		GetAppropriateWorldOrViewModel()->ParticleProp()->StopParticlesNamed( GetChargeEffect(), false );
	}
}

#endif

void CTFFlareGun::StartCharge( void )
{
	StartChargeStartTime();

#ifdef CLIENT_DLL
	if ( !m_pChargeLoop )
	{
		CLocalPlayerFilter filter;
		CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
		m_pChargeLoop = controller.SoundCreate( filter, entindex(), GetShootSound( WPN_DOUBLE ) );
		controller.Play( m_pChargeLoop, 1.0, 100 );
	}

	StartChargeEffects();
#endif
}

void CTFFlareGun::StopCharge( void )
{
	m_flChargeBeginTime = 0.0f;

#ifdef CLIENT_DLL
	if ( m_pChargeLoop )
	{
		CSoundEnvelopeController::GetController().SoundDestroy( m_pChargeLoop );
	}

	m_pChargeLoop = NULL;

	StopChargeEffects();
#endif
}


CTFFlareGun_Revenge::CTFFlareGun_Revenge()
{
	m_fLastExtinguishTime = 0.0f;

#ifdef CLIENT_DLL
	m_nOldRevengeCrits = 0;
#endif
}

void CTFFlareGun_Revenge::Precache()
{
	BaseClass::Precache();

	PrecacheParticleSystem( "drg_manmelter_vacuum" );
	PrecacheParticleSystem( "drg_manmelter_vacuum_flames" );
	PrecacheParticleSystem( "drg_manmelter_muzzleflash" );
}

int CTFFlareGun_Revenge::GetCustomDamageType() const
{
	CTFPlayer *pOwner = ToTFPlayer( GetPlayerOwner() );
	if ( pOwner )
	{
		int iRevengeCrits = pOwner->m_Shared.GetRevengeCrits();
		return iRevengeCrits > 0 ? TF_DMG_CUSTOM_SHOTGUN_REVENGE_CRIT : TF_DMG_CUSTOM_NONE;
	}
	return TF_DMG_CUSTOM_NONE;
}

bool CTFFlareGun_Revenge::Holster( CBaseCombatWeapon *pSwitchingTo )
{
#ifdef GAME_DLL
	CTFPlayer *pOwner = ToTFPlayer( GetPlayerOwner() );
	if ( pOwner && pOwner->m_Shared.GetRevengeCrits() )
	{
		pOwner->m_Shared.RemoveCond( TF_COND_CRITBOOSTED );
	}
#endif

	StopCharge();

	return BaseClass::Holster( pSwitchingTo );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFFlareGun_Revenge::Deploy( void )
{
#ifdef GAME_DLL
	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
	if ( pOwner && pOwner->m_Shared.GetRevengeCrits() )
	{
		pOwner->m_Shared.AddCond( TF_COND_CRITBOOSTED );
	}
#endif

	StopCharge();

	return BaseClass::Deploy();
}

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose: Reset revenge crits when the flaregun is changed
//-----------------------------------------------------------------------------
void CTFFlareGun_Revenge::Detach( void )
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( pPlayer )
	{
		pPlayer->m_Shared.SetRevengeCrits( 0 );
		pPlayer->m_Shared.RemoveCond( TF_COND_CRITBOOSTED );
	}

	BaseClass::Detach();
}
#endif

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int CTFFlareGun_Revenge::GetCount( void )
{
	CTFPlayer *pOwner = ToTFPlayer( GetPlayerOwner() );
	if ( pOwner )
	{
		return pOwner->m_Shared.GetRevengeCrits();
	}

	return 0;
}

void CTFFlareGun_Revenge::PrimaryAttack()
{
	if ( !CanAttack() )
		return;

	BaseClass::PrimaryAttack();

	// Lower the reveng crit count
	CTFPlayer *pOwner = ToTFPlayer( GetPlayerOwner() );
	if ( pOwner )
	{
		int iNewRevengeCrits = MAX( pOwner->m_Shared.GetRevengeCrits() - 1, 0 );
		pOwner->m_Shared.SetRevengeCrits( iNewRevengeCrits );
	}
}

void CTFFlareGun_Revenge::SecondaryAttack( void )
{
	if ( m_flNextSecondaryAttack > gpGlobals->curtime )
	{
		return;
	}

	CTFPlayer *pOwner = ToTFPlayer( GetPlayerOwner() );
	if ( !pOwner )
		return;

	if ( GetChargeBeginTime() == 0.0f )
	{
		StartCharge();

#ifdef GAME_DLL
		//SendWeaponAnim( ACT_VM_PULLBACK );
#endif
	}

	m_flNextSecondaryAttack = gpGlobals->curtime + 0.5f;
}

void CTFFlareGun_Revenge::ChargePostFrame( void )
{
	BaseClass::ChargePostFrame();

	if ( gpGlobals->curtime > m_fLastExtinguishTime + 0.5f )
	{
		CTFPlayer *pOwner = ToTFPlayer( GetPlayerOwner() );
		if ( pOwner )
		{
			// Extinguish friends
			Vector vecEye = pOwner->EyePosition();
			Vector vecForward, vecRight, vecUp;
			AngleVectors( pOwner->EyeAngles(), &vecForward, NULL, NULL );

			const Vector vHull = Vector( 16.0f, 16.0f, 16.0f );

			trace_t tr;
			UTIL_TraceHull( vecEye, vecEye + vecForward * 256.0f, -vHull, vHull, MASK_SOLID, pOwner, COLLISION_GROUP_NONE, &tr );

			CTFPlayer *pTarget = ToTFPlayer( tr.m_pEnt );
			if ( pTarget )
			{
#ifdef GAME_DLL
				// Get the player that ignited them before we extinguish
				CTFPlayer *pBurner = pTarget->m_Shared.GetOriginalBurnAttacker();
#endif

				if ( ExtinguishPlayerInternal( pTarget, pOwner ) )
				{
					m_fLastExtinguishTime = gpGlobals->curtime;

#ifdef GAME_DLL
					// Make sure the team isn't burning themselves to earn crits
					if ( pBurner && pBurner->GetTeamNumber() != pOwner->GetTeamNumber() )
					{
						// Grant revenge crits
						pOwner->m_Shared.SetRevengeCrits( pOwner->m_Shared.GetRevengeCrits() + 1 );

						// Return health to the Pyro.
						int iRestoreHealthOnExtinguish = 0;
						CALL_ATTRIB_HOOK_INT( iRestoreHealthOnExtinguish, extinguish_restores_health );
						if ( iRestoreHealthOnExtinguish > 0 && pOwner->TakeHealth( 20, DMG_GENERIC ) > 0 )
						{
							IGameEvent *healevent = gameeventmanager->CreateEvent( "player_healonhit" );
							if ( healevent )
							{
								healevent->SetInt( "amount", iRestoreHealthOnExtinguish );
								healevent->SetInt( "entindex", pOwner->entindex() );
								item_definition_index_t healingItemDef = INVALID_ITEM_DEF_INDEX;
								if ( GetAttributeContainer() && GetAttributeContainer()->GetItem() )
								{
									healingItemDef = GetAttributeContainer()->GetItem()->GetItemDefIndex();
								}
								healevent->SetInt( "weapon_def_index", healingItemDef );

								gameeventmanager->FireEvent( healevent );
							}
						}
					}
#endif
				}
			}
		}
	}
}

#ifdef GAME_DLL
extern void ExtinguishPlayer( CEconEntity *pExtinguisher, CTFPlayer *pOwner, CTFPlayer *pTarget, const char *pExtinguisherName );
#endif // GAME_DLL

bool CTFFlareGun_Revenge::ExtinguishPlayerInternal( CTFPlayer *pTarget, CTFPlayer *pOwner )
{
	if ( pTarget->GetTeamNumber() == pOwner->GetTeamNumber() )
	{
		if ( pTarget->m_Shared.InCond( TF_COND_BURNING ) )
		{
#ifdef GAME_DLL
			ExtinguishPlayer( this, pOwner, pTarget, GetName() );
#endif // GAME_DLL

			return true;
		}
	}

	return false;
}

#ifdef CLIENT_DLL
void CTFFlareGun_Revenge::OnDataChanged( DataUpdateType_t type )
{
	BaseClass::OnDataChanged( type );

	CTFPlayer *pOwner = ToTFPlayer( GetPlayerOwner() );
	if ( pOwner )
	{
		if ( m_nOldRevengeCrits < pOwner->m_Shared.GetRevengeCrits() )
		{
			DoAbsorbEffect();
		}
		
		m_nOldRevengeCrits = pOwner->m_Shared.GetRevengeCrits();
	}
}

void CTFFlareGun_Revenge::DoAbsorbEffect( void )
{
	WeaponSound( SPECIAL1 );

	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( pPlayer )
	{
		DispatchParticleEffect( "drg_manmelter_vacuum_flames", PATTACH_POINT_FOLLOW, GetAppropriateWorldOrViewModel(), "muzzle", GetParticleColor( 1 ), GetParticleColor( 2 ) );
	}
}
#endif
