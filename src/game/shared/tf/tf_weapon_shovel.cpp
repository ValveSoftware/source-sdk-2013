//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "tf_weapon_shovel.h"
#include "decals.h"

// Client specific.
#ifdef CLIENT_DLL
#include "c_tf_player.h"
// Server specific.
#else
#include "tf_player.h"
#endif

//=============================================================================
//
// Weapon Shovel tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFShovel, DT_TFWeaponShovel )

BEGIN_NETWORK_TABLE( CTFShovel, DT_TFWeaponShovel )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFShovel )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_shovel, CTFShovel );
PRECACHE_WEAPON_REGISTER( tf_weapon_shovel );

//=============================================================================
//
// Weapon Shovel functions.
//

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFShovel::CTFShovel()
{
	m_bHolstering = false;
	m_flLastHealthRatio = 1.f;
}

// -----------------------------------------------------------------------------
void CTFShovel::PrimaryAttack()
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( pPlayer && !(pPlayer->GetFlags() & FL_ONGROUND) )
	{
		int iFlappy = 0;
		CALL_ATTRIB_HOOK_INT( iFlappy, air_jump_on_attack );
		if ( iFlappy )
		{
#ifdef GAME_DLL
			EmitSound_t params;
			params.m_pSoundName = "General.banana_slip";
			params.m_flSoundTime = 0;
			params.m_pflSoundDuration = 0;
			//params.m_bWarnOnDirectWaveReference = true;
			CPASFilter filter( pPlayer->GetAbsOrigin() );
			params.m_flVolume = 0.1f;
			params.m_SoundLevel = SNDLVL_25dB;
			params.m_nPitch = 100.0f;
			params.m_nFlags |= ( SND_CHANGE_PITCH | SND_CHANGE_VOL );
			pPlayer->StopSound( "General.banana_slip" );
			pPlayer->EmitSound( filter, pPlayer->entindex(), params );
#endif
			Vector vForce = Vector(0,0,0);
			if ( pPlayer->GetAbsVelocity().z > 0 )
			{
				vForce.z = 275.0f;
			}
			else if ( pPlayer->m_Shared.InCond( TF_COND_BLASTJUMPING ) )
			{
				vForce.z = 500.0f;
			}
			else
			{
				vForce.z = 350.0f;
			}
			pPlayer->ApplyAbsVelocityImpulse( vForce );
		}	
	}
	

	BaseClass::PrimaryAttack();
}
// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
void CTFShovel::ItemPreFrame( void )
{
	return BaseClass::ItemPreFrame();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFShovel::GetMeleeDamage( CBaseEntity *pTarget, int* piDamageType, int* piCustomDamage )
{
	float flDamage = BaseClass::GetMeleeDamage( pTarget, piDamageType, piCustomDamage );

	if ( !HasDamageBoost() )
		return flDamage;

	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
	if ( !pOwner )
		return 0;

	float flOwnerHealthRatio = (float) pOwner->GetHealth() / (float) pOwner->GetMaxHealth();
	float flDamageScale = RemapValClamped( flOwnerHealthRatio, 0.f, 1.f, 1.65f, 0.5f );

	return flDamage * flDamageScale;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFShovel::GetSpeedMod( void )
{
	if ( m_bHolstering || !HasSpeedBoost() )
		return 1.f;

	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
	if ( !pOwner )
		return 0;

	float flOwnerHealthRatio = (float) pOwner->GetHealth() / (float) pOwner->GetMaxHealth();
	if ( flOwnerHealthRatio > 0.8 )
		return 1.f;
	else if ( flOwnerHealthRatio > 0.6 )
		return 1.1f;
	else if ( flOwnerHealthRatio > 0.4 )
		return 1.2f;
	else if ( flOwnerHealthRatio > 0.2 )
		return 1.4f;
	else
		return 1.6f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFShovel::Deploy( void )
{
	bool ret = BaseClass::Deploy();

	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
	if ( pOwner && HasSpeedBoost() )
	{
		SetContextThink( &CTFShovel::MoveSpeedThink, gpGlobals->curtime + 0.25f, "SHOVEL_SPEED_THINK" );
	}

	return ret;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFShovel::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	m_bHolstering = true;
	bool ret = BaseClass::Holster( pSwitchingTo );
	m_bHolstering = false;

	return ret;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFShovel::MoveSpeedThink( void )
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
	if ( !pOwner || !pOwner->IsAlive() )
		return;

	if ( this != pOwner->GetActiveWeapon() )
		return;

	pOwner->TeamFortress_SetSpeed();

	SetContextThink( &CTFShovel::MoveSpeedThink, gpGlobals->curtime + 0.25f, "SHOVEL_SPEED_THINK" );
}

#ifndef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFShovel::GetForceScale( void )
{
	if ( HasDamageBoost() )
	{
		return BaseClass::GetForceScale() * 2.f;
	}
	else
	{
		return 1.f;
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int CTFShovel::GetDamageCustom()
{
	if ( GetShovelType() == SHOVEL_SPEED_BOOST || GetShovelType() == SHOVEL_DAMAGE_BOOST )
	{
		return TF_DMG_CUSTOM_PICKAXE;
	}
	else
	{
		return BaseClass::GetDamageCustom();
	}
}
#endif
