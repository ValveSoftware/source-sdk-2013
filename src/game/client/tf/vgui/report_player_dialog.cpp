//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "report_player_dialog.h"
#include "gc_clientsystem.h"
#include "ienginevgui.h"
#include "c_tf_playerresource.h"
#include "c_tf_player.h"
#include "vgui_avatarimage.h"
#include <vgui_controls/ImageList.h>
#include "tf_gamerules.h"

using namespace vgui;

extern const char *FormatSeconds( int seconds );

// in seconds
static const float MIN_REPORT_INTERVAL = 300.f;

struct ReportedPlayer_t
{
	CSteamID steamID;
	float flReportedTime;
};
CUtlVector< ReportedPlayer_t > vecReportedPlayers;

bool CanReportPlayer( CSteamID steamID, bool bVerbose )
{
	bool bCanReport = true;
	for (int i = 0; i < vecReportedPlayers.Count(); ++i)
	{
		if ( vecReportedPlayers[i].steamID == steamID )
		{
			float flTimeSinceLastReported = gpGlobals->curtime - vecReportedPlayers[i].flReportedTime;
			bCanReport = flTimeSinceLastReported >= MIN_REPORT_INTERVAL;
			if ( !bCanReport && bVerbose )
			{
				float flCooldownTime = MIN_REPORT_INTERVAL - flTimeSinceLastReported;
				ConMsg( "Already reported this player. You can report this player again in %.2f seconds\n", flCooldownTime );
			}
			break;
		}
	}

	return bCanReport;
}

