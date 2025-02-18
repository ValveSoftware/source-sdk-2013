//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//


#include "cbase.h"
#include <vgui/ILocalize.h>
#include "vgui_controls/TextEntry.h"
#include "trading_start_dialog.h"
#include "econ_controls.h"
#include "econ_trading.h"
#include "c_playerresource.h"
#include "gcsdk/gcmsg.h"
#include "econ_item_inventory.h"
#include "econ_gcmessages.h"
#include "gc_clientsystem.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTradingStartDialog::CTradingStartDialog( vgui::Panel *parent ) : vgui::EditablePanel( parent, "TradingStartDialog" )
{
	m_pSelectFromServerButton = NULL;
	m_pCancelButton = NULL;
	m_pButtonKV = NULL;
	m_bReapplyButtonKVs = false;
	m_pURLFailLabel = NULL;
	m_pURLSearchingLabel = NULL;

	for ( int i = 0; i < TDS_NUM_STATES; i++ )
	{
		m_pStatePanels[i] = new vgui::EditablePanel( this, VarArgs("StatePanel%d",i) );
	}

	m_pPlayerList = new vgui::EditablePanel( this, "PlayerList" );
	m_pPlayerListScroller = new vgui::ScrollableEditablePanel( this, m_pPlayerList, "PlayerListScroller" );

	ListenForGameEvent( "gameui_hidden" );

	Reset();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTradingStartDialog::~CTradingStartDialog( void )
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
void CTradingStartDialog::Reset( void )
{
	m_iCurrentState = TDS_SELECTING_PLAYER;
	m_bGiftMode = false;
	m_giftItem = CEconItemView();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTradingStartDialog::ApplySettings( KeyValues *inResourceData )
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
void CTradingStartDialog::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "resource/ui/econ/TradingStartDialog.res" );

	m_pCancelButton = dynamic_cast<CExButton*>( FindChildByName( "CancelButton" ) );

	// Find all the sub buttons, and set their action signals to point to this panel
	for ( int i = 0; i < TDS_NUM_STATES; i++ )
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

	m_pURLFailLabel = dynamic_cast<vgui::Label*>( m_pStatePanels[TDS_SELECTING_FROM_PROFILE]->FindChildByName( "URLFailLabel" ) );
	m_pURLSearchingLabel = dynamic_cast<vgui::Label*>( m_pStatePanels[TDS_SELECTING_FROM_PROFILE]->FindChildByName( "URLSearchingLabel" ) );

	UpdateState();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTradingStartDialog::PerformLayout( void ) 
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
void CTradingStartDialog::FireGameEvent( IGameEvent *event )
{
	const char *type = event->GetName();

	if ( Q_strcmp(type, "gameui_hidden") == 0 )
	{
		Close();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTradingStartDialog::Close( void )
{
	TFModalStack()->PopModal( this );
	SetVisible( false );
	MarkForDeletion();
	PostMessage( GetParent(), new KeyValues("CancelSelection") );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTradingStartDialog::OnCommand( const char *command )
{
	if ( !Q_stricmp( command, "cancel" ) )
	{
		if ( m_iCurrentState != TDS_SELECTING_PLAYER )
		{
			m_iCurrentState = TDS_SELECTING_PLAYER;
			UpdateState();
			return;
		}

		Close();
		return;
	}
	else if ( !Q_stricmp( command, "friends" ) )
	{
		m_iCurrentState = TDS_SELECTING_FROM_FRIENDS;
		UpdateState();
		return;
	}
	else if ( !Q_stricmp( command, "server" ) )
	{
		m_iCurrentState = TDS_SELECTING_FROM_SERVER;
		UpdateState();
		return;
	}
	else if ( !Q_stricmp( command, "profile" ) )
	{
		m_iCurrentState = TDS_SELECTING_FROM_PROFILE;
		UpdateState();
		return;
	}
	else if ( !Q_strnicmp( command, "select_player", 13 ) )
	{
		int iPlayer = atoi( command + 13 ) - 1;
		if ( iPlayer >= 0 && iPlayer < m_PlayerInfoList.Count() )
		{
			StartTradeWith( m_PlayerInfoList[iPlayer].m_steamID );
		}
		return;
	}
	else if ( !Q_stricmp( command, "url_ok" ) )
	{
		vgui::TextEntry *pEntry = dynamic_cast<vgui::TextEntry*>( m_pStatePanels[m_iCurrentState]->FindChildByName("URLEntry") );
		if ( pEntry )
		{
			const int maxURLLength = 512;
			char inputURL[ maxURLLength ];
			pEntry->GetText( inputURL, maxURLLength );

			if ( m_pURLSearchingLabel )
			{
				m_pURLSearchingLabel->SetVisible( false );
			}

			bool bSuccess = ExtractSteamIDFromURL( inputURL );
			if ( m_pURLFailLabel )
			{
				m_pURLFailLabel->SetVisible( !bSuccess );
			}
		}

		return;
	}

	BaseClass::OnCommand( command );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTradingStartDialog::SendGiftTo( CSteamID steamID )
{
	Trading_SendGift( steamID, m_giftItem );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTradingStartDialog::StartTradeWith( CSteamID steamID )
{
	m_iCurrentState = TDS_SELECTING_PLAYER;
	OnCommand( "cancel" );

	if ( !m_bGiftMode ) 
	{
		Trading_RequestTrade( steamID );
		return;	
	}

	Trading_SendGift( steamID, m_giftItem );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTradingStartDialog::ExtractSteamIDFromURL( char *inputURL )
{
	if ( !inputURL || !inputURL[0] )
		return false;

	EUniverse localUniverse = GetUniverse();
	if ( localUniverse == k_EUniverseInvalid )
		return false;

	CSteamID steamID;
	int iLen = Q_strlen(inputURL);

	// First, see if it's a profile link. If it is, clip the SteamID from it.
	const char *pszProfilePrepend = ( localUniverse == k_EUniversePublic ) ? "http://steamcommunity.com/profiles/" : "http://beta.steamcommunity.com/profiles/";
	int iProfilePrependLen = Q_strlen(pszProfilePrepend);
	if ( Q_strnicmp( pszProfilePrepend, inputURL, iProfilePrependLen ) == 0 )
	{
		if ( iLen > iProfilePrependLen )
		{
			steamID.SetFromString( &inputURL[iProfilePrependLen], localUniverse );

			if ( steamID.IsValid() )
			{
				StartTradeWith( steamID );
				return true;
			}
		}
	}
	else
	{
		// If it's an id link, we download it and extract the steam ID from it.
		const char *pszIDPrepend = ( localUniverse == k_EUniversePublic ) ? "http://steamcommunity.com/id/" : "http://beta.steamcommunity.com/id/";
		int iIDPrependLen = Q_strlen(pszIDPrepend);
		if ( Q_strnicmp( pszIDPrepend, inputURL, iIDPrependLen ) == 0 )
		{
			if ( iLen > iIDPrependLen )
			{
				// Trim off a trailing slash
				if ( inputURL[iLen-1] == '/' || inputURL[iLen-1] == '\\' )
				{
					inputURL[iLen-1] = '\0';
				}


				// For now, return true and wait.
				if ( m_pURLSearchingLabel )
				{
					m_pURLSearchingLabel->SetVisible( true );
				}

				return true;
			}
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTradingStartDialog::OnLookupAccountResponse( uint64 iAccountID )
{
	if ( m_pURLSearchingLabel )
	{
		m_pURLSearchingLabel->SetVisible( false );
	}

	CSteamID steamID( iAccountID );
	if ( steamID.IsValid() )
	{
		if ( m_pURLFailLabel )
		{
			m_pURLFailLabel->SetVisible( false );
		}

		StartTradeWith( steamID );
	}
	else
	{
		if ( m_pURLFailLabel )
		{
			m_pURLFailLabel->SetVisible( true );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Sets a gift for the dialog to hand out. If pGiftItem is NULL, which is 
// fine, make sure to clear our our own existing gift.
//-----------------------------------------------------------------------------
void CTradingStartDialog::SetGift( CEconItemView* pGiftItem )
{
	if ( pGiftItem )
	{
		m_giftItem = *pGiftItem;
		m_bGiftMode = true;
	}
	else
	{
		// Reset to default
		m_giftItem = CEconItemView();
		m_bGiftMode = false;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTradingStartDialog::OnTextChanged( KeyValues *data )
{
	vgui::TextEntry *pEntry = dynamic_cast<vgui::TextEntry*>( m_pStatePanels[m_iCurrentState]->FindChildByName("URLEntry") );
	if ( !pEntry )
		return;

	Panel *pPanel = reinterpret_cast<vgui::Panel *>( data->GetPtr("panel") );
	if ( pEntry == pPanel )
	{
		CExButton *pButton = dynamic_cast<CExButton*>( m_pStatePanels[m_iCurrentState]->FindChildByName( "subbutton0" ) );
		if ( pButton )
		{
			pButton->SetEnabled( pEntry->GetTextLength() > 0 );
		}

		if ( m_pURLFailLabel )
		{
			m_pURLFailLabel->SetVisible( false );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTradingStartDialog::UpdateState( void )
{
	for ( int i = 0; i < TDS_NUM_STATES; i++ )
	{
		if ( !m_pStatePanels[i] )
			continue;

		m_pStatePanels[i]->SetVisible( m_iCurrentState == i );
	}

	if ( m_pSelectFromServerButton )
	{
		m_pSelectFromServerButton->SetEnabled( engine->IsInGame() );
	}

	if ( m_iCurrentState == TDS_SELECTING_PLAYER )
	{
		m_pCancelButton->SetText( g_pVGuiLocalize->Find( "#Cancel" ) );
	}
	else
	{
		m_pCancelButton->SetText( g_pVGuiLocalize->Find( "#TF_Back" ) );
	}

	switch ( m_iCurrentState )
	{
	case TDS_SELECTING_FROM_FRIENDS:
		SetupSelectFriends();
		break;
	case TDS_SELECTING_FROM_SERVER:
		SetupSelectServer();
		break;
	case TDS_SELECTING_FROM_PROFILE:
		SetupSelectProfile();
		break;
	case TDS_SELECTING_PLAYER:
	default:
		m_pPlayerListScroller->SetVisible( false );
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTradingStartDialog::SetupSelectFriends( void )
{
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
			if ( !steamapicontext->SteamFriends()->GetFriendGamePlayed( friendSteamID, &gameInfo ) )
				continue;

			// Friends is in-game. Make sure it's TF2.
			if ( gameInfo.m_gameID.IsValid() && gameInfo.m_gameID == myGameInfo.m_gameID )
			{
				const char *pszName = steamapicontext->SteamFriends()->GetFriendPersonaName( friendSteamID );
				int idx = m_PlayerInfoList.AddToTail();
				trade_partner_info_t &info = m_PlayerInfoList[idx];
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
void CTradingStartDialog::SetupSelectServer( void )
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
				int idx = m_PlayerInfoList.AddToTail();
				trade_partner_info_t &info = m_PlayerInfoList[idx];
				info.m_steamID = steamID;
				info.m_name = pi.name;
			}
		}
	}

	UpdatePlayerList();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTradingStartDialog::SetupSelectProfile( void )
{
	vgui::TextEntry *pEntry = dynamic_cast<vgui::TextEntry*>( m_pStatePanels[m_iCurrentState]->FindChildByName("URLEntry") );
	if ( pEntry )
	{
		pEntry->SetText( "" );
		pEntry->RequestFocus();
		pEntry->AddActionSignalTarget( this );
	}

	if ( m_pURLFailLabel )
	{
		m_pURLFailLabel->SetVisible( false );
	}
	if ( m_pURLSearchingLabel )
	{
		m_pURLSearchingLabel->SetVisible( false );
	}

	CExButton *pButton = dynamic_cast<CExButton*>( m_pStatePanels[m_iCurrentState]->FindChildByName( "subbutton0" ) );
	if ( pButton )
	{
		pButton->SetEnabled( false );
	}

	m_pPlayerListScroller->SetVisible( false );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTradingStartDialog::UpdatePlayerList( void )
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

	// Otherwise, build the player panels from the list of steam IDs
	for ( int i = 0; i < m_PlayerInfoList.Count(); i++ )
	{
		if ( m_pPlayerPanels.Count() <= i )
		{
			m_pPlayerPanels.AddToTail();
			m_pPlayerPanels[i] = new CTradeTargetPanel( m_pPlayerList, VarArgs("player%d",i) );
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

		m_pPlayerPanels[i]->SetInfo( m_PlayerInfoList[i].m_steamID, m_PlayerInfoList[i].m_name.Get() );
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
void CTradeTargetPanel::SetInfo( const CSteamID &steamID, const char *pszName )
{
	if ( !steamapicontext || !steamapicontext->SteamFriends() )
		return;

	m_pAvatar->SetPlayer( steamID, k_EAvatarSize64x64 );

	m_pButton->SetText( pszName );
}

static vgui::DHANDLE<CTradingStartDialog> g_hTradingStartDialog;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTradingStartDialog *OpenTradingStartDialog( vgui::Panel *pParent, CEconItemView* pOptGiftItem )
{
	if (!g_hTradingStartDialog.Get())
	{
		g_hTradingStartDialog = vgui::SETUP_PANEL( new CTradingStartDialog( pParent ) );
	}
	g_hTradingStartDialog->InvalidateLayout( false, true );

	g_hTradingStartDialog->Reset();
	g_hTradingStartDialog->SetVisible( true );
	g_hTradingStartDialog->MakePopup();
	g_hTradingStartDialog->MoveToFront();
	g_hTradingStartDialog->SetKeyBoardInputEnabled(true);
	g_hTradingStartDialog->SetMouseInputEnabled(true);
	g_hTradingStartDialog->SetGift( pOptGiftItem );
	TFModalStack()->PushModal( g_hTradingStartDialog );

	return g_hTradingStartDialog;
}


//-----------------------------------------------------------------------------
// Purpose: GC Msg handler to receive the Lookup Account response
//-----------------------------------------------------------------------------
class CGCLookupAccountResponse : public GCSDK::CGCClientJob
{
public:
	CGCLookupAccountResponse( GCSDK::CGCClient *pClient ) : GCSDK::CGCClientJob( pClient ) {}

	virtual bool BYieldingRunGCJob( GCSDK::IMsgNetPacket *pNetPacket )
	{
		GCSDK::CGCMsg<MsgGCStandardResponse_t> msg( pNetPacket );

		uint8 iAccounts = 0;
		uint64 iAccountID = 0;
		if ( msg.BReadUint8Data( &iAccounts ) )
		{
			// We only care about the first account we find in this panel.
			if ( iAccounts > 0 )
			{
				msg.BReadUint64Data( &iAccountID );
			}
		}

		if ( g_hTradingStartDialog.Get() )
		{
			g_hTradingStartDialog->OnLookupAccountResponse( iAccountID );
		}
		return true;
	}

};
GC_REG_JOB( GCSDK::CGCClient, CGCLookupAccountResponse, "CGCLookupAccountResponse", k_EMsgGCLookupAccountResponse, GCSDK::k_EServerTypeGCClient );
