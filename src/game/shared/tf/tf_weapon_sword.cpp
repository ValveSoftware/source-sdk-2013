//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "tf_weapon_sword.h"

// Client specific.
#ifdef CLIENT_DLL
#include "c_tf_player.h"
#include "econ_entity.h"
// Server specific.
#else
#include "tf_player.h"
#endif

//=============================================================================
//
// Weapon Sword tables.
//

// CTFSword
IMPLEMENT_NETWORKCLASS_ALIASED( TFSword, DT_TFWeaponSword )

BEGIN_NETWORK_TABLE( CTFSword, DT_TFWeaponSword )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFSword )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_sword, CTFSword );
PRECACHE_WEAPON_REGISTER( tf_weapon_sword );

// CTFKatana
IMPLEMENT_NETWORKCLASS_ALIASED( TFKatana, DT_TFWeaponKatana )

BEGIN_NETWORK_TABLE( CTFKatana, DT_TFWeaponKatana )
#ifdef CLIENT_DLL
	RecvPropBool( RECVINFO( m_bIsBloody ) ),
#else
	SendPropBool( SENDINFO( m_bIsBloody ) ),
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFKatana )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_katana, CTFKatana );
PRECACHE_WEAPON_REGISTER( tf_weapon_katana );

//=============================================================================
//
// Decapitation melee weapon base implementation.
//

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFDecapitationMeleeWeaponBase::CTFDecapitationMeleeWeaponBase()
	: m_bHolstering( false )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFDecapitationMeleeWeaponBase::Precache()
{
	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFDecapitationMeleeWeaponBase::GetMeleeDamage( CBaseEntity *pTarget, int* piDamageType, int* piCustomDamage )
{
	float flBaseDamage = BaseClass::GetMeleeDamage( pTarget, piDamageType, piCustomDamage );

	*piCustomDamage = TF_DMG_CUSTOM_DECAPITATION; 

	return flBaseDamage;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Activity CTFDecapitationMeleeWeaponBase::TranslateViewmodelHandActivityInternal( Activity actBase )
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return BaseClass::TranslateViewmodelHandActivityInternal( actBase );

	CEconItemView *pEconItemView = GetAttributeContainer()->GetItem();
	if ( pEconItemView )
	{
		if ( pEconItemView->GetAnimationSlot() == TF_WPN_TYPE_MELEE_ALLCLASS )
			return BaseClass::TranslateViewmodelHandActivityInternal( actBase );
	}

	// Alright, so we have some decapitation weapons (katana) that can be used
	// by both the soldier and the demoman, but the classes play totally different
	// animations using the same weapon.
	//
	// This logic is also responsible for playing the correct animations on the
	// demo when he's using non-shared weapons like the Eyelanders.
	if ( pPlayer->GetPlayerClass()->GetClassIndex() == TF_CLASS_DEMOMAN )
	{
		switch ( actBase )
		{
		case ACT_VM_DRAW:
			actBase = ACT_VM_DRAW_SPECIAL;
			break;
		case ACT_VM_HOLSTER:
			actBase = ACT_VM_HOLSTER_SPECIAL;
			break;
		case ACT_VM_IDLE:
			actBase = ACT_VM_IDLE_SPECIAL;
			break;
		case ACT_VM_PULLBACK:
			actBase = ACT_VM_PULLBACK_SPECIAL;
			break;
		case ACT_VM_PRIMARYATTACK:
			actBase = ACT_VM_PRIMARYATTACK_SPECIAL;
			break;
		case ACT_VM_SECONDARYATTACK:
			actBase = ACT_VM_PRIMARYATTACK_SPECIAL;
			break;
		case ACT_VM_HITCENTER:
			actBase = ACT_VM_HITCENTER_SPECIAL;
			break;
		case ACT_VM_SWINGHARD:
			actBase = ACT_VM_SWINGHARD_SPECIAL;
			break;
		case ACT_VM_IDLE_TO_LOWERED:
			actBase = ACT_VM_IDLE_TO_LOWERED_SPECIAL;
			break;
		case ACT_VM_IDLE_LOWERED:
			actBase = ACT_VM_IDLE_LOWERED_SPECIAL;
			break;
		case ACT_VM_LOWERED_TO_IDLE:
			actBase = ACT_VM_LOWERED_TO_IDLE_SPECIAL;
			break;
		default:
			break;
		}
	}

	return BaseClass::TranslateViewmodelHandActivityInternal( actBase );
}
/*
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFDecapitationMeleeWeaponBase::SendWeaponAnim( int iActivity )
{

}*/

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFDecapitationMeleeWeaponBase::CanDecapitate( void )
{
	CEconItemView *pScriptItem = GetAttributeContainer()->GetItem();
	if ( !pScriptItem )
		return false;

	CEconItemDefinition *pStaticData = pScriptItem->GetStaticData();
	if ( !pStaticData )
		return false;

	int iDecapitateType = 0;
	CALL_ATTRIB_HOOK_INT( iDecapitateType, decapitate_type );
	return iDecapitateType != 0;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFDecapitationMeleeWeaponBase::SetupGameEventListeners( void )
{
	ListenForGameEvent( "player_death" );
}

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFDecapitationMeleeWeaponBase::UpdateAttachmentModels( void )
{
	BaseClass::UpdateAttachmentModels();

	CTFPlayer *pTFPlayer = GetTFPlayerOwner();
	if ( !pTFPlayer )
		return;

	if ( !pTFPlayer->IsLocalPlayer() )
		return;

	if ( !pTFPlayer->GetViewModel() )
		return;

	if ( !pTFPlayer->m_Shared.IsShieldEquipped() )
		return;

	CTFWearableDemoShield* pMyShield = dynamic_cast<CTFWearableDemoShield*>( m_hShield.Get() );

	if ( pMyShield )
	{
		pMyShield->UpdateAttachmentModels();
	}
	else
	{
		// Find a shield wearable...
		for ( int i=0; i<pTFPlayer->GetNumWearables(); ++i )
		{
			CEconWearable* pItem = pTFPlayer->GetWearable(i);
			if ( !pItem )
				continue;

			pMyShield = dynamic_cast<CTFWearableDemoShield*>( pItem );
			if ( !pMyShield )
				continue;

			m_hShield.Set( pMyShield );
		}

		if ( pMyShield )
		{
			pMyShield->UpdateAttachmentModels();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CTFDecapitationMeleeWeaponBase::DrawOverriddenViewmodel( C_BaseViewModel *pViewmodel, int flags )
{
	int iRes = BaseClass::DrawOverriddenViewmodel( pViewmodel, flags );

	CTFWearableDemoShield* pMyShield = dynamic_cast<CTFWearableDemoShield*>( m_hShield.Get() );

	if ( pMyShield )
	{
		pMyShield->DrawOverriddenViewmodel( pViewmodel, flags );
	}

	return iRes;
}
#endif // CLIENT_DLL

//=============================================================================
//
// Weapon Sword functions.
//

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFSword::WeaponReset( void )
{
	BaseClass::WeaponReset();

#ifdef CLIENT_DLL
	m_flNextIdleWavRoll = 0;
	m_iPrevWavDecap = 0;
#endif
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int	CTFSword::GetSwingRange( void )
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
	if ( pOwner && pOwner->m_Shared.InCond( TF_COND_SHIELD_CHARGE ) )
	{
		return 128;
	}
	else
	{
		//int iRange = 0;
		//CALL_ATTRIB_HOOK_INT( iRange, is_a_sword )
		//return 72;
		return 72;
	}
}

//-----------------------------------------------------------------------------
// Purpose: A speed mod that is applied even if the weapon isn't in hand.
//-----------------------------------------------------------------------------
float CTFSword::GetSwordSpeedMod( void )
{
	if ( m_bHolstering )
		return 1.f;

	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
	if ( !pOwner )
		return 0;

	if ( !CanDecapitate() )
		return 1.f;

	int iDecaps = MIN( MAX_DECAPITATIONS, pOwner->m_Shared.GetDecapitations() );
	return 1.f + (iDecaps * 0.08f);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFSword::GetSwordHealthMod( void )
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
	if ( !pOwner )
		return 0;

	if ( !CanDecapitate() )
		return 0.f;

	int iDecaps = MIN( MAX_DECAPITATIONS, pOwner->m_Shared.GetDecapitations() );
	return (iDecaps * 15);
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFSword::OnDecapitation( CTFPlayer *pDeadPlayer )
{
	BaseClass::OnDecapitation( pDeadPlayer );

#ifdef GAME_DLL
	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
	Assert( pOwner );

	// The pyro's scythe is actually a "sword" as far as the gameplay code is concerned,
	// but we don't want the demo's head-collecting silliness to apply.
	if ( pOwner->GetPlayerClass()->GetClassIndex() == TF_CLASS_DEMOMAN )
	{
		// We have chopped someone's bloody 'ead off!
		int iDecap = pOwner->m_Shared.GetDecapitations();

		// transfer decapitations
		if ( pDeadPlayer )
		{
			iDecap += pDeadPlayer->m_Shared.GetDecapitations();
		}
		pOwner->m_Shared.SetDecapitations( ++iDecap );
		pOwner->TeamFortress_SetSpeed();
		if ( pOwner->m_Shared.GetBestOverhealDecayMult() == -1.f )
		{
			pOwner->m_Shared.SetBestOverhealDecayMult( 0.25f );
		}
		if ( pOwner->GetHealth() < pOwner->m_Shared.GetMaxBuffedHealth() )
		{
			pOwner->TakeHealth( 15, DMG_IGNORE_MAXHEALTH );
		}
		if ( !pOwner->m_Shared.InCond( TF_COND_DEMO_BUFF ) )
		{
			pOwner->m_Shared.AddCond( TF_COND_DEMO_BUFF );
		}
	}
#endif // GAME_DLL
}

#ifdef GAME_DLL

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFDecapitationMeleeWeaponBase::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	m_bHolstering = true;
	bool res = BaseClass::Holster( pSwitchingTo );
	m_bHolstering = false;
	if ( res )
	{
		StopListeningForAllEvents();
	}
	return res;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFDecapitationMeleeWeaponBase::FireGameEvent( IGameEvent *event )
{
	const char *pszEventName = event->GetName();
	if ( Q_strcmp( pszEventName, "player_death" ) != 0 )
		return;

	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
	if ( !pOwner )
		return;

	int iAttackerID = event->GetInt( "attacker" );
	if ( iAttackerID != pOwner->GetUserID() )
		return;

	int iWeaponID = event->GetInt( "weaponid" );
	int iCustomKill = event->GetInt( "customkill" );
	if ( iWeaponID != TF_WEAPON_SWORD && iCustomKill != TF_DMG_CUSTOM_TAUNTATK_BARBARIAN_SWING )
		return;

	// Off with their heads!
	if ( !CanDecapitate() )
		return;
	
	OnDecapitation( ToTFPlayer( UTIL_PlayerByUserId( event->GetInt( "userid" ) ) ) );
}
#endif

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFSword::Deploy( void )
{
	bool res = BaseClass::Deploy();

	if ( !CanDecapitate() )
	{
		CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
		if ( !pOwner )
			return res;

		pOwner->m_Shared.SetDecapitations( 0 );

		return res;
	}

#ifdef GAME_DLL
	if ( res )
	{
		SetupGameEventListeners();
	}
#else
	// When we go active, there's a chance we immediately thirst for heads.
	if ( RandomInt( 1,4 ) == 1 )
	{
		m_flNextIdleWavRoll = gpGlobals->curtime + 3.0;
	}
	else
	{
		m_flNextIdleWavRoll = gpGlobals->curtime + RandomFloat( 5.0, 30.0 );
	}
#endif

	return res;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFSword::GetCount( void )
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
	if ( !pOwner )
		return 0;

	if ( !CanDecapitate() )
		return 0;

	return pOwner->m_Shared.GetDecapitations();
}

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFSword::WeaponIdle( void )
{
	BaseClass::WeaponIdle();

	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
	if ( !pOwner )
		return;

	if ( !CanDecapitate() )
		return;

	// If we've decapped someone recently, we roll shortly afterwards
	int iDecaps = pOwner->m_Shared.GetDecapitations();
	if ( m_iPrevWavDecap < iDecaps )
	{
		 m_flNextIdleWavRoll = MIN( m_flNextIdleWavRoll, gpGlobals->curtime + RandomFloat(3,5) );
	}
	m_iPrevWavDecap = iDecaps;

	// Randomly play sounds to the local player. The more decaps we have, the more chance it's a good wav.
	if ( m_flNextIdleWavRoll < gpGlobals->curtime )
	{
		float flChance = RemapValClamped( iDecaps, 0, 10, 0.25, 0.9 );
		if ( RandomFloat() <= flChance )
		{
			// Chance to get the more powerful wav: 
			float flChanceForGoodWav = RemapValClamped( iDecaps, 0, 10, 0.1, 0.75 );
			if ( RandomFloat() <= flChanceForGoodWav )
			{
				WeaponSound( SPECIAL2 );
			}
			else
			{
				WeaponSound( SPECIAL1 );
			}
		}

		m_flNextIdleWavRoll = gpGlobals->curtime + RandomFloat( 30.0, 60.0 );
	}
}

#endif


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFKatana::CTFKatana()
{
	m_bIsBloody = false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFKatana::Deploy( void )
{
	bool res = BaseClass::Deploy();

	m_bIsBloody = false;

	if ( CanDecapitate() && res )
	{
#ifdef GAME_DLL
		SetupGameEventListeners();
#endif
	}

	return res;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
float CTFKatana::GetMeleeDamage( CBaseEntity *pTarget, int* piDamageType, int* piCustomDamage )
{
	// Start with our base damage. We use this to generate our custom damage flags,
	// if any. We may trash the damage amount.
	float fDamage = BaseClass::GetMeleeDamage( pTarget, piDamageType, piCustomDamage );

	// The katana is a weapon of honor!!!! (Hitting someone wielding a katana with
	// your katana results in a massive damage boost, a one-hit kill.)
	if ( IsHonorBound() )
	{
		CTFPlayer *pTFPlayerTarget = ToTFPlayer( pTarget );
		if ( pTFPlayerTarget )
		{
			// If our victim is wielding the weapon we're looking for, bump the damage way up.
			if ( pTFPlayerTarget->GetActiveTFWeapon() && pTFPlayerTarget->GetActiveTFWeapon()->IsHonorBound() )
			{
				fDamage = MAX( fDamage, pTFPlayerTarget->GetHealth() * 3 );
				*piDamageType |= DMG_DONT_COUNT_DAMAGE_TOWARDS_CRIT_RATE;
			}
		}
	}

	return fDamage;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int CTFKatana::GetActivityWeaponRole() const
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( pPlayer && pPlayer->GetPlayerClass()->GetClassIndex() == TF_CLASS_DEMOMAN )
	{
		// demo should use act table item1
		return TF_WPN_TYPE_ITEM1;
	}

	return BaseClass::GetActivityWeaponRole();
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFKatana::OnDecapitation( CTFPlayer *pDeadPlayer )
{
	BaseClass::OnDecapitation( pDeadPlayer );

#ifndef CLIENT_DLL
	m_bIsBloody = true;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFKatana::GetSkinOverride() const
{
 	if ( m_bIsBloody && !UTIL_IsLowViolence() )
	{
		CTFPlayer *pPlayer = GetTFPlayerOwner();
		if ( pPlayer )
		{
			return ( ( pPlayer->GetTeamNumber() == TF_TEAM_RED ) ? 2 : 3 );
		}
	}
	
	return BaseClass::GetSkinOverride();
}
