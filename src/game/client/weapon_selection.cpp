//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Weapon selection handling
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "weapon_selection.h"
#include "hud_macros.h"
#include "history_resource.h"
#include "menu.h"
#include "in_buttons.h"
#include <KeyValues.h>
#include "filesystem.h"
#include "iinput.h"
#include "inputsystem/iinputsystem.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define HISTORY_DRAW_TIME	"5"

ConVar hud_drawhistory_time( "hud_drawhistory_time", HISTORY_DRAW_TIME, 0 );
ConVar hud_fastswitch( "hud_fastswitch", "0", FCVAR_ARCHIVE | FCVAR_ARCHIVE_XBOX );

//-----------------------------------------------------------------------------
// Purpose: Weapon Selection commands
//-----------------------------------------------------------------------------
DECLARE_HUD_COMMAND_NAME(CBaseHudWeaponSelection, Slot1, "CHudWeaponSelection");
DECLARE_HUD_COMMAND_NAME(CBaseHudWeaponSelection, Slot2, "CHudWeaponSelection");
DECLARE_HUD_COMMAND_NAME(CBaseHudWeaponSelection, Slot3, "CHudWeaponSelection");
DECLARE_HUD_COMMAND_NAME(CBaseHudWeaponSelection, Slot4, "CHudWeaponSelection");
DECLARE_HUD_COMMAND_NAME(CBaseHudWeaponSelection, Slot5, "CHudWeaponSelection");
DECLARE_HUD_COMMAND_NAME(CBaseHudWeaponSelection, Slot6, "CHudWeaponSelection");
DECLARE_HUD_COMMAND_NAME(CBaseHudWeaponSelection, Slot7, "CHudWeaponSelection");
DECLARE_HUD_COMMAND_NAME(CBaseHudWeaponSelection, Slot8, "CHudWeaponSelection");
DECLARE_HUD_COMMAND_NAME(CBaseHudWeaponSelection, Slot9, "CHudWeaponSelection");
DECLARE_HUD_COMMAND_NAME(CBaseHudWeaponSelection, Slot0, "CHudWeaponSelection");
DECLARE_HUD_COMMAND_NAME(CBaseHudWeaponSelection, Slot10, "CHudWeaponSelection");
DECLARE_HUD_COMMAND_NAME(CBaseHudWeaponSelection, Close, "CHudWeaponSelection");
DECLARE_HUD_COMMAND_NAME(CBaseHudWeaponSelection, NextWeapon, "CHudWeaponSelection");
DECLARE_HUD_COMMAND_NAME(CBaseHudWeaponSelection, PrevWeapon, "CHudWeaponSelection");
DECLARE_HUD_COMMAND_NAME(CBaseHudWeaponSelection, LastWeapon, "CHudWeaponSelection");

HOOK_COMMAND( slot1, Slot1 );
HOOK_COMMAND( slot2, Slot2 );
HOOK_COMMAND( slot3, Slot3 );
HOOK_COMMAND( slot4, Slot4 );
HOOK_COMMAND( slot5, Slot5 );
HOOK_COMMAND( slot6, Slot6 );
HOOK_COMMAND( slot7, Slot7 );
HOOK_COMMAND( slot8, Slot8 );
HOOK_COMMAND( slot9, Slot9 );
HOOK_COMMAND( slot0, Slot0 );
HOOK_COMMAND( slot10, Slot10 );
HOOK_COMMAND( cancelselect, Close );
HOOK_COMMAND( invnext, NextWeapon );
HOOK_COMMAND( invprev, PrevWeapon );
HOOK_COMMAND( lastinv, LastWeapon );

