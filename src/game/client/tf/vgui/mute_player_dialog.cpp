//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "mute_player_dialog.h"
#include "gc_clientsystem.h"
#include "ienginevgui.h"
#include "c_tf_playerresource.h"
#include "c_tf_player.h"
#include "vgui_avatarimage.h"
#include <vgui_controls/ImageList.h>
#include "tf_gamerules.h"
#include "voice_status.h"

using namespace vgui;

extern const char *FormatSeconds( int seconds );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CMutePlayerDialog::CMutePlayerDialog( vgui::Panel *parent ) : BaseClass( parent, "MutePlayerDialog" )
{
	SetSize( 320, 270 );
	SetTitle( "#TF_MutePlayerCaps", true );

	m_pMuteButton = new Button( this, "MuteButton", "" );
	m_pPlayerList = new ListPanel( this, "PlayerList" );
	m_pPlayerList->SetEmptyListText( "#GameUI_NoOtherPlayersInGame" );

	m_mapAvatarsToImageList.SetLessFunc( DefLessFunc( CSteamID ) );
	m_mapAvatarsToImageList.RemoveAll();
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CMutePlayerDialog::~CMutePlayerDialog()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMutePlayerDialog::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "Resource/MutePlayerDialog.res" );

	if ( m_pImageList )
	{
		delete m_pImageList;
	}
	m_pImageList = new ImageList( false );

	if ( m_pPlayerList )
	{
		m_pPlayerList->DeleteAllItems();

		m_pPlayerList->AddColumnHeader( 0, "medal", "", m_iMedalWidth, ListPanel::COLUMN_IMAGE );

		// Avatars are always displayed at 32x32 regardless of resolution
		m_pPlayerList->AddColumnHeader( 1, "avatar", "", m_iAvatarWidth, ListPanel::COLUMN_IMAGE );

		// The player avatar is always a fixed size, so as we change resolutions we need to vary the size of the name column to adjust the total width of all the columns
		m_nExtraSpace = m_pPlayerList->GetWide() - m_iMedalWidth - m_iAvatarWidth - m_iNameWidth - m_iScoreWidth - m_iTimeWidth - m_iStatusWidth;

		m_pPlayerList->AddColumnHeader( 2, "name", "#TF_Scoreboard_Name", m_iNameWidth + m_nExtraSpace );
		m_pPlayerList->AddColumnHeader( 3, "score", "#TF_Scoreboard_Score", m_iScoreWidth );
		m_pPlayerList->AddColumnHeader( 4, "time", "#TF_Connected", m_iTimeWidth );
		m_pPlayerList->AddColumnHeader( 5, "status", "", m_iStatusWidth );

		// doesn't make sense to sort with the images
		m_pPlayerList->SetColumnSortable( 0, false );
		m_pPlayerList->SetColumnSortable( 1, false );

		m_pPlayerList->SetImageList( m_pImageList, false );
		m_pPlayerList->SetVisible( true );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMutePlayerDialog::Activate()
{
	BaseClass::Activate();

	if ( m_pPlayerList )
	{
		m_pPlayerList->DeleteAllItems();

		static EUniverse universe = steamapicontext->SteamUtils()->GetConnectedUniverse();

		for ( int playerIndex = 1; playerIndex <= MAX_PLAYERS; playerIndex++ )
		{
			if ( g_PR->IsConnected( playerIndex ) || g_PR->IsValid( playerIndex ) )
			{
				if ( g_TF_PR->GetPlayerConnectionState( playerIndex ) != MM_CONNECTED )
					continue;

				// No need to add local player
				if ( engine->GetLocalPlayer() == playerIndex )
					continue;

				player_info_t pi;
				if ( !engine->GetPlayerInfo( playerIndex, &pi ) )
					continue;

				// Don't add bots
				if ( pi.fakeplayer )
					continue;

				int nTeam = g_PR->GetTeam( playerIndex );

				KeyValues *pKeyValues = new KeyValues( "data" );
				pKeyValues->SetInt( "index", playerIndex );
				pKeyValues->SetString( "name", g_TF_PR->GetPlayerName( playerIndex ) );
				pKeyValues->SetInt( "score", g_TF_PR->GetTotalScore( playerIndex ) );

				// Update their avatar
				if ( steamapicontext->SteamFriends() && steamapicontext->SteamUtils() )
				{
					if ( pi.friendsID )
					{
						CSteamID steamIDForPlayer( pi.friendsID, 1, GetUniverse(), k_EAccountTypeIndividual );

						// See if we already have that avatar in our list
						int iMapIndex = m_mapAvatarsToImageList.Find( steamIDForPlayer );
						int iImageIndex;
						if ( iMapIndex == m_mapAvatarsToImageList.InvalidIndex() )
						{
							CAvatarImage *pImage = new CAvatarImage();
							pImage->SetAvatarSteamID( steamIDForPlayer );
							pImage->SetAvatarSize( 32, 32 );	// Deliberately non scaling
							iImageIndex = m_pImageList->AddImage( pImage );

							m_mapAvatarsToImageList.Insert( steamIDForPlayer, iImageIndex );
						}
						else
						{
							iImageIndex = m_mapAvatarsToImageList[iMapIndex];
						}

						pKeyValues->SetInt( "avatar", iImageIndex );

						CAvatarImage *pAvIm = ( CAvatarImage * ) m_pImageList->GetImage( iImageIndex );
						pAvIm->UpdateFriendStatus();
					}
				}

				// The medal column is just a place holder for the images that are displayed later
				pKeyValues->SetInt( "medal", 0 );
				pKeyValues->SetString( "time", FormatSeconds( gpGlobals->curtime - g_TF_PR->GetConnectTime( playerIndex ) ) );

				Color clr = g_PR->GetTeamColor( nTeam );
				pKeyValues->SetColor( "cellcolor", clr );

				m_pPlayerList->AddItem( pKeyValues, 0, false, false );

				pKeyValues->deleteThis();
			}
		}
	}

	// refresh player status info
	RefreshPlayerStatus();

	m_pPlayerList->InvalidateLayout( true );

	UpdateBadgePanels();

	m_pPlayerList->SetSingleSelectedItem( m_pPlayerList->GetItemIDFromRow( 0 ) );
	OnItemSelected();
}

//-----------------------------------------------------------------------------
// Purpose: walks the players and sets their info display in the list
//-----------------------------------------------------------------------------
void CMutePlayerDialog::RefreshPlayerStatus()
{
	for ( int i = 0; i <= m_pPlayerList->GetItemCount(); i++ )
	{
		KeyValues *data = m_pPlayerList->GetItem( i );
		if ( !data )
			continue;

		// assemble properties
		int playerIndex = data->GetInt( "index" );
		player_info_t pi;

		if ( !engine->GetPlayerInfo( playerIndex, &pi ) )
		{
			// disconnected
			data->SetString( "status", "#TF_Disconnected" );
			continue;
		}

		data->SetString( "name", UTIL_GetFilteredPlayerName( playerIndex, pi.name ) );

		if ( GetClientVoiceMgr() && GetClientVoiceMgr()->IsPlayerBlocked( playerIndex ) )
		{
			data->SetString( "status", "#TF_Muted" );
		}
		else
		{
			data->SetString( "status", "" );
		}
	}
	m_pPlayerList->RereadAllItems();
}

//-----------------------------------------------------------------------------
// Purpose: Handles the AddFriend command
//-----------------------------------------------------------------------------
void CMutePlayerDialog::OnCommand( const char *command )
{
	if ( !stricmp( command, "Mute" ) )
	{
		ToggleMuteStateOfSelectedUser();
	}
	else
	{
		BaseClass::OnCommand( command );
	}
}

//-----------------------------------------------------------------------------
// Purpose: toggles whether a user is muted or not
//-----------------------------------------------------------------------------
void CMutePlayerDialog::ToggleMuteStateOfSelectedUser()
{
	if ( !GetClientVoiceMgr() )
		return;

	for ( int iSelectedItem = 0; iSelectedItem < m_pPlayerList->GetSelectedItemsCount(); iSelectedItem++ )
	{
		KeyValues *data = m_pPlayerList->GetItem( m_pPlayerList->GetSelectedItem( iSelectedItem ) );
		if ( !data )
			return;
		int playerIndex = data->GetInt( "index" );
		Assert( playerIndex );

		if ( GetClientVoiceMgr()->IsPlayerBlocked( playerIndex ) )
		{
			GetClientVoiceMgr()->SetPlayerBlockedState( playerIndex, false );
		}
		else
		{
			GetClientVoiceMgr()->SetPlayerBlockedState( playerIndex, true );
		}
	}

	RefreshPlayerStatus();
	OnItemSelected();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMutePlayerDialog::OnItemSelected()
{
	// make sure the data is up-to-date
	RefreshPlayerStatus();

	// set the button state based on the selected item
	bool bMuteButtonEnabled = false;
	if ( m_pPlayerList->GetSelectedItemsCount() > 0 )
	{
		KeyValues *data = m_pPlayerList->GetItem( m_pPlayerList->GetSelectedItem( 0 ) );

		player_info_t pi;

		int iLocalPlayer = engine->GetLocalPlayer();

		int iPlayerIndex = data->GetInt( "index" );
		bool isValidPlayer = engine->GetPlayerInfo( iPlayerIndex, &pi );

		// make sure the player is not a bot, or the user 
		// Matt - changed this check to see if player indices match, instead of using friends ID
		if ( iPlayerIndex == iLocalPlayer ) // || pi.friendsID == g_pFriendsUser->GetFriendsID() )
		{
			// invalid player, 
			isValidPlayer = false;
		}

		if ( data && isValidPlayer && GetClientVoiceMgr() && GetClientVoiceMgr()->IsPlayerBlocked( data->GetInt( "index" ) ) )
		{
			m_pMuteButton->SetText( "#GameUI_UnmuteIngameVoice" );
		}
		else
		{
			m_pMuteButton->SetText( "#GameUI_MuteIngameVoice" );
		}

		if ( GetClientVoiceMgr() && isValidPlayer )
		{
			bMuteButtonEnabled = true;
		}
	}
	else
	{
		m_pMuteButton->SetText( "#GameUI_MuteIngameVoice" );
	}

	m_pMuteButton->SetEnabled( bMuteButtonEnabled );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMutePlayerDialog::UpdateBadgePanels()
{
	int iNumPanels = 0;

	const IMatchGroupDescription *pMatchDesc = TFGameRules() ? GetMatchGroupDescription( TFGameRules()->GetCurrentMatchGroup() ) : NULL;
	if ( pMatchDesc && m_pPlayerList )
	{
		if ( TFGameRules()->IsMatchTypeCasual() )
		{
			int parentTall = m_pPlayerList->GetTall();
			CTFBadgePanel *pPanel = NULL;

			for ( int i = 0; i < m_pPlayerList->GetItemCount(); i++ )
			{
				KeyValues *pKeyValues = m_pPlayerList->GetItem( m_pPlayerList->GetItemIDFromRow( i ) );
				if ( !pKeyValues )
					continue;

				int iPlayerIndex = pKeyValues->GetInt( "index" );
				const CSteamID steamID = GetSteamIDForPlayerIndex( iPlayerIndex );
				if ( steamID.IsValid() )
				{
					if ( iNumPanels >= m_pBadgePanels.Count() )
					{
						pPanel = new CTFBadgePanel( this, "BadgePanel" );
						pPanel->MakeReadyForUse();
						pPanel->SetVisible( true );
						pPanel->SetZPos( 9999 );
						m_pBadgePanels.AddToTail( pPanel );
					}
					else
					{
						pPanel = m_pBadgePanels[iNumPanels];
					}

					int x, y, wide, tall;
					m_pPlayerList->GetCellBounds( i, 0, x, y, wide, tall );

					if ( y + tall > parentTall )
						continue;

					if ( !pPanel->IsVisible() )
					{
						pPanel->SetVisible( true );
					}

					int xParent, yParent;
					m_pPlayerList->GetPos( xParent, yParent );

					int nPanelXPos, nPanelYPos, nPanelWide, nPanelTall;
					pPanel->GetBounds( nPanelXPos, nPanelYPos, nPanelWide, nPanelTall );

					if ( ( nPanelXPos != xParent + x )
						|| ( nPanelYPos != yParent + y )
						|| ( nPanelWide != wide )
						|| ( nPanelTall != tall ) )
					{
						pPanel->SetBounds( xParent + x, yParent + y, wide, tall );
						pPanel->InvalidateLayout( true, true );
					}

					pPanel->SetupBadge( pMatchDesc, steamID );
					iNumPanels++;
				}
			}
		}
	}

	// hide any unused images
	for ( int i = iNumPanels; i < m_pBadgePanels.Count(); i++ )
	{
		if ( m_pBadgePanels[i]->IsVisible() )
		{
			m_pBadgePanels[i]->SetVisible( false );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMutePlayerDialog::OnThink()
{
	if ( IsVisible() )
	{
		UpdateBadgePanels();
	}

	BaseClass::OnThink();
}