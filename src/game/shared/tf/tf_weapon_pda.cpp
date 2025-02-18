//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"

#ifdef CLIENT_DLL
#include "iinput.h"
#endif

#include "tf_weapon_pda.h"
#include "in_buttons.h"
#include "tf_gamerules.h"
#include "tf_weaponbase_gun.h"

// Server specific.
#if !defined( CLIENT_DLL )
	#include "tf_player.h"
	#include "vguiscreen.h"
// Client specific.
#else
	#include "c_tf_player.h"
	#include <igameevents.h>
	#include "tf_hud_menu_engy_build.h"
	#include "tf_hud_menu_engy_destroy.h"
	#include "tf_hud_menu_spy_disguise.h"
	#include "prediction.h"
#endif

//=============================================================================
//
// TFWeaponBase Melee tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFWeaponPDA, DT_TFWeaponPDA )

BEGIN_NETWORK_TABLE( CTFWeaponPDA, DT_TFWeaponPDA )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFWeaponPDA )
END_PREDICTION_DATA()

// Server specific.
#if !defined( CLIENT_DLL ) 
BEGIN_DATADESC( CTFWeaponPDA )
END_DATADESC()
#endif

LINK_ENTITY_TO_CLASS( tf_pda_expansion_dispenser, CTFWeaponPDAExpansion_Dispenser );
IMPLEMENT_NETWORKCLASS_ALIASED( TFWeaponPDAExpansion_Dispenser, DT_TFWeaponPDAExpansion_Dispenser )

// Network Table --
BEGIN_NETWORK_TABLE( CTFWeaponPDAExpansion_Dispenser, DT_TFWeaponPDAExpansion_Dispenser )
END_NETWORK_TABLE()
// -- Network Table

// Data Desc --
BEGIN_DATADESC( CTFWeaponPDAExpansion_Dispenser )
END_DATADESC()

//************************************************************************************************
LINK_ENTITY_TO_CLASS( tf_pda_expansion_teleporter, CTFWeaponPDAExpansion_Teleporter );
IMPLEMENT_NETWORKCLASS_ALIASED( TFWeaponPDAExpansion_Teleporter, DT_TFWeaponPDAExpansion_Teleporter )

// Network Table --
BEGIN_NETWORK_TABLE( CTFWeaponPDAExpansion_Teleporter, DT_TFWeaponPDAExpansion_Teleporter )
END_NETWORK_TABLE()
// -- Network Table

// Data Desc --
BEGIN_DATADESC( CTFWeaponPDAExpansion_Teleporter )
END_DATADESC()


CTFWeaponPDA::CTFWeaponPDA()
{
}


void CTFWeaponPDA::Spawn()
{
	BaseClass::Spawn();
}

//-----------------------------------------------------------------------------
// Purpose: cancel menu
//-----------------------------------------------------------------------------
void CTFWeaponPDA::PrimaryAttack( void )
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
	if ( !pOwner )
	{
		return;
	}

	pOwner->SelectLastItem();
}

//-----------------------------------------------------------------------------
// Purpose: toggle invis
//-----------------------------------------------------------------------------
void CTFWeaponPDA::SecondaryAttack( void )
{
	// semi-auto behaviour
	if ( m_bInAttack2 )
		return;

	// Get the player owning the weapon.
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( !pPlayer )
		return;

	pPlayer->DoClassSpecialSkill();

	m_bInAttack2 = true;

	m_flNextSecondaryAttack = gpGlobals->curtime + 0.5;
}

#if !defined( CLIENT_DLL )

	void CTFWeaponPDA::Precache()
	{
		BaseClass::Precache();
		PrecacheVGuiScreen( GetPanelName() );
	}

	//-----------------------------------------------------------------------------
	// Purpose: Gets info about the control panels
	//-----------------------------------------------------------------------------
	void CTFWeaponPDA::GetControlPanelInfo( int nPanelIndex, const char *&pPanelName )
	{
		pPanelName = GetPanelName();
	}

#else

	float CTFWeaponPDA::CalcViewmodelBob( void )
	{
		// no bob
		return BaseClass::CalcViewmodelBob();
	}

#endif

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CTFWeaponPDA::ShouldShowControlPanels( void )
{
	return true;
}

