//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//


#include "cbase.h"
#include <vgui/ILocalize.h>
#include "vgui_controls/TextEntry.h"
#include "select_player_dialog.h"
#include "tf_controls.h"
#include "c_playerresource.h"
#include "ienginevgui.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

int CSelectPlayerDialog::SortPartnerInfoFunc( const partner_info_t *pA, const partner_info_t *pB )
{
	return Q_stricmp( pA->m_name.Get(), pB->m_name.Get() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CSelectPlayerDialog::CSelectPlayerDialog( vgui::Panel *parent ) 
  : vgui::EditablePanel( parent, "SelectPlayerDialog" )
  , m_bAllowSameTeam( true )
  , m_bAllowOutsideServer( true )
{
	if ( parent == NULL )
	{
		vgui::HScheme scheme = vgui::scheme()->LoadSchemeFromFileEx( enginevgui->GetPanel( PANEL_CLIENTDLL ), "resource/ClientScheme.res", "ClientScheme");
		SetScheme(scheme);
		SetProportional( true );
	}

	m_pSelectFromServerButton = NULL;
	m_pCancelButton = NULL;
	m_pButtonKV = NULL;
	m_bReapplyButtonKVs = false;

	for ( int i = 0; i < SPDS_NUM_STATES; i++ )
	{
		m_pStatePanels[i] = new vgui::EditablePanel( this, VarArgs("StatePanel%d",i) );
	}

	m_pPlayerList = new vgui::EditablePanel( this, "PlayerList" );
	m_pPlayerListScroller = new vgui::ScrollableEditablePanel( this, m_pPlayerList, "PlayerListScroller" );

	m_iCurrentState = SPDS_SELECTING_PLAYER;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CSelectPlayerDialog::~CSelectPlayerDialog( void )
{
	if ( m_pButtonKV )
	{
		m_pButtonKV->deleteThis();
		m_pButtonKV = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSelectPlayerDialog::Reset( void )
{
	m_iCurrentState = SPDS_SELECTING_PLAYER;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSelectPlayerDialog::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );

	KeyValues *pItemKV = inResourceData->FindKey( "button_kv" );
	if ( pItemKV )
	{
		if ( m_pButtonKV )
		{
			m_pButtonKV->deleteThis();
		}
		m_pButtonKV = new KeyValues("button_kv");
		pItemKV->CopySubkeys( m_pButtonKV );

		m_bReapplyButtonKVs = true;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSelectPlayerDialog::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( GetResFile() );

	m_pCancelButton = dynamic_cast<CExButton*>( FindChildByName( "CancelButton" ) );

	// Find all the sub buttons, and set their action signals to point to this panel
	for ( int i = 0; i < SPDS_NUM_STATES; i++ )
	{
		int iButton = 0;
		CExButton *pButton = NULL;
		do
		{
			pButton = dynamic_cast<CExButton*>( m_pStatePanels[i]->FindChildByName( VarArgs("subbutton%d",iButton)) );
			if ( pButton )
			{
				pButton->AddActionSignalTarget( this );

				// The second button on the first state is the server button
				if ( iButton == 1 )
				{
					m_pSelectFromServerButton = pButton;
				}

				iButton++;
			}
		} while (pButton);
	}

	UpdateState();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSelectPlayerDialog::PerformLayout( void ) 
{
	BaseClass::PerformLayout();

	// Layout the player list buttons
	if ( m_pPlayerPanels.Count() )
	{
		int iButtonH = m_pPlayerPanels[0]->GetTall() + YRES(2);
		m_pPlayerList->SetSize( m_pPlayerList->GetWide(), YRES(2) + (iButtonH * m_pPlayerPanels.Count()) );

		// These need to all be layout-complete before we can position the player panels,
		// because the scrollbar will cause the playerlist entries to move when it lays out.
		m_pPlayerList->InvalidateLayout( true );
		m_pPlayerListScroller->InvalidateLayout( true );
		m_pPlayerListScroller->GetScrollbar()->InvalidateLayout( true );

		for ( int i = 0; i < m_pPlayerPanels.Count(); i++ )
		{
			m_pPlayerPanels[i]->SetPos( 0, YRES(2) + (iButtonH * i) );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSelectPlayerDialog::OnCommand( const char *command )
{
	if ( !Q_stricmp( command, "cancel" ) )
	{
		if ( m_iCurrentState != SPDS_SELECTING_PLAYER )
		{
			m_iCurrentState = SPDS_SELECTING_PLAYER;
			UpdateState();
			return;
		}

		TFModalStack()->PopModal( this );
		SetVisible( false );
		MarkForDeletion();
		if ( GetParent() )
		{
			PostMessage( GetParent(), new KeyValues("CancelSelection") );
		}
		return;
	}
	else if ( !Q_stricmp( command, "friends" ) )
	{
		m_iCurrentState = SPDS_SELECTING_FROM_FRIENDS;
		UpdateState();
		return;
	}
	else if ( !Q_stricmp( command, "server" ) )
	{
		m_iCurrentState = SPDS_SELECTING_FROM_SERVER;
		UpdateState();
		return;
	}
	else if ( !Q_strnicmp( command, "select_player", 13 ) )
	{
		int iPlayer = atoi( command + 13 ) - 1;
		if ( iPlayer >= 0 && iPlayer < m_PlayerInfoList.Count() )
		{
			m_iCurrentState = SPDS_SELECTING_PLAYER;
			OnCommand( "cancel" );
			OnSelectPlayer( m_PlayerInfoList[iPlayer].m_steamID );
		}
		return;
	}

	BaseClass::OnCommand( command );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSelectPlayerDialog::UpdateState( void )
{
	for ( int i = 0; i < SPDS_NUM_STATES; i++ )
	{
		if ( !m_pStatePanels[i] )
			continue;

		m_pStatePanels[i]->SetVisible( m_iCurrentState == i );
	}

	if ( m_pSelectFromServerButton )
	{
		m_pSelectFromServerButton->SetEnabled( engine->IsInGame() );
	}

	if ( m_iCurrentState == SPDS_SELECTING_PLAYER )
	{
		m_pCancelButton->SetText( g_pVGuiLocalize->Find( "#Cancel" ) );
	}
	else
	{
		m_pCancelButton->SetText( g_pVGuiLocalize->Find( "#TF_Back" ) );
	}

	switch ( m_iCurrentState )
	{
	case SPDS_SELECTING_FROM_FRIENDS:
		SetupSelectFriends();
		break;
	case SPDS_SELECTING_FROM_SERVER:
		SetupSelectServer( false );
		break;
	case SPDS_SELECTING_PLAYER:
	default:
		m_pPlayerListScroller->SetVisible( false );
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSelectPlayerDialog::SetupSelectFriends( void )
{
	// @todo optional check to see if friend is on my server
	if ( m_bAllowOutsideServer == false )
	{
		SetupSelectServer( true );
		return;
	}

	m_PlayerInfoList.Purge();

	if ( steamapicontext && steamapicontext->SteamFriends() )
	{
		// Get our game info so we can use that to test if our friends are connected to the same game as us
		FriendGameInfo_t myGameInfo;
		CSteamID mySteamID = steamapicontext->SteamUser()->GetSteamID();
		steamapicontext->SteamFriends()->GetFriendGamePlayed( mySteamID, &myGameInfo );

		int iFriends = steamapicontext->SteamFriends()->GetFriendCount( k_EFriendFlagImmediate );
		for ( int i = 0; i < iFriends; i++ )
		{
			CSteamID friendSteamID = steamapicontext->SteamFriends()->GetFriendByIndex( i, k_EFriendFlagImmediate );

			FriendGameInfo_t gameInfo;
			if ( !AllowOutOfGameFriends() && !steamapicontext->SteamFriends()->GetFriendGamePlayed( friendSteamID, &gameInfo ) )
				continue;

			// Friends is in-game. Make sure it's TF2.
			if ( AllowOutOfGameFriends() || (gameInfo.m_gameID.IsValid() && gameInfo.m_gameID == myGameInfo.m_gameID) )
			{
				const char *pszName = steamapicontext->SteamFriends()->GetFriendPersonaName( friendSteamID );
				int idx = m_PlayerInfoList.AddToTail();
				partner_info_t &info = m_PlayerInfoList[idx];
				info.m_steamID = friendSteamID;
				info.m_name = pszName;
			}
		}
	}

	UpdatePlayerList();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSelectPlayerDialog::SetupSelectServer( bool bFriendsOnly )
{
	m_PlayerInfoList.Purge();

	if ( steamapicontext && steamapicontext->SteamUtils() )
	{
		for( int iPlayerIndex = 1 ; iPlayerIndex <= MAX_PLAYERS; iPlayerIndex++ )
		{
			// find all players who are on the local player's team
			int iLocalPlayerIndex =  GetLocalPlayerIndex();
			if( ( iPlayerIndex != iLocalPlayerIndex ) && ( g_PR->IsConnected( iPlayerIndex ) ) )
			{
				player_info_t pi;
				if ( !engine->GetPlayerInfo( iPlayerIndex, &pi ) )
					continue;
				if ( !pi.friendsID )
					continue;

				CSteamID steamID( pi.friendsID, 1, GetUniverse(), k_EAccountTypeIndividual );

				if ( bFriendsOnly )
				{
					EFriendRelationship eRelationship = steamapicontext->SteamFriends()->GetFriendRelationship( steamID );
					if ( eRelationship != k_EFriendRelationshipFriend )
					{
						continue;
					}
				}

				if ( g_PR->GetTeam( iPlayerIndex ) != TF_TEAM_RED && g_PR->GetTeam( iPlayerIndex ) != TF_TEAM_BLUE )
					continue;

				if ( m_bAllowSameTeam == false )
				{
					if ( GetLocalPlayerTeam() == g_PR->GetTeam( iPlayerIndex ) )
					{
						continue;
					}
				}

				int idx = m_PlayerInfoList.AddToTail();
				partner_info_t &info = m_PlayerInfoList[idx];
				info.m_steamID = steamID;
				info.m_name = UTIL_GetFilteredPlayerName( iPlayerIndex, pi.name );
			}
		}
	}

	UpdatePlayerList();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSelectPlayerDialog::UpdatePlayerList( void )
{
	vgui::Label *pLabelEmpty = dynamic_cast<vgui::Label*>( m_pStatePanels[m_iCurrentState]->FindChildByName("EmptyPlayerListLabel") );
	vgui::Label *pLabelQuery = dynamic_cast<vgui::Label*>( m_pStatePanels[m_iCurrentState]->FindChildByName("QueryLabel") );

	// If we have no players in our list, show the no-player label.
	if ( m_PlayerInfoList.Count() == 0 )
	{
		if ( pLabelEmpty )
		{
			pLabelEmpty->SetVisible( true );
		}
		if ( pLabelQuery )
		{
			pLabelQuery->SetVisible( false );
		}
		return;
	}

	// First, reapply any KVs we have to reapply
	if ( m_bReapplyButtonKVs )
	{
		m_bReapplyButtonKVs = false;

		if ( m_pButtonKV )
		{
			FOR_EACH_VEC( m_pPlayerPanels, i )
			{
				m_pPlayerPanels[i]->ApplySettings( m_pButtonKV );
			}
		}
	}

	// sort by name
	m_PlayerInfoList.Sort( &SortPartnerInfoFunc );

	// Otherwise, build the player panels from the list of steam IDs
	for ( int i = 0; i < m_PlayerInfoList.Count(); i++ )
	{
		if ( m_pPlayerPanels.Count() <= i )
		{
			m_pPlayerPanels.AddToTail();
			m_pPlayerPanels[i] = new CSelectPlayerTargetPanel( m_pPlayerList, VarArgs("player%d",i) );
			m_pPlayerPanels[i]->GetButton()->SetCommand( VarArgs("select_player%d",i+1) );
			m_pPlayerPanels[i]->GetButton()->AddActionSignalTarget( this );
			m_pPlayerPanels[i]->GetAvatar()->SetShouldDrawFriendIcon( false );
			m_pPlayerPanels[i]->GetAvatar()->SetMouseInputEnabled( false );

			if ( m_pButtonKV )
			{
				m_pPlayerPanels[i]->ApplySettings( m_pButtonKV );
				m_pPlayerPanels[i]->InvalidateLayout( true );
			} 
		}

		m_pPlayerPanels[i]->SetInfo( m_PlayerInfoList[i].m_steamID, m_PlayerInfoList[i].m_name );
	}

	m_pPlayerListScroller->GetScrollbar()->SetAutohideButtons( true );
	m_pPlayerListScroller->GetScrollbar()->SetValue( 0 );	

	// Remove any extra player panels
	for ( int i = m_pPlayerPanels.Count()-1; i >= m_PlayerInfoList.Count(); i-- )
	{
		m_pPlayerPanels[i]->MarkForDeletion();
		m_pPlayerPanels.Remove(i);
	}

	if ( pLabelEmpty )
	{
		pLabelEmpty->SetVisible( false );
	}
	if ( pLabelQuery )
	{
		pLabelQuery->SetVisible( true );
	}
	m_pPlayerListScroller->SetVisible( true );

	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSelectPlayerTargetPanel::SetInfo( const CSteamID &steamID, const char *pszName )
{
	if ( !steamapicontext || !steamapicontext->SteamFriends() )
		return;

	m_pAvatar->SetPlayer( steamID, k_EAvatarSize64x64 );

	m_pButton->SetText( pszName );
}

