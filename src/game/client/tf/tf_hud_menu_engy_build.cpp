//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "hud.h"
#include "hudelement.h"
#include "c_tf_player.h"
#include "iclientmode.h"
#include "ienginevgui.h"
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <vgui/IVGui.h>
#include <vgui/IInput.h>
#include "c_baseobject.h"
#include "tf_gamerules.h"
#include "tf_item_inventory.h"
#include "tf_hud_menu_engy_build.h"
#include "inputsystem/iinputsystem.h"

// NVNT haptics for buildings
#include "haptics/haptic_utils.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

// Set to 1 to simulate xbox-style menu interaction
ConVar tf_build_menu_controller_mode( "tf_build_menu_controller_mode", "0", FCVAR_ARCHIVE, "Use console controller build menus. 1 = ON, 0 = OFF." );

const EngyConstructBuilding_t g_kEngyBuildings[ NUM_ENGY_BUILDINGS ] =
{
	// Sentry gun
	EngyConstructBuilding_t( true,
							 OBJ_SENTRYGUN,
							 0,
							 "sentry_active.res",
							 "sentry_already_built.res",
							 "sentry_cant_afford.res",
							 "sentry_unavailable.res",
							 "sentry_active.res",
							 "sentry_inactive.res",
							 "sentry_inactive.res" ),

	// Dispenser
	EngyConstructBuilding_t( true,
							 OBJ_DISPENSER,
							 0,
							 "dispenser_active.res",
							 "dispenser_already_built.res",
							 "dispenser_cant_afford.res",
							 "dispenser_unavailable.res",
							 "dispenser_active.res",
							 "dispenser_inactive.res",
							 "dispenser_inactive.res" ),
	
	// Teleporter entrance
	EngyConstructBuilding_t( true,
							 OBJ_TELEPORTER,
							 MODE_TELEPORTER_ENTRANCE,
							 "tele_entrance_active.res",
							 "tele_entrance_already_built.res",
							 "tele_entrance_cant_afford.res",
							 "tele_entrance_unavailable.res",
							 "tele_entrance_active.res",
							 "tele_entrance_inactive.res",
							 "tele_entrance_inactive.res" ),

	// Teleporter exit
	EngyConstructBuilding_t( true,
							 OBJ_TELEPORTER,
							 MODE_TELEPORTER_EXIT,
							 "tele_exit_active.res",
							 "tele_exit_already_built.res",
							 "tele_exit_cant_afford.res",
							 "tele_exit_unavailable.res",
							 "tele_exit_active.res",
							 "tele_exit_inactive.res",
							 "tele_exit_inactive.res" )
};


static int flagSlots[NUM_ENGY_BUILDINGS] =
{
	0x01,
	0x02,
	0x04,
	0x08
};



