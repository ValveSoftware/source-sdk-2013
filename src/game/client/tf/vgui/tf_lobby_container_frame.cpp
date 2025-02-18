//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#include "cbase.h"

#include "ienginevgui.h"
#include "vgui/IVGui.h"
#include "vgui/IInput.h"
#include "vgui_controls/MenuItem.h"
#include "vgui_controls/PropertySheet.h"

#include "tf_party.h"
#include "tf_partyclient.h"
#include "tf_matchcriteria.h"
#include "tf_lobbypanel.h"
#include "tf_pvp_rank_panel.h"

#include "tf_lobby_container_frame.h"
#include "tf_controls.h"

#include "tf_item_inventory.h"
#include "tf_lobbypanel.h"
#include "tf_hud_mainmenuoverride.h"
#include "tf_matchmaking_dashboard.h"
#include "tf_ping_panel.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

bool BIsPartyInUIState()
{
	if ( !GCClientSystem()->BConnectedtoGC() )
		return false;
	CTFParty *pParty = GTFGCClientSystem()->GetParty();
	return ( pParty == NULL || pParty->GetState() == CSOTFParty_State_UI );
}

CSteamID SteamIDFromDecimalString( const char *pszUint64InDecimal )
{
	uint64 ulSteamID = 0;
	if ( sscanf( pszUint64InDecimal, "%llu", &ulSteamID ) )
	{
		return CSteamID( ulSteamID );
	}
	else
	{
		Assert( false );
		return CSteamID();
	}
}

