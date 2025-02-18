//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "tf_weapon_invis.h"
#include "in_buttons.h"

#if !defined( CLIENT_DLL )
	#include "vguiscreen.h"
	#include "tf_player.h"
#else
	#include "c_tf_player.h"
#endif

extern ConVar tf_spy_invis_unstealth_time;
extern ConVar tf_spy_cloak_consume_rate;
extern ConVar tf_spy_cloak_regen_rate;

//=============================================================================
//
// TFWeaponBase Melee tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFWeaponInvis, DT_TFWeaponInvis )

BEGIN_NETWORK_TABLE( CTFWeaponInvis, DT_TFWeaponInvis )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFWeaponInvis )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_invis, CTFWeaponInvis );
PRECACHE_WEAPON_REGISTER( tf_weapon_invis );

// Server specific.
#if !defined( CLIENT_DLL ) 
	BEGIN_DATADESC( CTFWeaponInvis )
	END_DATADESC()
#endif

//-----------------------------------------------------------------------------
// Purpose: Use the offhand view model
//-----------------------------------------------------------------------------
void CTFWeaponInvis::Spawn( void )
{
	BaseClass::Spawn();

	SetViewModelIndex( 1 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWeaponInvis::OnActiveStateChanged( int iOldState )
{
	BaseClass::OnActiveStateChanged( iOldState );

	// If we are being removed, we need to remove all stealth effects from our owner
	if ( m_iState == WEAPON_NOT_CARRIED && iOldState != WEAPON_NOT_CARRIED )
	{
		CleanupInvisibilityWatch();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Clear out the view model when we hide
//-----------------------------------------------------------------------------
void CTFWeaponInvis::HideThink( void )
{ 
	SetWeaponVisible( false );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CTFWeaponInvis::GetViewModel( int viewmodelindex  ) const
{
	// Watch uses the player model as its viewmodel, because it's never seen being carried by the player
	const CEconItemView *pItem = GetAttributeContainer()->GetItem();
	if ( pItem->IsValid() )
	{
		int iClass = 0;
		int iTeam = 0;
		CTFPlayer *pTFPlayer = ToTFPlayer( GetOwnerEntity() );
		if ( pTFPlayer )
		{
			iClass = pTFPlayer->GetPlayerClass()->GetClassIndex();
			iTeam = pTFPlayer->GetTeamNumber();
		}

		return pItem->GetPlayerDisplayModel( iClass, iTeam );
	}

	return BaseClass::GetViewModel( viewmodelindex );
}

//-----------------------------------------------------------------------------
// Purpose: Show/hide weapon and corresponding view model if any
// Input  : visible - 
//-----------------------------------------------------------------------------
void CTFWeaponInvis::SetWeaponVisible( bool visible )
{
	CBaseViewModel *vm = NULL;

	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	if ( pOwner )
	{
		vm = pOwner->GetViewModel( m_nViewModelIndex );
	}

	if ( visible )
	{
		RemoveEffects( EF_NODRAW );
		if ( vm )
		{
			vm->RemoveEffects( EF_NODRAW );
		}
	}
	else
	{
		AddEffects( EF_NODRAW );
		if ( vm )
		{
			vm->AddEffects( EF_NODRAW );
		}
	}
}

//-----------------------------------------------------------------------------
bool CTFWeaponInvis::Deploy( void )
{
	bool b = BaseClass::Deploy();

	SetWeaponIdleTime( gpGlobals->curtime + 1.5 );

	return b;
}

//-----------------------------------------------------------------------------
bool CTFWeaponInvis::Holster( CBaseCombatWeapon *pSwitchingTo )
{ 
	bool bHolster = BaseClass::Holster( pSwitchingTo );

	// far in the future
	SetWeaponIdleTime( gpGlobals->curtime + 10 );

	return bHolster;
}

//-----------------------------------------------------------------------------
void CTFWeaponInvis::PrimaryAttack( void )
{
	// do nothing
}

//-----------------------------------------------------------------------------
void CTFWeaponInvis::SecondaryAttack( void )
{
	// do nothing
}

//-----------------------------------------------------------------------------
void CTFWeaponInvis::ItemBusyFrame( void )
{
	// do nothing
}

//-----------------------------------------------------------------------------
// Purpose: the player alt-fired or otherwise activated the functionality of
//			the invisibility watch
//-----------------------------------------------------------------------------
bool CTFWeaponInvis::ActivateInvisibilityWatch( void )
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
	if ( !pOwner )
		return false;

	SetCloakRates();

	bool bDoSkill = false;
	// If we're in TF_COND_STEALTHED - which means we gave it ourselves - always remove it
	// If we're in TF_COND_STEALTHED_USER_BUFF and we have Dead Ringer, allow it to be toggled
	// since we're allowed to fire from stealth
	if ( pOwner->m_Shared.InCond( TF_COND_STEALTHED ) )
	{
		// De-cloak.
		float flDecloakRate = 0.0f;
		CALL_ATTRIB_HOOK_FLOAT( flDecloakRate, mult_decloak_rate );
		if ( flDecloakRate <= 0.0f )
			flDecloakRate = 1.0f;

		pOwner->m_Shared.FadeInvis( 1.0f );
	}
	else
	{
		if ( HasFeignDeath() )
		{
			if ( pOwner->m_Shared.IsFeignDeathReady() )
			{
				// Turn it off...
				SetFeignDeathState( false );
			}
			else if ( pOwner->m_Shared.GetSpyCloakMeter() == 100.f )
			{
				// Turn it on...
				SetFeignDeathState( true );
			}
		}
		else if ( pOwner->CanGoInvisible() && ( pOwner->m_Shared.GetSpyCloakMeter() > 8.0f ) )	// must have over 10% cloak to start
		{
			// Do standard cloak.
			pOwner->m_Shared.AddCond( TF_COND_STEALTHED, -1.f, pOwner );


			bDoSkill = true;
		}
	}

	if ( bDoSkill )
	{
		pOwner->m_Shared.SetNextStealthTime( gpGlobals->curtime + 0.5 );
	}
	else
	{
		pOwner->m_Shared.SetNextStealthTime( gpGlobals->curtime + 0.1 );
	}

	return bDoSkill;
}

//-----------------------------------------------------------------------------
// Purpose: the player has changed loadouts or done something else that causes
//			us to clean up any side effects of our watch
//-----------------------------------------------------------------------------
void CTFWeaponInvis::CleanupInvisibilityWatch( void )
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
	if ( !pOwner )
		return;

	pOwner->m_Shared.SetFeignDeathReady( false );

	if ( pOwner->m_Shared.IsStealthed() )
	{
		// De-cloak.
		pOwner->m_Shared.FadeInvis( 1.0f );
	}

	pOwner->HolsterOffHandWeapon();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWeaponInvis::SetFeignDeathState( bool bEnabled )
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
	if ( !pOwner )
		return;

	if ( pOwner->m_Shared.InCond( TF_COND_GRAPPLINGHOOK ) )
		return;

	if ( bEnabled )
	{
		pOwner->m_Shared.SetFeignDeathReady( true );
		pOwner->SetOffHandWeapon( this );
		pOwner->m_Shared.SetNextStealthTime( gpGlobals->curtime + 0.5 );
	}
	else
	{
		pOwner->m_Shared.SetFeignDeathReady( false );
		if ( !pOwner->m_Shared.InCond( TF_COND_STEALTHED ) )
		{
			pOwner->HolsterOffHandWeapon();
			if ( pOwner->GetActiveWeapon() )
			{
				pOwner->GetActiveWeapon()->m_flNextPrimaryAttack = gpGlobals->curtime + 0.1f;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Set the correct cloak consume & regen rates for this item.
//-----------------------------------------------------------------------------
void CTFWeaponInvis::SetCloakRates( void )
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );

	float fCloakConsumeRate = tf_spy_cloak_consume_rate.GetFloat();
	float fCloakConsumeFactor = 1.0f;
	CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pOwner, fCloakConsumeFactor, mult_cloak_meter_consume_rate );	// Ask the owner since this attr may come from another weapon
	
	// This value is a inverse scale and does not match expectations on description
	// so we subtract and invert to make it align
	// ie 25% (0.75% in schema) makes 10seconds go to 13.3 when we want 12.5
	// 2 - 0.75 = 1.25... 1 / 1.25 = 0.8.. Consume rate "8". 10s / 0.8 = 12.5s
	if ( fCloakConsumeFactor < 1.0f )
	{
		fCloakConsumeFactor = 1.0f / (2.0f - fCloakConsumeFactor);
	}
	
	pOwner->m_Shared.SetCloakConsumeRate( fCloakConsumeRate * fCloakConsumeFactor );

	float fCloakRegenRate = tf_spy_cloak_regen_rate.GetFloat();
	float fCloakRegenFactor = 1.0f;
	CALL_ATTRIB_HOOK_FLOAT( fCloakRegenFactor, mult_cloak_meter_regen_rate );
	pOwner->m_Shared.SetCloakRegenRate( fCloakRegenRate * fCloakRegenFactor );
}

#ifndef CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose: Return the right pda panel for the watch model we're using
//-----------------------------------------------------------------------------
void CTFWeaponInvis::GetControlPanelInfo( int nPanelIndex, const char *&pPanelName )
{
	const char *pszViewModel = GetViewModel(0);
	if ( Q_stristr( pszViewModel, "pocket" ) )
	{
		pPanelName = "pda_panel_spy_invis_pocket";
	}
	else if ( Q_stristr( pszViewModel, "ttg_watch_spy" ) )
	{
		pPanelName = "pda_panel_spy_invis_pocket_ttg";
	}
	else if ( Q_stristr( pszViewModel, "hm_watch" ) )
	{
		pPanelName = "pda_panel_spy_invis_pocket_hm";
	}
	else
	{
		pPanelName = "pda_panel_spy_invis";
	}
}

#endif