bool ReportPlayerAccount( CSteamID steamID, int nReason )
{
	if ( !steamID.IsValid() )
	{
		Warning( "Reporting an invalid steam ID\n" );
		return false;
	}

	if ( !CanReportPlayer( steamID, true ) )
	{
		return false;
	}

	if ( nReason <= CMsgGC_ReportPlayer_EReason_kReason_INVALID || nReason >= CMsgGC_ReportPlayer_EReason_kReason_COUNT )
	{
		Assert( !"Invalid report reason" );
		return false;
	}

	GCSDK::CProtoBufMsg< CMsgGC_ReportPlayer > msg( k_EMsgGC_ReportPlayer );
	msg.Body().set_account_id_target( steamID.GetAccountID() );
	msg.Body().set_reason( (CMsgGC_ReportPlayer_EReason)nReason );
	GCClientSystem()->BSendMessage( msg );
	ConMsg( "Report sent. Thank you.\n" );

	ReportedPlayer_t reportedPlayer;
	reportedPlayer.steamID = steamID;
	reportedPlayer.flReportedTime = gpGlobals->curtime;
	vecReportedPlayers.AddToTail( reportedPlayer );
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CReportPlayerDialog::CReportPlayerDialog( vgui::Panel *parent ) : BaseClass( parent, "ReportPlayerDialog" )
{
	SetSize( 320, 270 );
	SetTitle( "#GameUI_ReportPlayerCaps", true );

	m_pReportButton = new Button( this, "ReportButton", "" );
	m_pPlayerList = new ListPanel( this, "PlayerList" );
	m_pPlayerList->SetEmptyListText( "#GameUI_NoOtherPlayersInGame" );
	m_pReasonBox = new ComboBox( this, "ReasonBox", 5, false );

	m_mapAvatarsToImageList.SetLessFunc( DefLessFunc( CSteamID ) );
	m_mapAvatarsToImageList.RemoveAll();
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CReportPlayerDialog::~CReportPlayerDialog()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CReportPlayerDialog::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );
	
	LoadControlSettings( "Resource/ReportPlayerDialog.res" );

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
		m_nExtraSpace = m_pPlayerList->GetWide() - m_iMedalWidth - m_iAvatarWidth - m_iNameWidth - m_iScoreWidth - m_iTimeWidth;

		m_pPlayerList->AddColumnHeader( 2, "name", "#TF_Scoreboard_Name", m_iNameWidth + m_nExtraSpace );
		m_pPlayerList->AddColumnHeader( 3, "score", "#TF_Scoreboard_Score", m_iScoreWidth );
		m_pPlayerList->AddColumnHeader( 4, "time", "#TF_Connected", m_iTimeWidth );
		
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
void CReportPlayerDialog::Activate()
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

				// Already reported
				CSteamID steamID( pi.friendsID, universe, k_EAccountTypeIndividual );
				if ( !CanReportPlayer( steamID, false ) )
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

				if ( pi.fakeplayer )
				{
					pKeyValues->SetString( "time", "#TF_Scoreboard_Bot" );
				}
				else
				{
					pKeyValues->SetString( "time", FormatSeconds( gpGlobals->curtime - g_TF_PR->GetConnectTime( playerIndex ) ) );
				}

				Color clr = g_PR->GetTeamColor( nTeam );
				pKeyValues->SetColor( "cellcolor", clr );

				m_pPlayerList->AddItem( pKeyValues, 0, false, false );

				pKeyValues->deleteThis();
			}
		}
	}

	m_pReasonBox->RemoveAll();
	KeyValues *pKeyValues = new KeyValues( "data" );
	SetDialogVariable( "combo_label", g_pVGuiLocalize->Find( "#GameUI_ReportPlayerReason" ) );
	pKeyValues->SetInt( "reason", 0 );
	m_pReasonBox->AddItem( g_pVGuiLocalize->Find( "GameUI_ReportPlayer_Choose" ), pKeyValues );
	pKeyValues->SetInt( "reason", 1 );
	m_pReasonBox->AddItem( g_pVGuiLocalize->Find( "GameUI_ReportPlayer_Cheating" ), pKeyValues );
	pKeyValues->SetInt( "reason", 2 );
	m_pReasonBox->AddItem( g_pVGuiLocalize->Find( "GameUI_ReportPlayer_Idle" ), pKeyValues );
	pKeyValues->SetInt( "reason", 3 );
	m_pReasonBox->AddItem( g_pVGuiLocalize->Find( "GameUI_ReportPlayer_Harassment" ), pKeyValues );
	pKeyValues->SetInt( "reason", 4 );
	m_pReasonBox->AddItem( g_pVGuiLocalize->Find( "GameUI_ReportPlayer_Griefing" ), pKeyValues );
	m_pReasonBox->SilentActivateItemByRow( 0 );
	pKeyValues->deleteThis();

	m_pPlayerList->InvalidateLayout( true );

	UpdateBadgePanels();
	
	m_pPlayerList->SetSingleSelectedItem( m_pPlayerList->GetItemIDFromRow( 0 ) );
	OnItemSelected();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CReportPlayerDialog::IsValidPlayerSelected()
{
	bool bIsValidPlayer = false;

	if ( m_pPlayerList->GetSelectedItemsCount() > 0 )
	{
		KeyValues *pData = m_pPlayerList->GetItem( m_pPlayerList->GetSelectedItem( 0 ) );
		player_info_t pi;
		bIsValidPlayer = engine->GetPlayerInfo( pData->GetInt( "index" ), &pi );
#ifdef _DEBUG
		bIsValidPlayer = bIsValidPlayer && pData->GetInt( "index" ) != engine->GetLocalPlayer();
#else
		bIsValidPlayer = bIsValidPlayer && !pi.fakeplayer && pData->GetInt( "index" ) != engine->GetLocalPlayer();
#endif
	}

	return bIsValidPlayer;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CReportPlayerDialog::OnCommand( const char *command )
{
	if ( !stricmp( command, "Report" ) )
	{
		ReportPlayer();
	}
	else
	{
		BaseClass::OnCommand( command );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CReportPlayerDialog::ReportPlayer()
{
	for ( int iSelectedItem = 0; iSelectedItem < m_pPlayerList->GetSelectedItemsCount(); iSelectedItem++ )
	{
		KeyValues *pPlayerData = m_pPlayerList->GetItem( m_pPlayerList->GetSelectedItem( iSelectedItem ) );
		if ( !pPlayerData )
			return;

		Assert( pPlayerData->GetInt( "index" ) );

		// 	INVALID = 0;
		// 	CHEATING = 1;
		// 	IDLE = 2;
		// 	HARASSMENT = 3;
		//	GRIEFING = 4;

		player_info_t pi;
		if ( !engine->GetPlayerInfo( pPlayerData->GetInt( "index" ), &pi ) )
			return;

		CSteamID steamID( pi.friendsID, GetUniverse(), k_EAccountTypeIndividual );
		KeyValues *pReasonData = m_pReasonBox->GetActiveItemUserData();
		int nReason = ( pReasonData ) ? pReasonData->GetInt( "reason", 0 ) : 0;
		ReportPlayerAccount( steamID, nReason );
		Close();
		return;
	}

	OnItemSelected();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CReportPlayerDialog::OnItemSelected()
{
	bool bReportButtonEnabled = IsValidPlayerSelected();
	if ( !bReportButtonEnabled )
	{
		m_pReportButton->SetText( "#GameUI_ReportPlayer" );
	}

	// Reason selected?
	KeyValues *pUserData = m_pReasonBox->GetActiveItemUserData();
	bReportButtonEnabled = bReportButtonEnabled && pUserData && pUserData->GetInt( "reason", 0 ) > 0;
	m_pReportButton->SetEnabled( bReportButtonEnabled );
}

//-----------------------------------------------------------------------------
// Purpose: Called when text changes in combo box
//-----------------------------------------------------------------------------
void CReportPlayerDialog::OnTextChanged( KeyValues *data )
{
	Panel *pPanel = reinterpret_cast< vgui::Panel* >( data->GetPtr( "panel" ) );
	vgui::ComboBox *pComboBox = dynamic_cast< vgui::ComboBox* >( pPanel );
	if ( pComboBox && pComboBox == m_pReasonBox )
	{
		bool bReportButtonEnabled = IsValidPlayerSelected();
		KeyValues *pReasonData = m_pReasonBox->GetActiveItemUserData();
		bReportButtonEnabled = bReportButtonEnabled && pReasonData && pReasonData->GetInt( "reason", 0 ) > 0;
		m_pReportButton->SetEnabled( bReportButtonEnabled );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CReportPlayerDialog::UpdateBadgePanels()
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
void CReportPlayerDialog::OnThink()
{
	if ( IsVisible() )
	{
		UpdateBadgePanels();
	}

	BaseClass::OnThink();
}