//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "tf_weapon_fists.h"
#include "decals.h"

// Client specific.
#ifdef CLIENT_DLL
#include "c_tf_player.h"
#include "c_tf_gamestats.h"
// Server specific.
#else
#include "tf_player.h"
#include "tf_gamestats.h"
#endif

//=============================================================================
//
// Weapon Fists tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFFists, DT_TFWeaponFists )

BEGIN_NETWORK_TABLE( CTFFists, DT_TFWeaponFists )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFFists )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_fists, CTFFists );
PRECACHE_WEAPON_REGISTER( tf_weapon_fists );

//=============================================================================
//
// Weapon Fists functions.
//

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
void CTFFists::ItemPreFrame( void )
{
	return BaseClass::ItemPreFrame();
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
void CTFFists::PrimaryAttack()
{
	if ( !CanAttack() )
		return;

	// Set the weapon usage mode - primary, secondary.
	// reversed for 360 because the primary attack is on the right side of the controller
	if ( IsX360() || IsViewModelFlipped() )
	{
		m_iWeaponMode = TF_WEAPON_SECONDARY_MODE;
	}
	else
	{
		m_iWeaponMode = TF_WEAPON_PRIMARY_MODE;
	}

	Punch();
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
void CTFFists::SecondaryAttack()
{
	if ( !CanAttack() )
		return;

	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( pPlayer && pPlayer->m_Shared.IsControlStunned() )
	{
		return;
	}

	// Set the weapon usage mode - primary, secondary.
	if ( IsX360() || IsViewModelFlipped() )
	{
		m_iWeaponMode = TF_WEAPON_PRIMARY_MODE;
	}
	else
	{
		m_iWeaponMode = TF_WEAPON_SECONDARY_MODE;
	}

	Punch();
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
bool CTFFists::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	return BaseClass::Holster( pSwitchingTo );
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
void CTFFists::Punch( void )
{
	// Get the current player.
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return;

	// Swing the weapon.
	Swing( pPlayer );

	m_flNextSecondaryAttack = m_flNextPrimaryAttack;

#if !defined( CLIENT_DLL ) 
	pPlayer->SpeakWeaponFire();
	CTF_GameStats.Event_PlayerFiredWeapon( pPlayer, IsCurrentAttackACrit() );

	if ( pPlayer->m_Shared.IsStealthed() )
	{
		pPlayer->RemoveInvisibility();
	}
#else
	C_CTF_GameStats.Event_PlayerFiredWeapon( pPlayer, IsCurrentAttackACrit() );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Allow melee weapons to send different anim events
// Input  :  - 
//-----------------------------------------------------------------------------
void CTFFists::SendPlayerAnimEvent( CTFPlayer *pPlayer )
{
	// Send extra activities to the weapon for breadgloves
	if ( IsCurrentAttackACrit() )
	{
		pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_SECONDARY );
	}
	else
	{
		pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFists::DoViewModelAnimation( void )
{
	Activity act;

	if ( IsCurrentAttackACrit() )
	{
		act = ACT_VM_SWINGHARD;
	}
	else
	{
		act = ( m_iWeaponMode == TF_WEAPON_PRIMARY_MODE ) ? ACT_VM_HITLEFT : ACT_VM_HITRIGHT;
	}

	SendWeaponAnim( act );

	// Send WeaponAnim actually sets all anims and we want an override for the world model
	//int iIsBreadgloves = 0;
	//CALL_ATTRIB_HOOK_INT( iIsBreadgloves, breadgloves_properties );
	//if ( iIsBreadgloves )
	//{
	//	ResetSequence( SelectWeightedSequence( ACT_BREADMONSTER_GLOVES_HITRIGHT ) );
	//	SetPlaybackRate( 0.0f );
	//	ResetClientsideFrame();
	//}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFFists::AllowTaunts( void )
{
	// Radial buff fists don't allow player to taunt manually
	return ( GetFistType() != FISTTYPE_RADIAL_BUFF );
}

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFists::OnEntityHit( CBaseEntity *pEntity, CTakeDamageInfo *info )
{
	CTFPlayer *pHitPlayer = ToTFPlayer( pEntity );

	if ( !pHitPlayer )
		return;

	// Get the current player.
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return;

	if ( pHitPlayer->GetTeamNumber() == pPlayer->GetTeamNumber() )
		return;

	if ( pPlayer->m_Shared.InCond( TF_COND_INVULNERABLE ) )
	{
		int iNumHealers = pPlayer->m_Shared.GetNumHealers();

		// for each medic healing me
		for ( int i=0;i<iNumHealers;i++ )
		{
			CTFPlayer *pMedic = ToTFPlayer( pPlayer->m_Shared.GetHealerByIndex( i ) );

			// if it's a medic and that medic is releasing charge
			if ( pMedic && pMedic->GetChargeEffectBeingProvided() == MEDIGUN_CHARGE_INVULN )
			{
				// they are invulning me - add pEntity to their list of people punched 
				pMedic->HandleAchievement_Medic_AssistHeavy( pHitPlayer );
			}
		}
	}

	// If we've killed someone, check to see for the unique fist kill response
	if ( !pEntity->IsAlive() )
	{
		if ( GetFistType() == FISTTYPE_RADIAL_BUFF )
		{
			pPlayer->Taunt();
		}
	}
}
#endif

void CTFFists::SetWeaponVisible( bool visible )
{ 
	if ( visible )	// intentionally never hiding the fists
	{ 
		BaseClass::SetWeaponVisible( visible );
	} 
}  

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFFists::Deploy( void )
{
// #ifdef GAME_DLL
// 	if ( GetFistType() == FISTTYPE_GRU )
// 	{
// 		CTFPlayer *pTFOwner = GetTFPlayerOwner();
// 		if ( pTFOwner )
// 		{
			// TODO: Penalty for spam-equipping gloves
// 		}
// 	}
// #endif

	return BaseClass::Deploy();
}