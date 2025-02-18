//========= Copyright Valve Corporation, All rights reserved. ============//
//
//
//=============================================================================
#include "cbase.h"
#include "tf_weapon_raygun.h"
#include "tf_fx_shared.h"
#include "in_buttons.h"

// Client specific.
#ifdef CLIENT_DLL
#include "c_tf_player.h"
#include "particle_property.h"
#else
#include "tf_player.h"
#include "ndebugoverlay.h"
#include "particle_parse.h"
#include "tf_fx.h"
#endif


//============================
IMPLEMENT_NETWORKCLASS_ALIASED( TFDRGPomson, DT_WeaponDRGPomson )

BEGIN_NETWORK_TABLE( CTFDRGPomson, DT_WeaponDRGPomson )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFDRGPomson )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_drg_pomson, CTFDRGPomson );
PRECACHE_WEAPON_REGISTER( tf_weapon_drg_pomson );



//============================

IMPLEMENT_NETWORKCLASS_ALIASED( TFRaygun, DT_WeaponRaygun )

BEGIN_NETWORK_TABLE( CTFRaygun, DT_WeaponRaygun )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFRaygun )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_raygun, CTFRaygun );
PRECACHE_WEAPON_REGISTER( tf_weapon_raygun );

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFRaygun::CTFRaygun()
{
	m_flIrradiateTime = 0.f;
	m_bEffectsThinking = false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFRaygun::Precache()
{
	PrecacheParticleSystem( "drg_bison_impact" );
	PrecacheParticleSystem( "drg_bison_idle" );
	PrecacheParticleSystem( "drg_bison_muzzleflash" );

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CTFRaygun::GetMuzzleFlashParticleEffect( void )
{
	return "drg_bison_muzzleflash";
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFRaygun::PrimaryAttack( void )
{
	if ( !Energy_HasEnergy() )
		return;

	BaseClass::PrimaryAttack();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFRaygun::ModifyProjectile( CBaseEntity* pProj )
{
#ifdef GAME_DLL
	/*
	CTFProjectile_EnergyRing* pEnergyBall = dynamic_cast<CTFProjectile_EnergyRing*>( pProj );
	if ( pEnergyBall == NULL )
		return;

	pEnergyBall->SetColor( 1, GetParticleColor( 1 ) );
	pEnergyBall->SetColor( 2, GetParticleColor( 2 ) );
	*/
#endif

	Energy_DrainEnergy();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFRaygun::GetProgress( void )
{
	return Energy_GetEnergy() / ENERGY_WEAPON_MAX_CHARGE;
}

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFRaygun::DispatchMuzzleFlash( const char* effectName, C_BaseEntity* pAttachEnt )
{
	DispatchParticleEffect( effectName, PATTACH_POINT_FOLLOW, pAttachEnt, "muzzle", GetParticleColor( 1 ), GetParticleColor( 2 ) );
}
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFRaygun::Holster( CBaseCombatWeapon *pSwitchingTo )
{
#ifdef CLIENT_DLL
	m_bEffectsThinking = false;
#endif

	return BaseClass::Holster( pSwitchingTo );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFRaygun::Deploy( void )
{
#ifdef CLIENT_DLL
	m_bEffectsThinking = true;
	SetContextThink( &CTFRaygun::ClientEffectsThink, gpGlobals->curtime + rand() % 5, "EFFECTS_THINK" );
#endif

	return BaseClass::Deploy();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFRaygun::ItemPostFrame( void )
{
	BaseClass::ItemPostFrame();

#ifdef CLIENT_DLL

	if ( !m_bEffectsThinking )
	{
		m_bEffectsThinking = true;
		SetContextThink( &CTFRaygun::ClientEffectsThink, gpGlobals->curtime + rand() % 5, "EFFECTS_THINK" );
	}
#endif
}

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFRaygun::ClientEffectsThink( void )
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

	SetContextThink( &CTFRaygun::ClientEffectsThink, gpGlobals->curtime + 2 + rand() % 5, "EFFECTS_THINK" );

	ParticleProp()->Init( this );
	CNewParticleEffect* pEffect = ParticleProp()->Create( "drg_bison_idle", PATTACH_POINT_FOLLOW, "muzzle" );
	if ( pEffect )
	{
		pEffect->SetControlPoint( CUSTOM_COLOR_CP1, GetParticleColor( 1 ) );
		pEffect->SetControlPoint( CUSTOM_COLOR_CP2, GetParticleColor( 2 ) );
	}
}
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFRaygun::GetProjectileSpeed( void )
{
	return 1200.f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFRaygun::GetProjectileGravity( void )
{
	return 0.f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFRaygun::IsViewModelFlipped( void )
{
	return !BaseClass::IsViewModelFlipped(); 
}