#ifdef CLIENT_DLL
void CTFWeaponPDA::OnDataChanged( DataUpdateType_t type )
{
	if ( m_iState != m_iOldState && GetOwner() == C_TFPlayer::GetLocalTFPlayer() )
	{
		// Was active, now not
		if ( m_iOldState == WEAPON_IS_ACTIVE && m_iState != m_iOldState )
		{
			CHudBaseBuildMenu *pBuildMenu = GetBuildMenu();
			Assert( pBuildMenu );
			if ( pBuildMenu )
			{
				pBuildMenu->SetBuilderEquipped( false );
			}
		}
		else if ( m_iState == WEAPON_IS_ACTIVE && m_iOldState == WEAPON_IS_CARRIED_BY_PLAYER ) // Was inactive, now is
		{
			CHudBaseBuildMenu *pBuildMenu = GetBuildMenu();
			Assert( pBuildMenu );
			if ( pBuildMenu )
			{
				pBuildMenu->SetBuilderEquipped( true );
			}
		}
	}

	BaseClass::OnDataChanged( type );
}


void CTFWeaponPDA::UpdateOnRemove()
{
	CHudBaseBuildMenu *pBuildMenu = GetBuildMenu();
	Assert( pBuildMenu );
	if ( pBuildMenu )
	{
		pBuildMenu->SetBuilderEquipped( false );
	}
	return BaseClass::UpdateOnRemove();
}

#endif

//==============================

IMPLEMENT_NETWORKCLASS_ALIASED( TFWeaponPDA_Engineer_Build, DT_TFWeaponPDA_Engineer_Build )

BEGIN_NETWORK_TABLE( CTFWeaponPDA_Engineer_Build, DT_TFWeaponPDA_Engineer_Build )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFWeaponPDA_Engineer_Build )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_pda_engineer_build, CTFWeaponPDA_Engineer_Build );
PRECACHE_WEAPON_REGISTER( tf_weapon_pda_engineer_build );

#ifdef CLIENT_DLL
CHudBaseBuildMenu *CTFWeaponPDA_Engineer_Build::GetBuildMenu() const
{ 
	return GET_HUDELEMENT( CHudMenuEngyBuild );
}
#endif // CLIENT_DLL


//==============================

IMPLEMENT_NETWORKCLASS_ALIASED( TFWeaponPDA_Engineer_Destroy, DT_TFWeaponPDA_Engineer_Destroy )

BEGIN_NETWORK_TABLE( CTFWeaponPDA_Engineer_Destroy, DT_TFWeaponPDA_Engineer_Destroy )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFWeaponPDA_Engineer_Destroy )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_pda_engineer_destroy, CTFWeaponPDA_Engineer_Destroy );
PRECACHE_WEAPON_REGISTER( tf_weapon_pda_engineer_destroy );

#ifdef CLIENT_DLL
CHudBaseBuildMenu *CTFWeaponPDA_Engineer_Destroy::GetBuildMenu() const 
{ 
	return GET_HUDELEMENT( CHudMenuEngyDestroy );
}
#endif // CLIENT_DLL

//==============================

IMPLEMENT_NETWORKCLASS_ALIASED( TFWeaponPDA_Spy, DT_TFWeaponPDA_Spy )

BEGIN_NETWORK_TABLE( CTFWeaponPDA_Spy, DT_TFWeaponPDA_Spy )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFWeaponPDA_Spy )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_pda_spy, CTFWeaponPDA_Spy );
PRECACHE_WEAPON_REGISTER( tf_weapon_pda_spy );

#ifdef CLIENT_DLL
CHudBaseBuildMenu *CTFWeaponPDA_Spy::GetBuildMenu() const
{ 
	return GET_HUDELEMENT( CHudMenuSpyDisguise );
}
#endif // CLIENT_DLL


//==============================

void CTFWeaponPDA_Spy::ItemPreFrame( void )
{
	BaseClass::ItemPreFrame();

	ProcessDisguiseImpulse();
	CheckDisguiseTimer();
}

void CTFWeaponPDA_Spy::ItemBusyFrame( void )
{
	BaseClass::ItemBusyFrame();

	ProcessDisguiseImpulse();
	CheckDisguiseTimer();
}

void CTFWeaponPDA_Spy::ItemHolsterFrame( void )
{
	BaseClass::ItemHolsterFrame();

	ProcessDisguiseImpulse();
	CheckDisguiseTimer();
}

void CTFWeaponPDA_Spy::ProcessDisguiseImpulse( void )
{
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( !pPlayer )
		return;

	pPlayer->m_Shared.ProcessDisguiseImpulse( pPlayer );
}

void CTFWeaponPDA_Spy::CheckDisguiseTimer( void )
{
	/*
	// Get the player owning the weapon.
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( !pPlayer )
		return;

	if ( pPlayer->m_Shared.InCond( TF_COND_DISGUISING ) )
	{
		if ( gpGlobals->curtime > pPlayer->m_Shared.GetDisguiseCompleteTime() )
		{
			pPlayer->m_Shared.CompleteDisguise();
		}
	}
	*/
}