//======================================
DECLARE_HUDELEMENT_DEPTH( CHudMenuEngyBuild, 40 );	// in front of engy building status

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudMenuEngyBuild::CHudMenuEngyBuild( const char *pElementName ) 
	: CHudBaseBuildMenu( pElementName, "HudMenuEngyBuild" )
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	SetHiddenBits( HIDEHUD_MISCSTATUS );

	for ( int i=0; i<NUM_ENGY_BUILDINGS; i++ )
	{
		char buf[32];

		Q_snprintf( buf, sizeof(buf), "active_item_%d", i+1 );
		m_pAvailableObjects[i] = new EditablePanel( this, buf );

		Q_snprintf( buf, sizeof(buf), "already_built_item_%d", i+1 );
		m_pAlreadyBuiltObjects[i] = new EditablePanel( this, buf );

		Q_snprintf( buf, sizeof(buf), "cant_afford_item_%d", i+1 );
		m_pCantAffordObjects[i] = new EditablePanel( this, buf );

		Q_snprintf( buf, sizeof(buf), "unavailable_item_%d", i+1 );
		m_pUnavailableObjects[i] = new EditablePanel( this, buf );
	}

	vgui::ivgui()->AddTickSignal( GetVPanel() );

	m_pActiveSelection = NULL;

	m_iSelectedItem = -1;

	m_pBuildLabelBright = NULL;
	m_pBuildLabelDim = NULL;

	m_pDestroyLabelBright = NULL;
	m_pDestroyLabelDim = NULL;

	m_bInConsoleMode = false;
	m_eCurrentBuildMenuLayout = BUILDMENU_DEFAULT;

	RegisterForRenderGroup( "mid" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudMenuEngyBuild::ApplySchemeSettings( IScheme *pScheme )
{
	bool bSteamController = ::input->IsSteamControllerActive();
	bool b360Style = ( bSteamController || IsConsole() || tf_build_menu_controller_mode.GetBool() );

	// load control settings...

	if ( b360Style )
	{
		auto res_dir = bSteamController ? "resource/UI/build_menu_sc" : "resource/UI/build_menu_360";

		LoadControlSettings( VarArgs("%s/HudMenuEngyBuild.res", res_dir ) );

		// Load the already built images, destroyable
		m_pAlreadyBuiltObjects[0]->LoadControlSettings( VarArgs( "%s/sentry_already_built.res", res_dir ) );
		m_pAlreadyBuiltObjects[1]->LoadControlSettings( VarArgs( "%s/dispenser_already_built.res", res_dir ) );
		m_pAlreadyBuiltObjects[2]->LoadControlSettings( VarArgs( "%s/tele_entrance_already_built.res", res_dir ) );
		m_pAlreadyBuiltObjects[3]->LoadControlSettings( VarArgs( "%s/tele_exit_already_built.res", res_dir ) );

		m_pAvailableObjects[0]->LoadControlSettings( VarArgs( "%s/sentry_active.res", res_dir ) );
		m_pAvailableObjects[1]->LoadControlSettings( VarArgs( "%s/dispenser_active.res", res_dir ) );
		m_pAvailableObjects[2]->LoadControlSettings( VarArgs( "%s/tele_entrance_active.res", res_dir ) );
		m_pAvailableObjects[3]->LoadControlSettings( VarArgs( "%s/tele_exit_active.res", res_dir ) );

		m_pCantAffordObjects[0]->LoadControlSettings( VarArgs( "%s/sentry_cant_afford.res", res_dir ) );
		m_pCantAffordObjects[1]->LoadControlSettings( VarArgs( "%s/dispenser_cant_afford.res", res_dir ) );
		m_pCantAffordObjects[2]->LoadControlSettings( VarArgs( "%s/tele_entrance_cant_afford.res", res_dir ) );
		m_pCantAffordObjects[3]->LoadControlSettings( VarArgs( "%s/tele_exit_cant_afford.res", res_dir ) );

		m_pUnavailableObjects[0]->LoadControlSettings( "resource/UI/build_menu/sentry_unavailable.res" );
		m_pUnavailableObjects[1]->LoadControlSettings( "resource/UI/build_menu/dispenser_unavailable.res" );
		m_pUnavailableObjects[2]->LoadControlSettings( "resource/UI/build_menu/tele_entrance_unavailable.res" );
		m_pUnavailableObjects[3]->LoadControlSettings( "resource/UI/build_menu/tele_exit_unavailable.res" );	

		m_pActiveSelection = dynamic_cast< CIconPanel * >( FindChildByName( "active_selection_bg" ) );

		m_pBuildLabelBright = dynamic_cast< CExLabel * >( FindChildByName( "BuildHintLabel_Bright" ) );
		m_pBuildLabelDim = dynamic_cast< CExLabel * >( FindChildByName( "BuildHintLabel_Dim" ) );

		m_pDestroyLabelBright = dynamic_cast< CExLabel * >( FindChildByName( "DestroyHintLabel_Bright" ) );
		m_pDestroyLabelDim = dynamic_cast< CExLabel * >( FindChildByName( "DestroyHintLabel_Dim" ) );

		// Reposition the active selection to the default position
		m_iSelectedItem = -1;	// force reposition
		SetSelectedItem( 1 );
	}
	else
	{
		const char *pszCustomDir = NULL;
		switch( m_eCurrentBuildMenuLayout )
		{
		case BUILDMENU_PIPBOY:
			pszCustomDir = "resource/UI/build_menu/pipboy";
			break;

		default:
		case BUILDMENU_DEFAULT:
			pszCustomDir = "resource/UI/build_menu";
			break;
		}

		LoadControlSettings( VarArgs("%s/HudMenuEngyBuild.res",pszCustomDir) );

		// Load the already built images, not destroyable
		for ( int i=0; i<NUM_ENGY_BUILDINGS; ++i )
		{
			CExLabel *pNumberLabel = NULL;
			m_pAvailableObjects[i]->LoadControlSettings( VarArgs( "%s/%s", pszCustomDir, m_Buildings[i].m_pszConstructAvailableObjectRes ) );
			m_pAlreadyBuiltObjects[i]->LoadControlSettings( VarArgs( "%s/%s", pszCustomDir, m_Buildings[i].m_pszConstructAlreadyBuiltObjectRes ) );
			m_pCantAffordObjects[i]->LoadControlSettings( VarArgs( "%s/%s", pszCustomDir, m_Buildings[i].m_pszConstructCantAffordObjectRes ) );
			m_pUnavailableObjects[i]->LoadControlSettings( VarArgs( "%s/%s", pszCustomDir, m_Buildings[i].m_pszConstructUnavailableObjectRes ) );

			// Set the Numerical Number
			pNumberLabel = dynamic_cast< CExLabel * >( m_pAvailableObjects[i]->FindChildByName( "NumberLabel" ) );
			if ( pNumberLabel )
			{
				pNumberLabel->SetText( VarArgs( "%d", i+1 ) );
			}
			// Set the Numerical Number
			pNumberLabel = dynamic_cast<CExLabel *>( m_pAlreadyBuiltObjects[i]->FindChildByName( "NumberLabel" ) );
			if ( pNumberLabel )
			{
				pNumberLabel->SetText( VarArgs( "%d", i+1 ) );
			}
			// Set the Numerical Number
			pNumberLabel = dynamic_cast<CExLabel *>( m_pCantAffordObjects[i]->FindChildByName( "NumberLabel" ) );
			if ( pNumberLabel )
			{
				pNumberLabel->SetText( VarArgs( "%d", i+1 ) );
			}
			// Set the Numerical Number
			pNumberLabel = dynamic_cast<CExLabel *>( m_pUnavailableObjects[i]->FindChildByName( "NumberLabel" ) );
			if ( pNumberLabel )
			{
				pNumberLabel->SetText( VarArgs( "%d", i+1 ) );
			}
		}

		m_pActiveSelection = NULL;

		m_pBuildLabelBright = NULL;
		m_pBuildLabelDim = NULL;

		m_pDestroyLabelBright = NULL;
		m_pDestroyLabelDim = NULL;
	}

	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

	// Set the cost label
	for ( int i=0; i<NUM_ENGY_BUILDINGS; i++ )
	{
		int iBuilding, iMode;
		GetBuildingIDAndModeFromSlot( i+1, iBuilding, iMode, m_Buildings );
		int iCost = ( pLocalPlayer ) ? pLocalPlayer->m_Shared.CalculateObjectCost( pLocalPlayer, iBuilding ) : GetObjectInfo( iBuilding )->m_Cost;

		m_pAvailableObjects[i]->SetDialogVariable( "metal", iCost );
		m_pAlreadyBuiltObjects[i]->SetDialogVariable( "metal", iCost );
		m_pCantAffordObjects[i]->SetDialogVariable( "metal", iCost );
		m_pUnavailableObjects[i]->SetDialogVariable( "metal", iCost );
	}

	BaseClass::ApplySchemeSettings( pScheme );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudMenuEngyBuild::GetBuildingIDAndModeFromSlot( int iSlot, int &iBuilding, int &iMode, const EngyConstructBuilding_t (&buildings)[ NUM_ENGY_BUILDINGS ] )
{
	iBuilding = OBJ_LAST;
	iMode = 0;
	int index = iSlot - 1;
	if ( index >= 0 && index < NUM_ENGY_BUILDINGS )
	{
		iBuilding = buildings[index].m_iObjectType;
		iMode = buildings[index].m_iMode;
	}
	else
	{
		Assert( !"What slot are we asking for and why?" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Keyboard input hook. Return 0 if handled
//-----------------------------------------------------------------------------
int	CHudMenuEngyBuild::HudElementKeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding )
{
	if ( !ShouldDraw() )
	{
		return 1;
	}

	if ( !down )
	{
		return 1;
	}

	bool bController = ( IsConsole() || ( keynum >= JOYSTICK_FIRST ) );

	if ( bController )
	{
		int iNewSelection = m_iSelectedItem;

		switch( keynum )
		{
		case KEY_XBUTTON_UP:
		case STEAMCONTROLLER_DPAD_UP:
			// jump to last
			iNewSelection = NUM_ENGY_BUILDINGS;
			break;

		case KEY_XBUTTON_DOWN:
		case STEAMCONTROLLER_DPAD_DOWN:
			// jump to first
			iNewSelection = 1;
			break;

		case KEY_XBUTTON_RIGHT:
		case STEAMCONTROLLER_DPAD_RIGHT:
			// move selection to the right
			iNewSelection++;
			if ( iNewSelection > NUM_ENGY_BUILDINGS )
				iNewSelection = 1;
			break;

		case KEY_XBUTTON_LEFT:
		case STEAMCONTROLLER_DPAD_LEFT:
			// move selection to the left
			iNewSelection--;
			if ( iNewSelection < 1 )
				iNewSelection = NUM_ENGY_BUILDINGS;
			break;

		case KEY_XBUTTON_A:
		case KEY_XBUTTON_RTRIGGER:
		case STEAMCONTROLLER_A:
			// build selected item
			SendBuildMessage( m_iSelectedItem );
			return 0;

		case KEY_XBUTTON_Y:
		case KEY_XBUTTON_LTRIGGER:
		case STEAMCONTROLLER_Y:
			{
				// destroy selected item
				bool bSuccess = SendDestroyMessage( m_iSelectedItem );

				if ( bSuccess )
				{
					engine->ExecuteClientCmd( "lastinv" );
				}
			}
			return 0;

		case KEY_XBUTTON_B:
		case STEAMCONTROLLER_B:
			// cancel, close the menu
			engine->ExecuteClientCmd( "lastinv" );
			return 0;

		default:
			return 1;	// key not handled
		}

		SetSelectedItem( iNewSelection );

		return 0;
	}
	else
	{
		int iSlot = 0;

		// convert slot1, slot2 etc to 1,2,3,4
		if( pszCurrentBinding && ( !Q_strncmp( pszCurrentBinding, "slot", NUM_ENGY_BUILDINGS ) && Q_strlen(pszCurrentBinding) > NUM_ENGY_BUILDINGS ) )
		{
			const char *pszNum = pszCurrentBinding+NUM_ENGY_BUILDINGS;
			iSlot = atoi(pszNum);

			// slot10 cancels
			if ( iSlot == 10 )
			{
				engine->ExecuteClientCmd( "lastinv" );
				return 0;
			}

			// allow slot1 - slot4 
			if ( iSlot < 1 || iSlot > NUM_ENGY_BUILDINGS )
				return 1;
		}
		else
		{
			switch( keynum )
			{
			case KEY_1:
				iSlot = 1;
				break;
			case KEY_2:
				iSlot = 2;
				break;
			case KEY_3:
				iSlot = 3;
				break;
			case KEY_4:
				iSlot = 4;
				break;

			case KEY_5:
			case KEY_6:
			case KEY_7:
			case KEY_8:
			case KEY_9:
				// Eat these keys
				return 0;

			case KEY_0:
			case KEY_XBUTTON_B:
			case STEAMCONTROLLER_B:
				// cancel, close the menu
				engine->ExecuteClientCmd( "lastinv" );
				return 0;

			default:
				return 1;	// key not handled
			}
		}		

		if ( iSlot > 0 )
		{
			SendBuildMessage( iSlot );
			return 0;
		}
	}

	return 1;	// key not handled
}

void CHudMenuEngyBuild::SendBuildMessage( int iSlot )
{
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

	if ( !pLocalPlayer )
		return;

	int iBuilding, iMode;
	GetBuildingIDAndModeFromSlot( iSlot, iBuilding, iMode, m_Buildings );

	if ( CanBuild( iSlot ) == false )
	{
		return;
	}

	C_BaseObject *pObj = pLocalPlayer->GetObjectOfType( iBuilding, iMode );
	int iCost = pLocalPlayer->m_Shared.CalculateObjectCost( pLocalPlayer, iBuilding );

	int iBuildDisposableSents = CB_CANNOT_BUILD;
	if ( TFGameRules()->GameModeUsesUpgrades() && iBuilding == OBJ_SENTRYGUN )
	{
		iBuildDisposableSents = pLocalPlayer->CanBuild( iBuilding, iMode );
	}

	// If we don't already have a sentry (NULL), or we're allowed to build multiple, and we can afford it
	if ( ( pObj == NULL || iBuildDisposableSents == CB_CAN_BUILD ) && pLocalPlayer->GetAmmoCount( TF_AMMO_METAL ) >= iCost )
	{
		char szCmd[128];
		Q_snprintf( szCmd, sizeof(szCmd), "build %d %d", iBuilding, iMode );
		engine->ClientCmd( szCmd );
		
		// NVNT send the build command
		if ( haptics )
			haptics->ProcessHapticEvent(2, "Game", szCmd);

	}
	else
	{
		pLocalPlayer->EmitSound( "Player.DenyWeaponSelection" );
	}
}

bool CHudMenuEngyBuild::SendDestroyMessage( int iSlot )
{
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

	if ( !pLocalPlayer )
		return false;

	bool bSuccess = false;

	int iBuilding, iMode;
	GetBuildingIDAndModeFromSlot( iSlot, iBuilding, iMode, m_Buildings );

	C_BaseObject *pObj = pLocalPlayer->GetObjectOfType( iBuilding, iMode );

	if ( pObj != NULL )
	{
		char szCmd[128];
		Q_snprintf( szCmd, sizeof(szCmd), "destroy %d %d", iBuilding, iMode );
		engine->ClientCmd( szCmd );
		// NVNT send the destroy command
		if ( haptics )
			haptics->ProcessHapticEvent(2, "Game", szCmd);
		bSuccess = true; 
	}
	else
	{
		pLocalPlayer->EmitSound( "Player.DenyWeaponSelection" );
	}

	return bSuccess;
}

// NVNT gate for placing effect.

void CHudMenuEngyBuild::OnTick( void )
{
	if ( !IsVisible() )
		return;

	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

	if ( !pLocalPlayer )
		return;

	int iAccount = pLocalPlayer->GetAmmoCount( TF_AMMO_METAL );

	for ( int i=0;i<NUM_ENGY_BUILDINGS; i++ )
	{
		int iRemappedObjectID, iMode;
		GetBuildingIDAndModeFromSlot( i + 1, iRemappedObjectID, iMode, m_Buildings );

		// update this slot
		C_BaseObject *pObj = NULL;

		if ( pLocalPlayer )
		{
			pObj = pLocalPlayer->GetObjectOfType( iRemappedObjectID, iMode );
		}

		m_pAvailableObjects[i]->SetVisible( false );
		m_pAlreadyBuiltObjects[i]->SetVisible( false );
		m_pCantAffordObjects[i]->SetVisible( false );
		m_pUnavailableObjects[i]->SetVisible( false );

		if ( !m_Buildings[i].m_bEnabled )
		{
			continue;
		}

		int iCost = pLocalPlayer->m_Shared.CalculateObjectCost( pLocalPlayer, iRemappedObjectID );
		bool bAvailable = CanBuild( i + 1 );

		// If the building is already built, and we don't have an ability to build more than one (sentry)
		if ( pObj != NULL && !pObj->IsPlacing() && !( pLocalPlayer->CanBuild( iRemappedObjectID, iMode ) == CB_CAN_BUILD ) )
		{
			m_pAlreadyBuiltObjects[i]->SetVisible( true );
		}
		// unavailable
		else if ( bAvailable == false )
		{
			m_pUnavailableObjects[i]->SetVisible( true );
		}
		// See if we can afford it
		else if ( iAccount < iCost )
		{
			m_pCantAffordObjects[i]->SetVisible( true );
		}
		else
		{
			// we can buy it
			m_pAvailableObjects[i]->SetVisible( true );
		}
	}
}


void CHudMenuEngyBuild::SetVisible( bool state )
{
	if ( state == true )
	{
		InitBuildings();

		// close the weapon selection menu
		engine->ClientCmd( "cancelselect" );

		bool bConsoleMode = ( IsConsole() || tf_build_menu_controller_mode.GetBool() );

		if ( bConsoleMode != m_bInConsoleMode )
		{
			InvalidateLayout( true, true );
			m_bInConsoleMode = bConsoleMode;
		}
		else
		{
			// See if our layout needs to change, due to equipped items
			buildmenulayouts_t eDesired = CalcCustomBuildMenuLayout();
			if ( eDesired != m_eCurrentBuildMenuLayout )
			{
				m_eCurrentBuildMenuLayout = eDesired;
				InvalidateLayout( true, true );
			}
		}

		// set the %lastinv% dialog var to our binding
		const char *key = engine->Key_LookupBinding( "lastinv" );
		if ( !key )
		{
			key = "< not bound >";
		}

		SetDialogVariable( "lastinv", key );

		// Set selection to the first available building that we can build

		C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

		if ( !pLocalPlayer )
			return;

		int iDefaultSlot = 1;

		// Find the first slot that represents a building that we haven't built
		int iSlot;
		for ( iSlot = 1; iSlot <= NUM_ENGY_BUILDINGS; iSlot++ )
		{
			int iBuilding, iMode;
			GetBuildingIDAndModeFromSlot( iSlot, iBuilding, iMode, m_Buildings );
			C_BaseObject *pObj = pLocalPlayer->GetObjectOfType( iBuilding, iMode );

			if ( pObj == NULL )
			{
				iDefaultSlot = iSlot;
				break;
			}
		}

		m_iSelectedItem = -1;	//force redo
		SetSelectedItem( iDefaultSlot );

		HideLowerPriorityHudElementsInGroup( "mid" );

		for ( int i=0; i<NUM_ENGY_BUILDINGS; i++ )
		{
			int iBuilding, iMode;
			GetBuildingIDAndModeFromSlot( i+1, iBuilding, iMode, m_Buildings );
			int iCost = pLocalPlayer->m_Shared.CalculateObjectCost( pLocalPlayer, iBuilding );
			m_pAvailableObjects[i]->SetDialogVariable( "metal", iCost );
			m_pAlreadyBuiltObjects[i]->SetDialogVariable( "metal", iCost );
			m_pCantAffordObjects[i]->SetDialogVariable( "metal", iCost );
		}
	}
	else
	{
		UnhideLowerPriorityHudElementsInGroup( "mid" );
	}

	BaseClass::SetVisible( state );
}

void CHudMenuEngyBuild::SetSelectedItem( int iSlot )
{
	if ( m_iSelectedItem != iSlot )
	{
		m_iSelectedItem = iSlot;

		// move the selection item to the new position
		if ( m_pActiveSelection )
		{
			// move the selection background
			int x, y;
			m_pAlreadyBuiltObjects[m_iSelectedItem-1]->GetPos( x, y );

			x -= XRES(NUM_ENGY_BUILDINGS);
			y -= XRES(NUM_ENGY_BUILDINGS);

			m_pActiveSelection->SetPos( x, y );

			UpdateHintLabels();			
		}
	}
}

void CHudMenuEngyBuild::UpdateHintLabels( void )
{
	// hilight the action we can perform ( build or destroy or neither )
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

	if ( pLocalPlayer )
	{
		int iBuilding, iMode;
		GetBuildingIDAndModeFromSlot( m_iSelectedItem, iBuilding, iMode, m_Buildings );
		C_BaseObject *pObj = pLocalPlayer->GetObjectOfType( iBuilding );

		bool bDestroyLabelBright = false;
		bool bBuildLabelBright = false;

		int iCost = pLocalPlayer->m_Shared.CalculateObjectCost( pLocalPlayer, iBuilding );

		if ( pObj )
		{
			// hilight destroy, we have a building
			bDestroyLabelBright = true;
		}
		else if ( pLocalPlayer->GetAmmoCount( TF_AMMO_METAL ) >= iCost )	// I can afford it
		{
			// hilight build, we can build this
			bBuildLabelBright = true;
		}
		else
		{
			// dim both, do nothing
		}

		if ( m_pDestroyLabelBright && m_pDestroyLabelDim && m_pBuildLabelBright && m_pBuildLabelDim )
		{
			m_pDestroyLabelBright->SetVisible( bDestroyLabelBright );
			m_pDestroyLabelDim->SetVisible( !bDestroyLabelBright );

			m_pBuildLabelBright->SetVisible( bBuildLabelBright );
			m_pBuildLabelDim->SetVisible( !bBuildLabelBright );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
buildmenulayouts_t CHudMenuEngyBuild::CalcCustomBuildMenuLayout( void )
{
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pLocalPlayer )
		return BUILDMENU_DEFAULT;

	int iMenu = BUILDMENU_DEFAULT;
	CALL_ATTRIB_HOOK_INT_ON_OTHER( pLocalPlayer, iMenu, set_custom_buildmenu );
	return (buildmenulayouts_t)iMenu;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudMenuEngyBuild::InitBuildings()
{
	for( int i=0; i<NUM_ENGY_BUILDINGS; ++i )
	{
		m_Buildings[i] = g_kEngyBuildings[i];
	}

	ReplaceBuildings( m_Buildings );
	
	InvalidateLayout( true, true );
}


void CHudMenuEngyBuild::ReplaceBuildings( EngyConstructBuilding_t (&targetBuildings)[NUM_ENGY_BUILDINGS] )
{
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pLocalPlayer )
		return;

	CUtlVector< const EngyBuildingReplacement_t* > vecReplacements;


	// verify the override data to make sure that they don't conflict with each other
	int iReplacedSlots = 0;
	int iDisabledSlots = 0;
	bool bReplaced = false;
	for ( int i=0; i<vecReplacements.Count(); ++i )
	{
		const EngyBuildingReplacement_t* pReplace = vecReplacements[i];
		int iReplacingSlots = pReplace->m_iReplacementSlots;
		int iDisablingSlots = pReplace->m_iDisableSlots;

		if ( iReplacedSlots & iReplacingSlots )
		{
			AssertMsg( 0, "Trying to replace the same engineer building slot multiple time" );
			continue;
		}

		if ( iReplacedSlots & iDisablingSlots )
		{
			AssertMsg( 0, "Trying to disable a replaced engineer building slot" );
			continue;
		}

		if ( iDisabledSlots & iReplacingSlots )
		{
			AssertMsg( 0, "Trying to replace a disabled slot" );
			continue;
		}
		
		// no conflict, replace the building
		for ( int j=0; j<ARRAYSIZE( flagSlots ); ++j )
		{
			COMPILE_TIME_ASSERT( ARRAYSIZE( targetBuildings ) == ARRAYSIZE( flagSlots ) );
			if ( flagSlots[j] & iReplacingSlots )
			{
				targetBuildings[j] = pReplace->m_building;
			}
			else if ( flagSlots[j] & iDisablingSlots )
			{
				targetBuildings[j].m_bEnabled = false;
			}
		}

		iReplacedSlots |= iReplacingSlots;
		iDisabledSlots |= iDisablingSlots;
		bReplaced = true;
	}
}


bool CHudMenuEngyBuild::CanBuild( int iSlot )
{
	bool bInTraining = TFGameRules() && TFGameRules()->IsInTraining();
	if ( bInTraining == false )
	{
		int slot = iSlot - 1;
		if ( slot >= 0 && slot < NUM_ENGY_BUILDINGS )
		{
			return m_Buildings[slot].m_bEnabled;
		}

		return false;
	}

	bool bCanBuild = true;
	switch ( iSlot )
	{
	case 1:
		{
			ConVarRef training_can_build_sentry( "training_can_build_sentry");
			bCanBuild = training_can_build_sentry.GetInt() != 0;
		}			
		break;
	case 2:
		{
			ConVarRef training_can_build_dispenser( "training_can_build_dispenser");
			bCanBuild = training_can_build_dispenser.GetInt() != 0;
		}
		break;
	case 3:
		{
			ConVarRef training_can_build_tele_entrance( "training_can_build_tele_entrance");
			bCanBuild = training_can_build_tele_entrance.GetInt() != 0;
		}
		break;
	case 4:
		{
			ConVarRef training_can_build_tele_exit( "training_can_build_tele_exit");
			bCanBuild = training_can_build_tele_exit.GetInt() != 0;
		}
		break;
	}
	return bCanBuild;
}