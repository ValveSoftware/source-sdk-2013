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
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <vgui/IVGui.h>
#include "c_baseobject.h"
#include "inputsystem/iinputsystem.h"

#include "tf_hud_menu_eureka_teleport.h"

// NVNT haptics for buildings
#include "haptics/haptic_utils.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

// Set to 1 to simulate xbox-style menu interaction
extern ConVar tf_build_menu_controller_mode;


DECLARE_HUDELEMENT_DEPTH( CHudEurekaEffectTeleportMenu, 41 );	// in front of engy building status
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudEurekaEffectTeleportMenu::CHudEurekaEffectTeleportMenu( const char *pElementName )
	: CHudElement( pElementName )
	, BaseClass( NULL, "HudEurekaEffectTeleportMenu" )
	, m_bWantsToTeleport( false )
	, m_eSelectedTeleportTarget( EUREKA_TELEPORT_HOME )
	, m_eCurrentBuildMenuLayout( BUILDMENU_DEFAULT )
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	SetHiddenBits( HIDEHUD_MISCSTATUS );

	vgui::ivgui()->AddTickSignal( GetVPanel() );

	for( int i=0; i < EUREKA_NUM_TARGETS; ++i )
	{
		m_pAvilableTargets[ i ] = new EditablePanel( this, VarArgs( "available_target_%d", i+1 ) );
		m_pUnavailableTargets[ i ] = new EditablePanel( this, VarArgs( "unavailable_target_%d", i+1 ) );
	}

	RegisterForRenderGroup( "mid" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudEurekaEffectTeleportMenu::ApplySchemeSettings( IScheme *pScheme )
{
	const char *pszCustomDir = NULL;
	switch( m_eCurrentBuildMenuLayout )
	{
	case BUILDMENU_PIPBOY:
		pszCustomDir = "resource/UI/build_menu/pipboy";
		break;

	default:
	case BUILDMENU_DEFAULT:
		if ( ::input->IsSteamControllerActive() )
		{
			pszCustomDir = "resource/UI/build_menu_sc";
		}
		else
		{
			pszCustomDir = "resource/UI/build_menu";	
		}
		break;
	}

	LoadControlSettings( VarArgs( "%s/HudMenuEurekaEffect.res", pszCustomDir) );

	m_pAvilableTargets[ EUREKA_TELEPORT_HOME ]->LoadControlSettings( VarArgs( "%s/eureka_target_home_avail.res", pszCustomDir ) );
	m_pAvilableTargets[ EUREKA_TELEPORT_TELEPORTER_EXIT ]->LoadControlSettings( VarArgs( "%s/eureka_target_tele_exit_avail.res", pszCustomDir ) );

	m_pUnavailableTargets[ EUREKA_TELEPORT_HOME ]->LoadControlSettings( VarArgs( "%s/eureka_target_home_unavail.res", pszCustomDir ) );
	m_pUnavailableTargets[ EUREKA_TELEPORT_TELEPORTER_EXIT ]->LoadControlSettings( VarArgs( "%s/eureka_target_tele_exit_unavail.res", pszCustomDir ) );

	BaseClass::ApplySchemeSettings( pScheme );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHudEurekaEffectTeleportMenu::ShouldDraw( void )
{
	if ( !CanTeleport() )
	{
		m_bWantsToTeleport = false;
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudEurekaEffectTeleportMenu::SetVisible( bool bState )
{
	if ( bState == true )
	{
		// close the weapon selection menu
		engine->ClientCmd( "cancelselect" );

		// See if our layout needs to change, due to equipped items
		buildmenulayouts_t eDesired = CHudMenuEngyBuild::CalcCustomBuildMenuLayout();
		if ( eDesired != m_eCurrentBuildMenuLayout )
		{
			m_eCurrentBuildMenuLayout = eDesired;
			InvalidateLayout( true, true );
		}
		
		const char* key = engine->Key_LookupBinding( "lastinv" );
		if ( !key )
		{
			key = "< not bound >";
		}

		SetDialogVariable( "lastinv", key );

		// Set selection to the first available building that we can build

		C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

		if ( !pLocalPlayer )
			return;

		m_eSelectedTeleportTarget = EUREKA_TELEPORT_HOME;
		SetSelectedItem( m_eSelectedTeleportTarget );

		HideLowerPriorityHudElementsInGroup( "mid" );
	}
	else
	{
		UnhideLowerPriorityHudElementsInGroup( "mid" );
	}

	BaseClass::SetVisible( bState );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHudEurekaEffectTeleportMenu::CanTeleport() const
{
	if ( !m_bWantsToTeleport )
		return false;

	CTFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pPlayer )
		return false;

	CTFWeaponBase *pWpn = pPlayer->GetActiveTFWeapon();

	if ( !pWpn )
		return false;

	// Don't show the menu for first person spectator
	if ( pPlayer != pWpn->GetOwner() )
		return false;

	if ( !const_cast<CHudEurekaEffectTeleportMenu*>(this)->CHudElement::ShouldDraw() )
		return false;

	return ( pWpn->GetWeaponID() == TF_WEAPON_WRENCH );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudEurekaEffectTeleportMenu::SendTeleportMessage( eEurekaTeleportTargets eTeleportTarget )
{
	// They've made their selection.  Setting this to false will close this panel.
	m_bWantsToTeleport = false;

	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

	if ( !pLocalPlayer )
		return;

	// We do code elsewhere that set the available targets panel visible if they're available.
	// If it's not visible, it's not available.

	// Can not use while karting
	if ( !m_pAvilableTargets[ eTeleportTarget ]->IsVisible() || pLocalPlayer->m_Shared.InCond( TF_COND_HALLOWEEN_KART ) )
	{
		pLocalPlayer->EmitSound( "Player.DenyWeaponSelection" );
		return;
	}

	char szCmd[128];
	Q_snprintf( szCmd, sizeof(szCmd), "eureka_teleport %d", (int)eTeleportTarget );
	engine->ClientCmd( szCmd );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CHudEurekaEffectTeleportMenu::HudElementKeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding )
{
	if ( !ShouldDraw() )
	{
		return 1;
	}

	if ( !down )
	{
		return 1;
	}

	bool bConsoleController = ( IsConsole() || ( keynum >= JOYSTICK_FIRST && !IsSteamControllerCode(keynum)) );

	if ( bConsoleController )
	{
		int nNewSelection = m_eSelectedTeleportTarget;

		switch( keynum )
		{
		case KEY_XBUTTON_UP:
			// jump to last
			nNewSelection = EUREKA_LAST_TARGET;
			break;

		case KEY_XBUTTON_DOWN:
			// jump to first
			nNewSelection = EUREKA_FIRST_TARGET;
			break;

		case KEY_XBUTTON_RIGHT:
			// move selection to the right
			nNewSelection++;
			if ( nNewSelection > EUREKA_LAST_TARGET )
				nNewSelection = EUREKA_FIRST_TARGET;
			break;

		case KEY_XBUTTON_LEFT:
			// move selection to the left
			nNewSelection--;
			if ( nNewSelection <= 0 )
				nNewSelection = EUREKA_LAST_TARGET;
			break;

		case KEY_XBUTTON_A:
		case KEY_XBUTTON_RTRIGGER:
		case KEY_XBUTTON_Y:
		case KEY_XBUTTON_LTRIGGER:
			// build selected item
			SendTeleportMessage( (eEurekaTeleportTargets)nNewSelection );
			return 0;

		case KEY_XBUTTON_B:
			// cancel, close the menu
			engine->ExecuteClientCmd( "lastinv" );
			return 0;

		default:
			return 1;	// key not handled
		}

		SetSelectedItem( (eEurekaTeleportTargets)nNewSelection );

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
			if ( iSlot < 1 || iSlot > EUREKA_NUM_TARGETS )
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
			case KEY_4:
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

			case STEAMCONTROLLER_DPAD_LEFT:
				SendTeleportMessage( EUREKA_TELEPORT_HOME );
				return 0;

			case STEAMCONTROLLER_DPAD_RIGHT:
				SendTeleportMessage( EUREKA_TELEPORT_TELEPORTER_EXIT );
				return 0;

			default:
				return 1;	// key not handled
			}
		}		

		if ( iSlot > 0 )
		{
			SendTeleportMessage( (eEurekaTeleportTargets)(iSlot-1) );
			return 0;
		}
	}

	return 1;	// key not handled
}


void CHudEurekaEffectTeleportMenu::WantsToTeleport()
{
	m_bWantsToTeleport = true;
	InvalidateLayout( true, true );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudEurekaEffectTeleportMenu::SetSelectedItem( eEurekaTeleportTargets eSelectedTeleportTarget )
{
	if ( m_eSelectedTeleportTarget != eSelectedTeleportTarget )
	{
		m_eSelectedTeleportTarget = eSelectedTeleportTarget;

		// move the selection item to the new position
		if ( m_pActiveSelection )
		{
			// move the selection background
			int x, y;
			m_pAvilableTargets[m_eSelectedTeleportTarget]->GetPos( x, y );

			x -= XRES(4);
			y -= XRES(4);

			m_pActiveSelection->SetPos( x, y );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudEurekaEffectTeleportMenu::OnTick( void )
{
	if ( !IsVisible() )
		return;

	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

	if ( !pLocalPlayer )
		return;

	m_pAvilableTargets[ EUREKA_TELEPORT_HOME ]->SetVisible( true );
	m_pUnavailableTargets[ EUREKA_TELEPORT_HOME ]->SetVisible( false );

	const C_BaseObject *pObj = pLocalPlayer->GetObjectOfType( OBJ_TELEPORTER, MODE_TELEPORTER_EXIT );
	bool bTeleAvailable = pObj 
					    && !pObj->IsBuilding()
					    && !pObj->IsPlacing()
					    && !pObj->IsUpgrading()
					    && !pObj->IsCarried();

	m_pAvilableTargets[ EUREKA_TELEPORT_TELEPORTER_EXIT ]->SetVisible( bTeleAvailable );
	m_pUnavailableTargets[ EUREKA_TELEPORT_TELEPORTER_EXIT ]->SetVisible( !bTeleAvailable );
}

//----------------------------------------------------------------------------