#ifdef CLIENT_DLL

bool CTFWeaponPDA_Spy::Deploy( void )
{
	bool bDeploy = BaseClass::Deploy();

	if ( bDeploy )
	{
		// let the spy pda menu know to reset
		IGameEvent *event = gameeventmanager->CreateEvent( "spy_pda_reset" );
		if ( event )
		{
			gameeventmanager->FireEventClientSide( event );
		}
	}

	return bDeploy;
}

#endif


bool CTFWeaponPDA_Spy::CanBeSelected( void )
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
	if ( pOwner && !pOwner->CanDisguise() )
	{
		return false;
	}

	return BaseClass::CanBeSelected();
}


bool CTFWeaponPDA_Spy::VisibleInWeaponSelection( void )
{
	if ( !CanBeSelected() )
		return false;

	return BaseClass::VisibleInWeaponSelection();
}


//-----------------------------------------------------------------------------
// PDA Expansion Slots
void CTFWeaponPDAExpansion_Dispenser::Equip( CBasePlayer *pOwner )
{
#ifdef GAME_DLL
	CTFPlayer *pPlayer = ToTFPlayer( pOwner );
	if ( pPlayer )
	{
		// Detonate
		CBaseObject *pObject = pPlayer->GetObjectOfType( OBJ_DISPENSER );
		if ( pObject )
		{
			pObject->DetonateObject();
		}
	}
#endif
	BaseClass::Equip( pOwner );
}
//-----------------------------------------------------------------------------
void CTFWeaponPDAExpansion_Dispenser::UnEquip( CBasePlayer *pOwner )
{
#ifdef GAME_DLL
	CTFPlayer *pPlayer = ToTFPlayer( pOwner );
	if ( pPlayer )
	{
		// Detonate
		CBaseObject *pObject = pPlayer->GetObjectOfType( OBJ_DISPENSER );
		if ( pObject )
		{
			pObject->DetonateObject();
		}
	}
#endif
	BaseClass::UnEquip( pOwner );
}

//-----------------------------------------------------------------------------
void CTFWeaponPDAExpansion_Teleporter::Equip( CBasePlayer *pOwner )
{
#ifdef GAME_DLL
	CTFPlayer *pPlayer = ToTFPlayer( pOwner );
	if ( pPlayer )
	{
		// Detonate entrance and exit
		CBaseObject *pObject = pPlayer->GetObjectOfType( OBJ_TELEPORTER, 0 );
		if ( pObject )
		{
			pObject->DetonateObject();
		}
		pObject = pPlayer->GetObjectOfType( OBJ_TELEPORTER, 1 );
		if ( pObject )
		{
			pObject->DetonateObject();
		}

		pObject = pPlayer->GetObjectOfType( OBJ_TELEPORTER, 2 );
		if ( pObject )
		{
			pObject->DetonateObject();
		}
		pObject = pPlayer->GetObjectOfType( OBJ_TELEPORTER, 3 );
		if ( pObject )
		{
			pObject->DetonateObject();
		}
	}
#endif
	BaseClass::Equip( pOwner );
}
//-----------------------------------------------------------------------------
void CTFWeaponPDAExpansion_Teleporter::UnEquip( CBasePlayer *pOwner )
{
#ifdef GAME_DLL
	CTFPlayer *pPlayer = ToTFPlayer( pOwner );
	if ( pPlayer )
	{
		// Detonate entrance and exit
		CBaseObject *pObject = pPlayer->GetObjectOfType( OBJ_TELEPORTER, 0 );
		if ( pObject )
		{
			pObject->DetonateObject();
		}
		pObject = pPlayer->GetObjectOfType( OBJ_TELEPORTER, 1 );
		if ( pObject )
		{
			pObject->DetonateObject();
		}

		pObject = pPlayer->GetObjectOfType( OBJ_TELEPORTER, 2 );
		if ( pObject )
		{
			pObject->DetonateObject();
		}
		pObject = pPlayer->GetObjectOfType( OBJ_TELEPORTER, 3 );
		if ( pObject )
		{
			pObject->DetonateObject();
		}
	}
#endif
	BaseClass::UnEquip( pOwner );
}

//-----------------------------------------------------------------------------
// Purpose: Check if we should show the "destroy" panel
//-----------------------------------------------------------------------------
bool	CTFWeaponPDA_Engineer_Destroy::VisibleInWeaponSelection( void )
{
	if ( IsConsole()
#ifdef CLIENT_DLL
		|| ::input->IsSteamControllerActive()
		|| tf_build_menu_controller_mode.GetBool()
#endif 
		)
	{
		return false;
	}

	return BaseClass::VisibleInWeaponSelection();
}