CSteamID SteamIDFromDecimalString( const char *pszUint64InDecimal )
{
	uint64 ulSteamID = 0;
	if ( sscanf( pszUint64InDecimal, "%llu", &ulSteamID ) )
	{
		return CSteamID( ulSteamID );
	}
	else
	{
		Assert( false );
		return CSteamID();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBaseLobbyContainerFrame::CBaseLobbyContainerFrame( const char* pszPanelName ) 
	: vgui::PropertyDialog( NULL, pszPanelName )
	, m_bNextButtonEnabled( false )
	, m_pContextMenu( NULL )
{
	vgui::VPANEL gameuiPanel = enginevgui->GetPanel( PANEL_GAMEUIDLL );
	SetParent( gameuiPanel );

	SetMoveable( false );
	SetSizeable( false );

	vgui::HScheme scheme = vgui::scheme()->LoadSchemeFromFileEx( enginevgui->GetPanel( PANEL_CLIENTDLL ), "resource/ClientScheme.res", "ClientScheme");
	SetScheme(scheme);
	SetProportional( true );

	ListenForGameEvent( "gameui_hidden" );

	ListenForGameEvent( "lobby_updated" );
	ListenForGameEvent( "party_updated" );
	ListenForGameEvent( "client_beginconnect" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBaseLobbyContainerFrame::~CBaseLobbyContainerFrame( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseLobbyContainerFrame::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	// load control settings...
	LoadControlSettings( GetResFile() );

	m_pStartPartyButton = dynamic_cast<vgui::Button *>(FindChildByName( "StartPartyButton", true )); Assert( m_pStartPartyButton );
	m_pBackButton = dynamic_cast<vgui::Button *>( FindChildByName( "BackButton", true ) ); Assert( m_pBackButton );
	m_pNextButton = dynamic_cast<vgui::Button *>( FindChildByName( "NextButton", true ) ); Assert( m_pNextButton );

	SetOKButtonVisible(false);
	SetCancelButtonVisible(false);
}

//-----------------------------------------------------------------------------
void CBaseLobbyContainerFrame::ShowPanel(bool bShow)
{
	tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s", __FUNCTION__ );

	m_pContents->SetControlVisible( "PartyActiveGroupBox", false );

	// Make sure we're signed on
	if ( bShow )
	{
		if ( GetPropertySheet()->GetActivePage() != m_pContents )
		{
			GetPropertySheet()->SetActivePage( m_pContents );
		}
		else
		{
			// VGUI doesn't tell the starting active page that it's active, so we post a pageshow to it
			vgui::ivgui()->PostMessage( m_pContents->GetVPanel(), new KeyValues("PageShow"), GetPropertySheet()->GetVPanel() );
		}

		Activate();

		// I don't know why, I don't want to know why, I shouldn't
		// have to wonder why, but for whatever reason this stupid
		// panel isn't laying out correctly unless we do this terribleness
		InvalidateLayout( true );
		m_pContents->InvalidateLayout( true, true );

		GTFPartyClient()->MutLocalPlayerCriteria().SetSquadSurplus( false );
		WriteControls();
		m_pContents->UpdateControls();

		Panel* pPvPRankPanel = FindChildByName( "RankPanel", true );
		if ( pPvPRankPanel )
		{
			pPvPRankPanel->OnCommand( "update_base_state" );
			pPvPRankPanel->OnCommand( "begin_xp_lerp" );
		}
	}
	else
	{
		if ( m_hPingPanel )
		{
			m_hPingPanel->MarkForDeletion();
		}
	}

	OnCommand( "leave_party" );

	SetVisible( bShow );
	m_pContents->SetVisible( bShow );
}

//-----------------------------------------------------------------------------
void CBaseLobbyContainerFrame::SetNextButtonEnabled( bool bValue )
{
	m_bNextButtonEnabled = bValue;
}


//-----------------------------------------------------------------------------
void CBaseLobbyContainerFrame::OnThink()
{
	BaseClass::OnThink();

	WriteControls();
}


//-----------------------------------------------------------------------------
void CBaseLobbyContainerFrame::FireGameEvent( IGameEvent *event )
{
	if ( !CanHandleCurrentMatchGroup() )
		return;

	const char *pszEventname = event->GetName();
	if ( !Q_stricmp( pszEventname, "lobby_updated" ) || !Q_stricmp( pszEventname, "party_updated" ) )
	{
		WriteControls();
		return;
	}

	// Bail when we connect to any server
	if ( !Q_stricmp( pszEventname, "client_beginconnect" ) )
	{
		ShowPanel( false );
		return;
	}
}

//-----------------------------------------------------------------------------
void CBaseLobbyContainerFrame::StartSearch( void )
{
	Assert( !GTFPartyClient()->BInQueue() );

	// Is anyone banned from matchmaking?
	RTime32 rtimeExpire = 0;
	if ( m_pContents->IsAnyoneBanned( rtimeExpire ) )
	{
		CRTime timeExpire( rtimeExpire );
		timeExpire.SetToGMT( false );
		char out_buf[k_RTimeRenderBufferSize];
		wchar_t wszExpire[512];
		wchar_t wszLocalized[512];
		g_pVGuiLocalize->ConvertANSIToUnicode( timeExpire.Render( out_buf ), wszExpire, sizeof( wszExpire ) );
		g_pVGuiLocalize->ConstructString_safe( wszLocalized, g_pVGuiLocalize->Find( "#TF_Matchmaking_Banned" ), 1, wszExpire );
		ShowMessageBox( "#TF_Matchmaking_Title", wszLocalized, "#GameUI_OK" );
		return;
	}

	if ( VerifyPartyAuthorization() )
	{
		GTFPartyClient()->RequestQueueForMatch();
	}
}

//-----------------------------------------------------------------------------
void CBaseLobbyContainerFrame::OnCommand( const char *command )
{
	if ( FStrEq( command, "options" ) )
	{
		OpenOptionsContextMenu();
	}
	else if ( FStrEq( command, "back" ) )
	{
		if ( !GCClientSystem()->BConnectedtoGC() || !GTFPartyClient()->BIsPartyLeader() )
		{
			// TODO(Universal Parties): --v
			// TODO: Remove this when we have the dashboard everywhere.
			//		 Well...this entire panel should be gone.
			GTFGCClientSystem()->EndModalMM();
			// And hide us
			ShowPanel( false );
			return;
		}

		HandleBackPressed();
	}
	else if ( FStrEq( command, "leave_party" ) )
	{
		m_pStartPartyButton->SetVisible( true );
		SetControlVisible( "PlayWithFriendsExplanation", true );
		m_pContents->SetControlVisible( "PartyActiveGroupBox", false );
	}
	else if ( FStrEq( command, "start_party" ) )
	{
		m_pStartPartyButton->SetVisible( false );
		SetControlVisible( "PlayWithFriendsExplanation", false );
		m_pContents->SetControlVisible( "PartyActiveGroupBox", true );

		Assert( steamapicontext );
	}
	else
	{

		// What other commands are there?
		Assert( false );
	}
}

//-----------------------------------------------------------------------------
void CBaseLobbyContainerFrame::PerformLayout( void )
{
	if ( GetVParent() )
	{
		int w,h;
		vgui::ipanel()->GetSize( GetVParent(), w, h );
		SetBounds(0,0,w,h);
	}

	BaseClass::PerformLayout();
}

//-----------------------------------------------------------------------------
static void LeaveSearch( bool bConfirmed, void *pContext )
{
	if ( bConfirmed )
	{
		GTFPartyClient()->CancelQueueRequest();
	}
}

//-----------------------------------------------------------------------------
void CBaseLobbyContainerFrame::OnKeyCodeTyped(vgui::KeyCode code)
{
	if ( code == KEY_ESCAPE )
	{
		if ( m_pContents->IsPartyActiveGroupBoxVisible() )
		{
			ShowConfirmDialog( "#TF_MM_LeaveParty_Title", "#TF_MM_LeaveParty_Confirm",
								"#TF_Coach_Yes", "#TF_Coach_No",
								&CBaseLobbyContainerFrame::LeaveLobbyPanel );
			return;
		}
		else if ( GTFPartyClient()->BInQueue() )
		{
			ShowConfirmDialog( "#TF_MM_LeaveQueue_Title", "#TF_MM_LeaveQueue_Confirm",
								"#TF_Coach_Yes", "#TF_Coach_No",
								&LeaveSearch );
			return;
		}

		OnCommand( "back" );
		return;
	}

	BaseClass::OnKeyCodeTyped( code );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseLobbyContainerFrame::OnKeyCodePressed(vgui::KeyCode code)
{
	ButtonCode_t nButtonCode = GetBaseButtonCode( code );

	if ( nButtonCode == KEY_XBUTTON_B || nButtonCode == STEAMCONTROLLER_B || nButtonCode == STEAMCONTROLLER_START )
	{
		OnCommand( "back" );
		return;
	}
	else if ( nButtonCode == KEY_XBUTTON_X )
	{
		m_pContents->ToggleJoinLateCheckButton();
	}

	BaseClass::OnKeyCodePressed( code );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseLobbyContainerFrame::WriteControls()
{
	tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s", __FUNCTION__ );

	if ( !IsVisible() )
		return;

	// Make sure we want to be in matchmaking.  (If we don't, the frame should hide us pretty quickly.)
	// We might get an event or something right at the transition point occasionally when the UI should
	// not be visible
	if ( !GTFGCClientSystem()->BUserInModalMMUI() )
		{ return; }

	bool bNoGC = false;
	if ( !GCClientSystem()->BConnectedtoGC() || GTFGCClientSystem()->BHaveLiveMatch() ||
	     !GTFGCClientSystem()->BHealthyGCConnection() )
	{
		bNoGC = true;
	}

	SetControlVisible( "PlayWithFriendsExplanation", !bNoGC && ShouldShowPartyButton(), true );
	SetControlVisible( "RankPanel", !bNoGC, true );
	SetControlVisible( "NoGCGroupBox", bNoGC, true );

	GetPropertySheet()->SetTabWidth( -1 );

	// Check if we already have a party, then make sure and show it
	if ( !m_pStartPartyButton->IsVisible() && m_pContents->NumPlayersInParty() > 1 )
	{
		m_pContents->SetControlVisible( "PartyActiveGroupBox", true );
	}

	// Show/hide start party button as appropriate
	bool bShowPartyButton = ShouldShowPartyButton();

	m_pStartPartyButton->SetVisible( bShowPartyButton && !bNoGC );

	// Check for matchmaking bans and display time remaining if we're banned
	if ( steamapicontext == NULL || steamapicontext->SteamUser() == NULL )
		return;

	CEconGameAccountClient *pGameAccountClient = NULL;
	if ( InventoryManager() && TFInventoryManager()->GetLocalTFInventory() && TFInventoryManager()->GetLocalTFInventory()->GetSOC() )
	{
		pGameAccountClient = TFInventoryManager()->GetLocalTFInventory()->GetSOC()->GetSingleton<CEconGameAccountClient>();
	}

	const IMatchGroupDescription *pMatchDesc = GetMatchGroupDescription( m_pContents->GetMatchGroup() );
	EMMPenaltyPool ePenaltyPool = pMatchDesc ? pMatchDesc->m_params.m_ePenaltyPool : eMMPenaltyPool_Invalid;

	CRTime timeExpire = CRTime::RTime32TimeCur();
	int nDuration = -1;
	bool bBanned = false;

	if ( pGameAccountClient && ePenaltyPool != eMMPenaltyPool_Invalid )
	{
		switch ( ePenaltyPool )
		{
		case eMMPenaltyPool_Casual:
			timeExpire = pGameAccountClient->Obj().matchmaking_casual_ban_expiration();
			nDuration = pGameAccountClient->Obj().matchmaking_casual_ban_last_duration();
			bBanned = timeExpire > CRTime::RTime32TimeCur();
			break;
		case eMMPenaltyPool_Ranked:
			timeExpire = pGameAccountClient->Obj().matchmaking_ranked_ban_expiration();
			nDuration = pGameAccountClient->Obj().matchmaking_ranked_ban_last_duration();
			bBanned = timeExpire > CRTime::RTime32TimeCur();
			break;
		default: Assert( false );
		}

		SetControlVisible( "MatchmakingBanPanel", bBanned );

		if ( bBanned )
		{
			CExLabel *pBanLabel = FindControl<CExLabel>( "MatchmakingBanDurationLabel", true );

			if ( pBanLabel )
			{
				timeExpire.SetToGMT( false );

				CRTime rtNow = CRTime::RTime32TimeCur();

				int nSecondsRemaining = timeExpire.GetRTime32() - rtNow.GetRTime32();

				if ( nSecondsRemaining >= 0 )
				{

					const int nDaysForLongBan = 2;

					int nDaysRemaining = nSecondsRemaining / 86400;
					int nHoursRemaining = nSecondsRemaining / 3600;
					int nMinutesRemaining = ( nSecondsRemaining % 3600 ) / 60;

					int nDurationDays = nDuration / 86400;
					int nDurationHours = nDuration / 3600;
					int nDurationMinutes = ( nDuration % 3600 ) / 60;

					// Want the remainder hours if we're going to display 'days' remaining
					if ( nDurationDays >= nDaysForLongBan )
					{
						nDurationHours = ( nDuration % ( 86400 * nDurationDays ) ) / 3600;

						if ( nDaysRemaining >= nDaysForLongBan )
						{
							nHoursRemaining = ( nSecondsRemaining % ( 86400 * nDaysRemaining ) ) / 3600;
						}
					}

					wchar_t wszDaysRemaining[16];
					wchar_t wszHoursRemaining[16];
					wchar_t wszMinutesRemaining[16];
					wchar_t wszDurationDays[16];
					wchar_t wszDurationHours[16];
					wchar_t wszDurationMinutes[16];

					_snwprintf( wszDaysRemaining, ARRAYSIZE( wszDaysRemaining ), L"%d", nDaysRemaining );
					_snwprintf( wszHoursRemaining, ARRAYSIZE( wszHoursRemaining ), L"%d", nHoursRemaining );
					_snwprintf( wszMinutesRemaining, ARRAYSIZE( wszMinutesRemaining ), L"%d", nMinutesRemaining );
					_snwprintf( wszDurationDays, ARRAYSIZE( wszDurationDays ), L"%d", nDurationDays );
					_snwprintf( wszDurationHours, ARRAYSIZE( wszDurationHours ), L"%d", nDurationHours );
					_snwprintf( wszDurationMinutes, ARRAYSIZE( wszDurationMinutes ), L"%d", nDurationMinutes );

					wchar_t wszLocalized[512];

					// Short ban (less than "nDaysForLongBan" days and thus less than that remaining)
					if ( nDurationDays < nDaysForLongBan )
					{
						// Less than an hour ban
						if ( nDurationHours < 1 )
						{
							g_pVGuiLocalize->ConstructString_safe( wszLocalized, g_pVGuiLocalize->Find( "#TF_Matchmaking_Ban_Duration_Remaining_Short" ), 2, wszDurationMinutes, wszMinutesRemaining );
						}
						else
						{
							g_pVGuiLocalize->ConstructString_safe( wszLocalized, g_pVGuiLocalize->Find( "#TF_Matchmaking_Ban_Duration_Remaining" ), 4, wszDurationHours, wszDurationMinutes, wszHoursRemaining, wszMinutesRemaining );
						}
					}
					// Long ban (at least "nDaysForLongBan" days) but less than that remaining
					else if ( nDaysRemaining < nDaysForLongBan )
					{
						g_pVGuiLocalize->ConstructString_safe( wszLocalized, g_pVGuiLocalize->Find( "#TF_Matchmaking_Ban_Duration_Remaining_Long_Penalty_Short_Duration" ), 4, wszDurationDays, wszDurationHours, wszHoursRemaining, wszMinutesRemaining );
					}
					// Long ban and at least that long remaining)
					else
					{
						g_pVGuiLocalize->ConstructString_safe( wszLocalized, g_pVGuiLocalize->Find( "TF_Matchmaking_Ban_Duration_Remaining_Long_Penalty" ), 4, wszDurationDays, wszDurationHours, wszDaysRemaining, wszHoursRemaining );
					}
					pBanLabel->SetText( wszLocalized );
				}
				else
				{
					pBanLabel->SetText( "#TF_Matchmaking_Ban_Duration_Remaining_Shortly" );
				}
			}
		}
	}

	m_pNextButton->SetEnabled( m_bNextButtonEnabled && !bBanned && !bNoGC );
}

//-----------------------------------------------------------------------------
// Purpose: Handle cases not handled by our derived classes
//-----------------------------------------------------------------------------
void CBaseLobbyContainerFrame::HandleBackPressed()
{
	GTFGCClientSystem()->EndModalMM();
	ShowPanel( false );
}

//-----------------------------------------------------------------------------
bool CBaseLobbyContainerFrame::ShouldShowPartyButton() const
{
	return ( !m_pContents->IsPartyActiveGroupBoxVisible() &&
	         GTFPartyClient()->BInQueue() &&
	         GCClientSystem()->BConnectedtoGC() &&
	         GTFPartyClient()->BIsPartyLeader() );
}


//-----------------------------------------------------------------------------
void CBaseLobbyContainerFrame::OpenOptionsContextMenu()
{
	if ( m_pContextMenu )
		delete m_pContextMenu;

	m_pContextMenu = new Menu( this, "ContextMenu" );
	MenuBuilder contextMenuBuilder( m_pContextMenu, this );
	const char *pszContextMenuBorder = "NotificationDefault";
	const char *pszContextMenuFont = "HudFontMediumSecondary";
	m_pContextMenu->SetBorder( scheme()->GetIScheme( GetScheme() )->GetBorder( pszContextMenuBorder ) );
	m_pContextMenu->SetFont( scheme()->GetIScheme( GetScheme() )->GetFont( pszContextMenuFont, IsProportional() ) );

	contextMenuBuilder.AddMenuItem( "#TF_LobbyContainer_Ping", new KeyValues( "Context_Ping" ), "ping" );
	contextMenuBuilder.AddMenuItem( "#TF_LobbyContainer_Help", "show_explanations", "help" );

	m_pContextMenu->SetVisible(true);
	m_pContextMenu->AddActionSignalTarget(this);

	m_pContextMenu->MakeReadyForUse();

	Panel* pOptionsButton = FindChildByName( "OptionsButton" );

	if ( !pOptionsButton )
	{
		// Position to the cursor's position
		int nX, nY;
		g_pVGuiInput->GetCursorPosition( nX, nY );
		m_pContextMenu->SetPos( nX - 1, nY - 1 );
	}
	else
	{
		int nOptionsX = pOptionsButton->GetXPos();
		int nX = Min( nOptionsX, GetWide() - m_pContextMenu->GetWide() );
		int nY = pOptionsButton->GetYPos() + pOptionsButton->GetTall();
		m_pContextMenu->SetPos( nX - 1, nY - 1 );
	}
}

//-----------------------------------------------------------------------------
bool CBaseLobbyContainerFrame::CanHandleCurrentMatchGroup() const
{
	return CanHandleMatchGroup( GTFPartyClient()->GetEffectiveMatchGroup() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseLobbyContainerFrame::OpenPingOptions()
{
	// just create a new one. panel will destroy itself on close.
	m_hPingPanel = new CTFPingPanel( this, "PingPanel", m_pContents->GetMatchGroup() );
}
