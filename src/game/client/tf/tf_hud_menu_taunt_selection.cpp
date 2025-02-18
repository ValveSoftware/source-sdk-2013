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
#include "c_baseobject.h"
#include "IGameUIFuncs.h" // for key bindings
#include "inputsystem/iinputsystem.h"

#ifdef SIXENSE
#include "sixense/in_sixense.h"
#endif

#include "tf_hud_menu_taunt_selection.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

//======================================

DECLARE_HUDELEMENT( CHudMenuTauntSelection );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudMenuTauntSelection::CHudMenuTauntSelection( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudMenuTauntSelection" )
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	SetHiddenBits( HIDEHUD_MISCSTATUS );

	for ( int i=0; i<NUM_TAUNT_SLOTS; ++i )
	{
		char pszKeyItemModelPanelName[128];
		V_sprintf_safe( pszKeyItemModelPanelName, "TauntModelPanel%d", i+1 );
		m_pItemModelPanels[i] = new CItemModelPanel( this, pszKeyItemModelPanelName );

		/* char pszKeyIconName[64];
		V_sprintf_safe( pszKeyIconName, "NumberBg%d", i+1 );
		m_pKeyIcons[i] = new CIconPanel( this, pszKeyIconName );

		char pszNumberLabel[64];
		V_sprintf_safe( pszNumberLabel, "NumberLabel%d", i+1 );
		m_pKeyLabels[i] = new CExLabel( this, pszNumberLabel, CFmtStr( "%d", i+1 ) ); */
	}			  

	ListenForGameEvent( "gameui_hidden" );

	m_iSelectedItem = -1;

	InvalidateLayout( false, true );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudMenuTauntSelection::ApplySchemeSettings( IScheme *pScheme )
{
	// load control settings...
	if ( ::input->IsSteamControllerActive() )
	{
		LoadControlSettings( "resource/UI/HudMenuTauntSelection_SC.res" );
		m_iSelectedItem = 1;
	}
	else
	{
		LoadControlSettings( "resource/UI/HudMenuTauntSelection.res" );
	}

	BaseClass::ApplySchemeSettings( pScheme );

	UpdateItemModelPanels();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHudMenuTauntSelection::ShouldDraw( void )
{
	if ( !CHudElement::ShouldDraw() )
		return false;

	CTFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pPlayer || !pPlayer->IsAlive() )
		return false;

	if ( pPlayer->m_Shared.InCond( TF_COND_TAUNTING ) )
		return false;

	if ( pPlayer->m_Shared.InCond( TF_COND_COMPETITIVE_LOSER ) )
		return false;

	if ( pPlayer->m_Shared.InCond( TF_COND_HALLOWEEN_KART ) )
		return false;

	return pPlayer->ShouldShowHudMenuTauntSelection();
}

//-----------------------------------------------------------------------------
// Purpose: Keyboard input hook. Return 0 if handled
//-----------------------------------------------------------------------------
int	CHudMenuTauntSelection::HudElementKeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding )
{
	if ( !ShouldDraw() )
	{
		return 1;
	}

	if ( !down )
	{
		return 1;
	}

	static struct TauntInput_t
	{
		const char *m_pszCommand;
		int m_iReturnValue;
		bool m_bDoWeaponTaunt;
	} s_tauntInput[] =
	{
		{ "lastinv",		0,	false },
		{ "invnext",		1,	false },
		{ "invprev",		1,	false },
		{ "+attack",		1,	false },
		{ "+taunt",			0,	true },
		{ "weapon_taunt",	0,	true },
	};

	// Handles specific key
	for ( int i=0; i<ARRAYSIZE( s_tauntInput ); ++i )
	{
		if ( keynum == gameuifuncs->GetButtonCodeForBind( s_tauntInput[i].m_pszCommand ) )
		{
			CTFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
			if ( s_tauntInput[i].m_bDoWeaponTaunt )
			{
				SelectTaunt( 0 );
			}
			else if ( pPlayer )
			{
				pPlayer->SetShowHudMenuTauntSelection( false );
			}

			return s_tauntInput[i].m_iReturnValue;
		}
	}

	bool bController = ( IsConsole() || ( keynum >= JOYSTICK_FIRST ) );

	if ( bController )
	{
		int iNewSelection = m_iSelectedItem;

		switch( keynum )
		{
		case KEY_XBUTTON_UP:
			// jump to last
			iNewSelection = NUM_TAUNT_SLOTS;
			break;

		case KEY_XBUTTON_DOWN:
			// jump to first
			iNewSelection = 1;
			break;

		case KEY_XBUTTON_RIGHT:
		case STEAMCONTROLLER_DPAD_RIGHT:
			// move selection to the right
			iNewSelection++;
			if ( iNewSelection > NUM_TAUNT_SLOTS )
				iNewSelection = 1;
			break;

		case KEY_XBUTTON_LEFT:
		case STEAMCONTROLLER_DPAD_LEFT:
			// move selection to the right
			iNewSelection--;
			if ( iNewSelection < 1 )
				iNewSelection = NUM_TAUNT_SLOTS;
			break;

		case KEY_XBUTTON_RTRIGGER:
		case KEY_XBUTTON_A:
		case STEAMCONTROLLER_A:
			{
				SelectTaunt( m_iSelectedItem );
			}
			return 0;

		case STEAMCONTROLLER_B:
			{
				CTFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
				if ( pPlayer )
				{
					pPlayer->SetShowHudMenuTauntSelection( false );
				}
			}
			return 0;

		case STEAMCONTROLLER_X:
			SelectTaunt( 0 );		// Weapon X
			return 0;

		default:
			return 1;	// key not handled
		}

		SetSelectedItem( iNewSelection );

		return 0;
	}
	else
	{
		int iSlot = -1;

#ifdef SIXENSE
		if ( !g_pSixenseInput->IsEnabled() )
#endif
		{
			// convert slot1, slot2 etc to 1,2,3,4
			if ( pszCurrentBinding && !Q_strncmp( pszCurrentBinding, "slot", 4 ) && Q_strlen(pszCurrentBinding) > 4 )
			{
				const char *pszNum = pszCurrentBinding+4;
				iSlot = atoi(pszNum);

				// allow slot1 - slot4 
				if ( iSlot < 1 || iSlot > NUM_TAUNT_SLOTS )
					return 1;
			}
		}

		if ( iSlot == -1 )
		{
			switch( keynum )
			{
			case KEY_1:
			case KEY_2:
			case KEY_3:
			case KEY_4:
			case KEY_5:
			case KEY_6:
			case KEY_7:
			case KEY_8:
				{
					iSlot = keynum - KEY_1 + 1;
				}
				break;

			default:
				return 1;	// key not handled
			}
		}

		if ( iSlot >= 0 )
		{
			SelectTaunt( iSlot );

			return 0;
		}
	}
	
	return 1;	// key not handled
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudMenuTauntSelection::FindTauntKeyBinding( void )
{
	const char *key = engine->Key_LookupBinding( "taunt" );
	if ( !key )
	{
		key = "< not bound >";
	}
	SetDialogVariable( "taunt", key );

	key = engine->Key_LookupBinding( "lastinv" );
	if ( !key )
	{
		key = "< not bound >";
	}
	SetDialogVariable( "lastinv", key );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
extern const char *g_szItemBorders[][5];
void CHudMenuTauntSelection::UpdateItemModelPanels()
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pPlayer )
		return;

	bool bSteamController = ::input->IsSteamControllerActive();

	int iClass = pPlayer->GetPlayerClass()->GetClassIndex();
	vgui::IScheme *pScheme = vgui::scheme()->GetIScheme( GetScheme() );

	for ( int i=0; i<ARRAYSIZE( m_pItemModelPanels ); ++i )
	{
		int iTauntSlot = LOADOUT_POSITION_TAUNT + i;

		CItemModelPanel *pItemModelPanel = m_pItemModelPanels[i];

		CEconItemView *pOwnedItemInSlot = pPlayer->Inventory()->GetCacheServerItemInLoadout( iClass, iTauntSlot );
		pItemModelPanel->SetItem( pOwnedItemInSlot );
		pItemModelPanel->SetNoItemText( "#Hud_Menu_Taunt_NoItem" );

		int iRarity = 0;
		if ( pOwnedItemInSlot && pOwnedItemInSlot->IsValid() )
			iRarity = pOwnedItemInSlot->GetItemQuality();

		const char *pszBorder = g_szItemBorders[iRarity][0];
		
		IBorder *pBorder = pScheme->GetBorder( pszBorder );

		pItemModelPanel->SetBorder( !bSteamController || i == (m_iSelectedItem - 1) ? pBorder : nullptr );

		pItemModelPanel->UpdatePanels();
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudMenuTauntSelection::SelectTaunt( int iTaunt )
{
	CTFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( pPlayer )
	{
		if ( !pPlayer->IsAllowedToTaunt() )
		{
			pPlayer->EmitSound( "Player.DenyWeaponSelection" );
			return;
		}

		char pszTaunt[32];
		V_sprintf_safe( pszTaunt, "taunt %d", iTaunt );

		engine->ClientCmd( pszTaunt );

		pPlayer->SetShowHudMenuTauntSelection( false );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudMenuTauntSelection::SetSelectedItem( int iSlot )
{
	if ( m_iSelectedItem != iSlot )
	{
		m_iSelectedItem = iSlot;
		UpdateItemModelPanels();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudMenuTauntSelection::FireGameEvent( IGameEvent *event )
{
	const char * type = event->GetName();

	if ( Q_strcmp(type, "gameui_hidden") == 0 )
	{
		FindTauntKeyBinding();
	}
	else
	{
		CHudElement::FireGameEvent( event );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudMenuTauntSelection::SetVisible( bool state )
{
	if ( state == true )
	{
		// close the weapon selection menu
		engine->ClientCmd( "cancelselect" );

		FindTauntKeyBinding();

		HideLowerPriorityHudElementsInGroup( "mid" );

		InvalidateLayout( true, true );
	}
	else
	{
		UnhideLowerPriorityHudElementsInGroup( "mid" );
	}

	BaseClass::SetVisible( state );
}


static void OpenTauntSelectionUI()
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pPlayer )
		return;

	if ( !pPlayer->IsAllowedToTaunt() )
		return;

	if ( pPlayer->ShouldShowHudMenuTauntSelection() )
		return;

	CTFWeaponBase *pWpn = pPlayer->GetActiveTFWeapon();
	if ( !pWpn )
		return;

	if ( pWpn->GetWeaponID() == TF_WEAPON_PDA_SPY )
	{
		engine->ClientCmd( "taunt" );
		return;
	}

	int iClass = pPlayer->GetPlayerClass()->GetClassIndex();
	bool bHasAnyTauntEquipped = false;
	CTFPlayerInventory *pInv = pPlayer->Inventory();
	if ( pInv )
	{
		for ( int iTauntSlot = LOADOUT_POSITION_TAUNT; iTauntSlot <= LOADOUT_POSITION_TAUNT8; ++iTauntSlot )
		{
			CEconItemView *pItem = pInv->GetCacheServerItemInLoadout( iClass, iTauntSlot );
			if ( pItem && pItem->IsValid() )
			{
				bHasAnyTauntEquipped = true;
				break;
			}
		}
	}

	if ( !bHasAnyTauntEquipped )
	{
		engine->ClientCmd( "taunt" );
		return;
	}

	pPlayer->SetShowHudMenuTauntSelection( true );
}
static ConCommand in_taunt_keydown( "+taunt", OpenTauntSelectionUI );
