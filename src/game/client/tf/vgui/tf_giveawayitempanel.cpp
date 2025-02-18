//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#include "cbase.h"
#include "vgui/IInput.h"
#include <vgui/IVGui.h>
#include <vgui/IScheme.h>
#include "tf_giveawayitempanel.h"
#include "iclientmode.h"
#include "baseviewport.h"
#include "econ_entity.h"
#include "c_tf_player.h"
#include "gamestringpool.h"
#include "vgui_controls/TextImage.h"
#include "vgui_controls/Label.h"
#include "vgui_controls/Button.h"
#include "econ_item_system.h"
#include "ienginevgui.h"
#include "achievementmgr.h"
#include "fmtstr.h"
#include "c_tf_playerresource.h"
#include "tf_gamerules.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CGiveawayPlayerPanel::CGiveawayPlayerPanel( vgui::Panel *parent, const char *name ) : BaseClass(parent,name)
{
	m_pNameLabel = new vgui::Label( this, "name_label", "" );
	m_pScoreLabel = new vgui::Label( this, "score_label", "" );

	REGISTER_COLOR_AS_OVERRIDABLE( m_PlayerColorLocal, "fgcolor_local" );
	REGISTER_COLOR_AS_OVERRIDABLE( m_PlayerColorOther, "fgcolor_other" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGiveawayPlayerPanel::PerformLayout( void ) 
{
	BaseClass::PerformLayout();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGiveawayPlayerPanel::SetPlayer( CTFPlayer *pPlayer )
{
	m_iBonus = 0;

	if ( pPlayer )
	{
		SetVisible( true );

		if ( g_TF_PR )
		{
			int playerIndex = pPlayer->entindex();
			const char *pszName = g_TF_PR->GetPlayerName( playerIndex );
			m_iBonus = pPlayer->m_Shared.GetItemFindBonus();

			SetDialogVariable( "playername", pszName );
			SetDialogVariable( "playerscore", m_iBonus );
		}

		m_iPlayerIndex = pPlayer->entindex();

		if ( pPlayer->IsLocalPlayer() )
		{
			m_pNameLabel->SetFgColor( m_PlayerColorLocal );
			m_pScoreLabel->SetFgColor( m_PlayerColorLocal );
		}
		else
		{
			m_pNameLabel->SetFgColor( m_PlayerColorOther );
			m_pScoreLabel->SetFgColor( m_PlayerColorOther );
		}
	}
	else
	{
		SetVisible( false );
		m_iPlayerIndex = 0;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGiveawayPlayerPanel::SpinBonus( void )
{
	SetDialogVariable( "playerscore", RandomInt( PLAYER_ROLL_MIN, PLAYER_ROLL_MAX ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGiveawayPlayerPanel::LockBonus( int iRoll )
{
	m_iRoll = iRoll;
	SetDialogVariable( "playerscore", m_iBonus + m_iRoll );
}

//===========================================================================================================================


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFGiveawayItemPanel::CTFGiveawayItemPanel( IViewPort *pViewPort ) : Frame( NULL, PANEL_GIVEAWAY_ITEM )
{
	m_pViewPort = pViewPort;

	// load the new scheme early!!
	SetScheme( "ClientScheme" );

	SetTitleBarVisible( false );
	SetMinimizeButtonVisible( false );
	SetMaximizeButtonVisible( false );
	SetCloseButtonVisible( false );
	SetSizeable( false );
	SetMoveable( false );
	SetProportional( true );
	SetVisible( false );
	SetKeyBoardInputEnabled( true );
	SetMouseInputEnabled( true );

	m_pModelPanel = new CItemModelPanel( this, "item_panel" );
	m_pPlayerListPanelKVs = NULL;

	m_iNumActivePlayers = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFGiveawayItemPanel::~CTFGiveawayItemPanel( void )
{
	if ( m_pPlayerListPanelKVs )
	{
		m_pPlayerListPanelKVs->deleteThis();
		m_pPlayerListPanelKVs = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGiveawayItemPanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "Resource/UI/GiveawayItemPanel.res" );

	for ( int i = 0; i < m_aPlayerList.Count(); i++ )
	{
		m_aPlayerList[i]->ApplySettings( m_pPlayerListPanelKVs );
		m_aPlayerList[i]->SetBorder( pScheme->GetBorder("EconItemBorder") );
		m_aPlayerList[i]->InvalidateLayout();
	}

	m_pModelPanel->SetNoItemText( "#Item_Giveaway_NoItem" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGiveawayItemPanel::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );

	KeyValues *pItemKV = inResourceData->FindKey( "playerlist_panel_kvs" );
	if ( pItemKV )
	{
		if ( m_pPlayerListPanelKVs )
		{
			m_pPlayerListPanelKVs->deleteThis();
		}
		m_pPlayerListPanelKVs = new KeyValues("playerlist_panel_kvs");
		pItemKV->CopySubkeys( m_pPlayerListPanelKVs );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGiveawayItemPanel::PerformLayout( void ) 
{
	BaseClass::PerformLayout();

	int iPosition = 0;
	for ( int i = 0; i < m_aPlayerList.Count(); i++ )
	{
		if ( !m_aPlayerList[i]->IsVisible() )
			continue;

		int iCenter = GetWide() * 0.5;
		int iXPos = iCenter;
		int iYPos = 0;
		if ( iPosition < (m_iNumActivePlayers * 0.5) )
		{
			iXPos -= m_aPlayerList[i]->GetWide() + m_iPlayerXOffset;
			iYPos = m_iPlayerYPos + iPosition * m_aPlayerList[i]->GetTall();
		}
		else
		{
			iXPos += m_iPlayerXOffset;
			iYPos = m_iPlayerYPos + (iPosition - ceil(m_iNumActivePlayers * 0.5)) * m_aPlayerList[i]->GetTall();
		}
		m_aPlayerList[i]->SetPos( iXPos, iYPos );

		iPosition++;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGiveawayItemPanel::ShowPanel(bool bShow)
{
	if ( bShow )
	{
		SetItem( NULL );
		m_bBuiltPlayerList = false;
		m_flNextRollStart = 0;
		m_iRollingForPlayer = 0;
		m_iNumActivePlayers = 0;
	}

	SetMouseInputEnabled( bShow );
	SetVisible( bShow );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGiveawayItemPanel::FireGameEvent( IGameEvent *event )
{
	const char * type = event->GetName();

	if ( Q_strcmp(type, "gameui_hidden") == 0 )
	{
		ShowPanel( false );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGiveawayItemPanel::OnCommand( const char *command )
{
	if ( !Q_stricmp( command, "vguicancel" ) )
	{
		ShowPanel( false );

		// If we're connected to a game server, we also close the game UI.
		if ( engine->IsInGame() )
		{
			engine->ClientCmd_Unrestricted( "gameui_hide" );
		}
	}
	else
	{
		engine->ClientCmd( const_cast<char *>( command ) );
	}

	BaseClass::OnCommand( command );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGiveawayItemPanel::Update( void )
{
	// First, wait for the player list to get built
	if ( !m_bBuiltPlayerList )
	{
		BuildPlayerList();
		if ( !m_bBuiltPlayerList )
			return;
	}

	// Then wait for the server to send us the item details
	if ( !m_pModelPanel->HasItem() )
	{
		if ( TFGameRules() )
		{
			CBonusRoundLogic *pLogic = TFGameRules()->GetBonusLogic();
			if ( pLogic )
			{
				m_pModelPanel->SetItem( pLogic->GetBonusItem() );
			}
		}

		if ( !m_pModelPanel->HasItem() )
			return;
	}

	// Then start rolling through the players and locking in their bonuses
	if ( m_iRollingForPlayer < m_iNumActivePlayers )
	{
		// Spin all the numbers & lock them in one by one
		for ( int i = 0; i < m_aPlayerList.Count(); i++ )
		{
			if ( m_iRollingForPlayer < i )
			{
				// Haven't got to this player yet, so spin their bonus.
				m_aPlayerList[i]->SpinBonus();
				continue;
			}

			if ( i == m_iRollingForPlayer )
			{
				if ( gpGlobals->curtime > m_flNextRollStart )
				{
					// Lock in this player's bonus and move on to the next one
					CBonusRoundLogic *pLogic = TFGameRules()->GetBonusLogic();
					if ( pLogic )
					{
						m_aPlayerList[i]->LockBonus( pLogic->GetPlayerBonusRoll( m_aPlayerList[i]->GetPlayerIndex() ) );
					}

					m_iRollingForPlayer++;
					m_flNextRollStart = gpGlobals->curtime + 0.4;
				}
				else
				{
					m_aPlayerList[i]->SpinBonus();
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGiveawayItemPanel::BuildPlayerList( void )
{
	CBonusRoundLogic *pLogic = TFGameRules()->GetBonusLogic();
	if ( !pLogic )
		return;

	m_bBuiltPlayerList = true;

	pLogic->BuildBonusPlayerList();

	for ( int i = 0; i < m_aPlayerList.Count(); i++ )
	{
		m_aPlayerList[i]->SetPlayer(NULL);
	}

	m_iNumActivePlayers = 0;
	for( int playerIndex = 0; playerIndex < pLogic->GetNumBonusPlayers(); playerIndex++ )
	{
		C_TFPlayer *pPlayer = pLogic->GetBonusPlayer(playerIndex);
		if ( !pPlayer )
			continue;

		CGiveawayPlayerPanel *pPlayerPanel;
		if ( m_iNumActivePlayers < m_aPlayerList.Count() )
		{
			pPlayerPanel = m_aPlayerList[m_iNumActivePlayers];
		}
		else
		{
			const char *pszCommand = VarArgs("playerpanel%d",m_iNumActivePlayers);
			pPlayerPanel = new CGiveawayPlayerPanel( this, pszCommand );
			if ( m_pPlayerListPanelKVs )
			{
				pPlayerPanel->ApplySettings( m_pPlayerListPanelKVs );
			} 
			pPlayerPanel->MakeReadyForUse();

			vgui::IScheme *pScheme = vgui::scheme()->GetIScheme( GetScheme() );
			pPlayerPanel->SetBorder( pScheme->GetBorder("EconItemBorder") );

			m_aPlayerList.AddToTail( pPlayerPanel );
		}

		pPlayerPanel->SetPlayer( pPlayer );

		m_iNumActivePlayers++;
	}

	// Start the animation
	m_flNextRollStart = gpGlobals->curtime + 1.0;
	m_iRollingForPlayer = 0;

	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGiveawayItemPanel::SetItem( CEconItemView *pItem )
{
	m_pModelPanel->SetItem( pItem );
}

static vgui::DHANDLE<CTFGiveawayItemPanel> g_GiveawayItemPanel;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFGiveawayItemPanel *OpenGiveawayItemPanel( CEconItemView *pItem )
{
	CTFGiveawayItemPanel *pPanel = (CTFGiveawayItemPanel*)gViewPortInterface->FindPanelByName( PANEL_GIVEAWAY_ITEM );
	if ( pPanel )
	{
		pPanel->InvalidateLayout( false, true );
		pPanel->SetItem( pItem );
		gViewPortInterface->ShowPanel( pPanel, true );
	}
	return pPanel;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void Test_GiveawayItemPanel( const CCommand &args )
{
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pLocalPlayer )
		return;

	CEconItemView *pScriptCreatedItem = NULL;

	bool bAllItems = (args.ArgC() <= 1);
	for ( int i = bAllItems ? 0 : clamp( atoi(args[1]), 0, 2 ); i <= 2; i++ )
	{
		CEconEntity *pItem = dynamic_cast<CEconEntity *>( pLocalPlayer->Weapon_GetWeaponByType( i ) );
		if ( !pItem )
			continue;

		pScriptCreatedItem = pItem->GetAttributeContainer()->GetItem();
		break;
	}

	if ( pScriptCreatedItem )
	{
		OpenGiveawayItemPanel( pScriptCreatedItem );
	}
}

ConCommand test_giveawayitem( "test_giveawayitem", Test_GiveawayItemPanel, "Debugging tool to test the item giveaway panel. Usage: test_giveawayitem <weapon name>\n   <weapon id>: 0 = primary, 1 = secondary, 2 = melee.", FCVAR_CHEAT );