// instance info
CBaseHudWeaponSelection *CBaseHudWeaponSelection::s_pInstance = NULL;
CBaseHudWeaponSelection *CBaseHudWeaponSelection::GetInstance()
{
	return s_pInstance;
}
CBaseHudWeaponSelection *GetHudWeaponSelection()
{
	return CBaseHudWeaponSelection::GetInstance();
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CBaseHudWeaponSelection::CBaseHudWeaponSelection( const char *pElementName ) : CHudElement( pElementName )
{
	s_pInstance = this;
	
	SetHiddenBits( HIDEHUD_WEAPONSELECTION | HIDEHUD_NEEDSUIT | HIDEHUD_PLAYERDEAD | HIDEHUD_INVEHICLE );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseHudWeaponSelection::Init(void)
{
	Reset();

	// Initialise the weapons resource
	gWR.Init();

	m_flSelectionTime = gpGlobals->curtime;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseHudWeaponSelection::Reset(void)
{
	gWR.Reset();

	// Start hidden
	m_bSelectionVisible = false;
	m_flSelectionTime = gpGlobals->curtime;
	gHUD.UnlockRenderGroup( gHUD.LookupRenderGroupIndexByName( "weapon_selection" ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseHudWeaponSelection::UpdateSelectionTime( void )
{
	m_flSelectionTime = gpGlobals->curtime;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseHudWeaponSelection::VidInit(void)
{
	// If we've already loaded weapons, let's get new sprites
	gWR.LoadAllWeaponSprites();

	// set spacing of pickup history
	CHudHistoryResource *pHudHR = GET_HUDELEMENT( CHudHistoryResource );
	if( pHudHR )
	{
		pHudHR->SetHistoryGap( 21 );
	}

	Reset();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseHudWeaponSelection::OnThink( void )
{
	// Don't allow weapon selection if we're frozen in place
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( pPlayer->GetFlags() & FL_FROZEN || pPlayer->IsPlayerDead() )
	{
		if ( IsInSelectionMode() )
		{
			CancelWeaponSelection();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Think used for selection of weapon menu item.
//-----------------------------------------------------------------------------
void CBaseHudWeaponSelection::ProcessInput()
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return;

	int nFastswitchMode = hud_fastswitch.GetInt();
	if ( ::input->IsSteamControllerActive() )
	{
		nFastswitchMode = HUDTYPE_FASTSWITCH;
	}

	// Check to see if the player is in VGUI mode...
	if ( pPlayer->IsInVGuiInputMode() && !pPlayer->IsInViewModelVGuiInputMode() )
	{
		// If so, close weapon selection when they press fire
		if ( gHUD.m_iKeyBits & IN_ATTACK )
		{
			if ( HUDTYPE_PLUS != nFastswitchMode )
			{
				// Swallow the button
				gHUD.m_iKeyBits &= ~IN_ATTACK;
				input->ClearInputButton( IN_ATTACK );
			}

			engine->ClientCmd( "cancelselect\n" );
		}
		return;
	}

	// Has the player selected a weapon?
	if ( gHUD.m_iKeyBits & (IN_ATTACK | IN_ATTACK2) )
	{
		if ( IsWeaponSelectable() )
		{
#ifndef TF_CLIENT_DLL
			if ( HUDTYPE_PLUS != nFastswitchMode )
#endif
			{
				// Swallow the button
				gHUD.m_iKeyBits &= ~(IN_ATTACK | IN_ATTACK2);
				input->ClearInputButton( IN_ATTACK );
				input->ClearInputButton( IN_ATTACK2 );
			}

			// select weapon
			SelectWeapon();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CBaseHudWeaponSelection::IsInSelectionMode()
{
	return m_bSelectionVisible;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseHudWeaponSelection::OpenSelection( void )
{
	m_bSelectionVisible = true;
	gHUD.LockRenderGroup( gHUD.LookupRenderGroupIndexByName( "weapon_selection" ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseHudWeaponSelection::HideSelection( void )
{
	m_bSelectionVisible = false;
	gHUD.UnlockRenderGroup( gHUD.LookupRenderGroupIndexByName( "weapon_selection" ) );
}

//-----------------------------------------------------------------------------
// Purpose: Returns whether a weapon can be selected in the HUD, based on hud type
//-----------------------------------------------------------------------------
bool CBaseHudWeaponSelection::CanBeSelectedInHUD( C_BaseCombatWeapon *pWeapon )
{
	int nFastswitchMode = hud_fastswitch.GetInt();
	if ( ::input->IsSteamControllerActive() )
	{
		nFastswitchMode = HUDTYPE_FASTSWITCH;
	}

	// Xbox: In plus type, weapons without ammo can still be selected in the HUD
	if( HUDTYPE_PLUS == nFastswitchMode )
	{
		return pWeapon->VisibleInWeaponSelection();
	}

	if ( !pWeapon->VisibleInWeaponSelection() )
	{
		return false;
	}

	// All other current hud types
	return pWeapon->CanBeSelected();
}

//-----------------------------------------------------------------------------
// Purpose: handles keyboard input
//-----------------------------------------------------------------------------
int	CBaseHudWeaponSelection::KeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding ) 
{
	if (IsInSelectionMode() && pszCurrentBinding && !stricmp(pszCurrentBinding, "cancelselect"))
	{
		HideSelection();
		// returning 0 indicates, we've handled it, no more action needs to be taken
		return 0;
	}

	if ( down >= 1 && keynum >= KEY_1 && keynum <= KEY_9 )
	{
		if ( HandleHudMenuInput( keynum - KEY_0 ) )
			return 0;
	}

	// let someone else handle it
	return 1;
}


//-----------------------------------------------------------------------------
// Purpose: called when a weapon has been picked up
//-----------------------------------------------------------------------------
void CBaseHudWeaponSelection::OnWeaponPickup( C_BaseCombatWeapon *pWeapon )
{
	// add to pickup history
	CHudHistoryResource *pHudHR = GET_HUDELEMENT( CHudHistoryResource );
	
	if ( pHudHR )
	{
		pHudHR->AddToHistory( pWeapon );
	}
}

//------------------------------------------------------------------------
// Command Handlers
//------------------------------------------------------------------------
void CBaseHudWeaponSelection::UserCmd_Slot1(void)
{
	int nFastswitchMode = hud_fastswitch.GetInt();
	if ( ::input->IsSteamControllerActive() )
	{
		nFastswitchMode = HUDTYPE_FASTSWITCH;
	}

	if( HUDTYPE_CAROUSEL == nFastswitchMode )
	{
		UserCmd_LastWeapon();
	}
	else
	{
		SelectSlot( 1 );
	}
}

void CBaseHudWeaponSelection::UserCmd_Slot2(void)
{
	int nFastswitchMode = hud_fastswitch.GetInt();
	if ( ::input->IsSteamControllerActive() )
	{
		nFastswitchMode = HUDTYPE_FASTSWITCH;
	}

	if( HUDTYPE_CAROUSEL == nFastswitchMode )
	{
		UserCmd_NextWeapon();
	}
	else
	{
		SelectSlot( 2 );
	}
}

void CBaseHudWeaponSelection::UserCmd_Slot3(void)
{
	int nFastswitchMode = hud_fastswitch.GetInt();
	if ( ::input->IsSteamControllerActive() )
	{
		nFastswitchMode = HUDTYPE_FASTSWITCH;
	}

	if( HUDTYPE_CAROUSEL == nFastswitchMode )
	{
		engine->ClientCmd( "phys_swap" );
	}
	else
	{
		SelectSlot( 3 );
	}
}

void CBaseHudWeaponSelection::UserCmd_Slot4(void)
{
	int nFastswitchMode = hud_fastswitch.GetInt();
	if ( ::input->IsSteamControllerActive() )
	{
		nFastswitchMode = HUDTYPE_FASTSWITCH;
	}

	if( HUDTYPE_CAROUSEL == nFastswitchMode )
	{
		UserCmd_PrevWeapon();
	}
	else
	{
		SelectSlot( 4 );
	}
}

void CBaseHudWeaponSelection::UserCmd_Slot5(void)
{
	SelectSlot( 5 );
}

void CBaseHudWeaponSelection::UserCmd_Slot6(void)
{
	SelectSlot( 6 );
}

void CBaseHudWeaponSelection::UserCmd_Slot7(void)
{
	SelectSlot( 7 );
}

void CBaseHudWeaponSelection::UserCmd_Slot8(void)
{
	SelectSlot( 8 );
}

void CBaseHudWeaponSelection::UserCmd_Slot9(void)
{
	SelectSlot( 9 );
}

void CBaseHudWeaponSelection::UserCmd_Slot0(void)
{
	SelectSlot( 0 );
}

void CBaseHudWeaponSelection::UserCmd_Slot10(void)
{
	SelectSlot( 10 );
}

//-----------------------------------------------------------------------------
// Purpose: returns true if the CHudMenu should take slot1, etc commands
//-----------------------------------------------------------------------------
bool CBaseHudWeaponSelection::IsHudMenuTakingInput()
{
	CHudMenu *pHudMenu = GET_HUDELEMENT( CHudMenu );
	return ( pHudMenu && pHudMenu->IsMenuOpen() );
}

//-----------------------------------------------------------------------------
// Purpose: returns true if the CHudMenu handles the slot command
//-----------------------------------------------------------------------------
bool CBaseHudWeaponSelection::HandleHudMenuInput( int iSlot )
{
	CHudMenu *pHudMenu = GET_HUDELEMENT( CHudMenu );
	if ( !pHudMenu || !pHudMenu->IsMenuOpen() )
		return false;

	pHudMenu->SelectMenuItem( iSlot );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: returns true if the weapon selection hud should be hidden because
//          the CHudMenu is open
//-----------------------------------------------------------------------------
bool CBaseHudWeaponSelection::IsHudMenuPreventingWeaponSelection()
{
	// Don't allow weapon selection if we're frozen in place
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( pPlayer->GetFlags() & FL_FROZEN || pPlayer->IsPlayerDead() )
		return true;

	return IsHudMenuTakingInput();
}

//-----------------------------------------------------------------------------
// Purpose: Menu Selection Code
//-----------------------------------------------------------------------------
void CBaseHudWeaponSelection::SelectSlot( int iSlot )
{
	// A menu may be overriding weapon selection commands
	if ( HandleHudMenuInput( iSlot ) )
	{
		return;
	}

	// If we're not allowed to draw, ignore weapon selections
	if ( !BaseClass::ShouldDraw() )
	{
		return;
	}

	UpdateSelectionTime();
	SelectWeaponSlot( iSlot );
}

//-----------------------------------------------------------------------------
// Purpose: Close the weapon selection
//-----------------------------------------------------------------------------
void CBaseHudWeaponSelection::UserCmd_Close(void)
{
	CancelWeaponSelection();
}

//-----------------------------------------------------------------------------
// Purpose: Selects the next item in the weapon menu
//-----------------------------------------------------------------------------
void CBaseHudWeaponSelection::UserCmd_NextWeapon(void)
{
	// If we're not allowed to draw, ignore weapon selections
	if ( !BaseClass::ShouldDraw() )
		return;

	int nFastswitchMode = hud_fastswitch.GetInt();
	if ( ::input->IsSteamControllerActive() )
	{
		nFastswitchMode = HUDTYPE_FASTSWITCH;
	}

	CycleToNextWeapon();
	if( nFastswitchMode > 0 )
	{
		SelectWeapon();
	}
	UpdateSelectionTime();
}

//-----------------------------------------------------------------------------
// Purpose: Selects the previous item in the menu
//-----------------------------------------------------------------------------
void CBaseHudWeaponSelection::UserCmd_PrevWeapon(void)
{
	// If we're not allowed to draw, ignore weapon selections
	if ( !BaseClass::ShouldDraw() )
		return;

	int nFastswitchMode = hud_fastswitch.GetInt();
	if ( ::input->IsSteamControllerActive() )
	{
		nFastswitchMode = HUDTYPE_FASTSWITCH;
	}

	CycleToPrevWeapon();
	if( nFastswitchMode > 0 )
	{
		SelectWeapon();
	}

	UpdateSelectionTime();
}

//-----------------------------------------------------------------------------
// Purpose: Switches the last weapon the player was using
//-----------------------------------------------------------------------------
void CBaseHudWeaponSelection::UserCmd_LastWeapon(void)
{
	// If we're not allowed to draw, ignore weapon selections
	if ( !BaseClass::ShouldDraw() )
		return;

	/*
	if ( IsHudMenuPreventingWeaponSelection() )	
	{ 
		return;
	}
	*/

	SwitchToLastWeapon();
}

//-----------------------------------------------------------------------------
// Purpose: Switches the last weapon the player was using
//-----------------------------------------------------------------------------
void CBaseHudWeaponSelection::SwitchToLastWeapon( void )
{
	// Get the player's last weapon
	C_BasePlayer *player = C_BasePlayer::GetLocalPlayer();
	if ( !player )
		return;

	input->MakeWeaponSelection( player->GetLastWeapon() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseHudWeaponSelection::SetWeaponSelected( void )
{
	Assert( GetSelectedWeapon() );
	// Mark selection so that it's placed into next CUserCmd created
	input->MakeWeaponSelection( GetSelectedWeapon() );
}


//-----------------------------------------------------------------------------
// Purpose: Player has chosen to draw the currently selected weapon
//-----------------------------------------------------------------------------
void CBaseHudWeaponSelection::SelectWeapon( void )
{
	if ( !GetSelectedWeapon() )
	{
		engine->ClientCmd( "cancelselect\n" );
		return;
	}

	C_BasePlayer *player = C_BasePlayer::GetLocalPlayer();
	if ( !player )
		return;

	// Don't allow selections of weapons that can't be selected (out of ammo, etc)
	if ( !GetSelectedWeapon()->CanBeSelected() )
	{
		player->EmitSound( "Player.DenyWeaponSelection" );
	}
	else
	{
		SetWeaponSelected();
	
		m_hSelectedWeapon = NULL;
	
		engine->ClientCmd( "cancelselect\n" );

		// Play the "weapon selected" sound
		player->EmitSound( "Player.WeaponSelected" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Abort selecting a weapon
//-----------------------------------------------------------------------------
void CBaseHudWeaponSelection::CancelWeaponSelection( void )
{
	C_BasePlayer *player = C_BasePlayer::GetLocalPlayer();
	if ( !player )
		return;

	// Fastswitches happen in a single frame, so the Weapon Selection HUD Element isn't visible
	// yet, but it's going to be next frame. We need to ask it if it thinks it's going to draw,
	// instead of checking it's IsActive flag.
	if ( ShouldDraw() )
	{
		HideSelection();

		m_hSelectedWeapon = NULL;

		// Play the "close weapon selection" sound
		player->EmitSound( "Player.WeaponSelectionClose" );
	}
	else
	{
		engine->ClientCmd("escape");
	}
}

//-----------------------------------------------------------------------------
// Purpose: Returns the first weapon for a given slot.
//-----------------------------------------------------------------------------
C_BaseCombatWeapon *CBaseHudWeaponSelection::GetFirstPos( int iSlot )
{
	int iLowestPosition = MAX_WEAPON_POSITIONS;
	C_BaseCombatWeapon *pFirstWeapon = NULL;

	C_BasePlayer *player = C_BasePlayer::GetLocalPlayer();
	if ( !player )
		return NULL;

	for ( int i = 0; i < MAX_WEAPONS; i++ )
	{
		C_BaseCombatWeapon *pWeapon = player->GetWeapon( i );
		if ( !pWeapon )
			continue;

		if ( ( pWeapon->GetSlot() == iSlot ) && (pWeapon->VisibleInWeaponSelection()) )
		{
			// If this weapon is lower in the slot than the current lowest, it's our new winner
			if ( pWeapon->GetPosition() <= iLowestPosition )
			{
				iLowestPosition = pWeapon->GetPosition();
				pFirstWeapon = pWeapon;
			}
		}
	}

	return pFirstWeapon;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_BaseCombatWeapon *CBaseHudWeaponSelection::GetNextActivePos( int iSlot, int iSlotPos )
{
	if ( iSlotPos >= MAX_WEAPON_POSITIONS || iSlot >= MAX_WEAPON_SLOTS )
		return NULL;

	int iLowestPosition = MAX_WEAPON_POSITIONS;
	C_BaseCombatWeapon *pNextWeapon = NULL;

	C_BasePlayer *player = C_BasePlayer::GetLocalPlayer();
	if ( !player )
		return NULL;
	for ( int i = 0; i < MAX_WEAPONS; i++ )
	{
		C_BaseCombatWeapon *pWeapon = player->GetWeapon( i );
		if ( !pWeapon )
			continue;

		if ( CanBeSelectedInHUD( pWeapon ) && pWeapon->GetSlot() == iSlot )
		{
			// If this weapon is lower in the slot than the current lowest, and above our desired position, it's our new winner
			if ( pWeapon->GetPosition() <= iLowestPosition && pWeapon->GetPosition() >= iSlotPos )
			{
				iLowestPosition = pWeapon->GetPosition();
				pNextWeapon = pWeapon;
			}
		}
	}

	return pNextWeapon;
}
