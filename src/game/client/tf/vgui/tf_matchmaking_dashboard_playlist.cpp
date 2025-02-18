//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//


#include "cbase.h"
#include "iclientmode.h"
#include "vgui_controls/AnimationController.h"
#include "tf_controls.h"
#include "tf_shareddefs.h"
#include "tf_matchmaking_dashboard.h"
#include "tf_matchmaking_dashboard_playlist.h"
#include "tf_match_description.h"
#include "tf_item_inventory.h"
#include "ienginevgui.h"
#include "tf_party.h"
#include <vgui/ISurface.h>
#include "tf_partyclient.h"
#include "util_misc.h"
#include "tf_matchmaking_dashboard_explanations.h"

using namespace vgui;
using namespace GCSDK;

ConVar tf_special_event_hide( "tf_special_event_hide", "0", FCVAR_ARCHIVE | FCVAR_HIDDEN );

Panel* GetPlayListPanel()
{
	Panel* pPanel = new CTFDashboardPlaylistPanel( NULL, "ExpandableList" );
	pPanel->MakeReadyForUse();
	pPanel->AddActionSignalTarget( GetMMDashboard() );
	return pPanel;
}
REGISTER_FUNC_FOR_DASHBOARD_PANEL_TYPE( GetPlayListPanel, k_ePlayList );

DECLARE_BUILD_FACTORY( CPlayListEntry );
CPlayListEntry::CPlayListEntry( Panel* pParent, const char* pszName )
	: EditablePanel( pParent, pszName )
	, m_bDisabled( false )
{
	vgui::HScheme scheme = vgui::scheme()->LoadSchemeFromFileEx( enginevgui->GetPanel( PANEL_CLIENTDLL ), "resource/ClientScheme.res", "ClientScheme");
	SetScheme(scheme);
	SetProportional( true );

	m_pToolTipHack = new EditablePanel( this, "ToolTipHack" );
	m_pToolTipButtonHack = new EditablePanel( this, "ToolTipButtonHack" );
	m_pDisabledIcon = new CExImageButton( this, "DisabledIcon", (const char*)NULL, this );
	m_pModeButton = new CExButton( this, "ModeButton", (const char*)NULL, this );
	m_pToolTipButtonHack->InstallMouseHandler( m_pModeButton, true, true );

	m_pToolTipHack->SetTooltip( GetDashboardTooltip( k_eMediumFont ), NULL );
	m_pToolTipButtonHack->SetTooltip( GetDashboardTooltip( k_eMediumFont ), NULL );
}

CPlayListEntry::~CPlayListEntry()
{}

void CPlayListEntry::ApplySchemeSettings( IScheme *pScheme )
{	
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "resource/ui/MainMenuPlayListEntry.res" );

	UpdateDisabledState();
}

void CPlayListEntry::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );

	m_eMatchGroup = (ETFMatchGroup)inResourceData->GetInt( "matchgroup", k_eTFMatchGroup_Invalid );
	m_strImageName = inResourceData->GetString( "image_name" );
	m_strButtonCommand = inResourceData->GetString( "button_command" );
	m_strButtonToken = inResourceData->GetString( "button_token" );
	m_strDescToken = inResourceData->GetString( "desc_token" );
}

void CPlayListEntry::OnCommand( const char *command )
{
	if ( FStrEq( command, "comp_access_info" ) )
	{
		GetMMDashboard()->PostMessage( GetMMDashboard(), new KeyValues( "ShowCompAccess" ) ) ;
	}
}

struct DisabledState_t
{
	bool bUserCanAccess;
	const char* pszImage;
	const char* pszCommand;
	const wchar_t* pszTooltipLocalized;
};

void CPlayListEntry::UpdateBannedState()
{
	auto *pMatchDesc = GetMatchGroupDescription( m_eMatchGroup );
	if ( !pMatchDesc )
		return;

	auto pParty = GTFPartyClient()->GetActiveParty();
	EMMPenaltyPool ePenaltyPool = pMatchDesc->GetPenaltyPool();

	// Check to see if we are banned
	CRTime timeExpire = CRTime::RTime32TimeCur();
	int nDuration = -1;
	bool bBanned = GTFGCClientSystem()->BIsBannedFromMatchmaking( ePenaltyPool, &timeExpire, &nDuration );

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
		return;
	}

	// Check if a party member is banned
	if ( pParty && pParty->BAnyMembersBanned( ePenaltyPool ) )
	{
		CUtlVector< CSteamID > vecBannedSteamIDs;
		for( int i=0; i < pParty->GetNumMembers(); ++i )
		{
			CSteamID steamIDMember = pParty->GetMember( i );
			if ( pParty->BMembersIsBanned( steamIDMember, ePenaltyPool ) )
			{
				vecBannedSteamIDs.AddToTail( steamIDMember );
			}
		}

		Assert( vecBannedSteamIDs.Count() );

		SetDisabled( true, NULL, NULL, BlameNames_t( vecBannedSteamIDs, "#TF_Matchmaking_CantQueue_BanParty", "#TF_Matchmaking_CantQueue_VerbSingle", "#TF_Matchmaking_CantQueue_VerbPlural" ).Get() );
		return;
	}
}

void CPlayListEntry::UpdateDisabledState()
{
	auto *pMatchDesc = GetMatchGroupDescription( m_eMatchGroup );
	if ( !pMatchDesc )
	{
		// Community Browser, Tutorials, etc don't have match groups
		SetEnabled();
		return;
	}

	CUtlVector< CTFPartyClient::QueueEligibilityData_t > vecReasons;
	GTFPartyClient()->BCanQueueForMatch( m_eMatchGroup, vecReasons );
	 
	// Innocent until proven guilty
	SetEnabled();

	// Special case for comp access.  We have to udpate the button
	if ( pMatchDesc->BRequiresCompetitiveAccess() )
	{
		// We don't have comp access
		if( !GTFGCClientSystem()->BHasCompetitiveAccess() )
		{
			SetDisabled( false, "locked_icon", "comp_access_info", g_pVGuiLocalize->Find( "#TF_Competitive_Requirements" ) );
			return;
		}


		auto pParty = GTFPartyClient()->GetActiveParty();
		// Someone else doesn't have comp access
		if( pParty && pParty->BAnyMemberWithoutCompetitiveAccess() )
		{
			CUtlVector< CSteamID > vecBlameSteamIDs;
			for( int i=0; i < pParty->GetNumMembers(); ++i )
			{
				CSteamID steamIDMember = pParty->GetMember( i );
				if ( pParty->BMemberWithoutCompetitiveAccess( steamIDMember ) )
				{
					vecBlameSteamIDs.AddToTail( steamIDMember );
				}
			}

			SetDisabled( false, "locked_icon", "comp_access_info", BlameNames_t( vecBlameSteamIDs, "#TF_Competitive_Requirements_Party", "#TF_PartyMemberState_Singular", "#TF_PartyMemberState_Plural" ).Get() );
			return;
		}
	}

	UpdateBannedState();

	FOR_EACH_VEC( vecReasons, i )
	{
		auto& reason = vecReasons[ i ];
		switch ( reason.m_eReason )
		{
			case CTFPartyClient::k_eDisabledType_None:
			{
				continue;
			}

			case CTFPartyClient::k_eDisabledType_Banned:
			case CTFPartyClient::k_eDisabledType_System:
			{
				SetDisabled( true, "glyph_alert", NULL, reason.wszCantReason );
				return;
			}

			case CTFPartyClient::k_eDisabledType_Criteria:
			{
				// We dont care about criteria issues
				continue;
			}

			case CTFPartyClient::k_eDisabledType_Locked:
			{
				SetDisabled( true, "locked_icon", NULL, reason.wszCantReason );
				return;
			}

			case CTFPartyClient::k_eDisabledType_Network:
			{
				SetDisabled( true, "gc_dc", NULL, reason.wszCantReason );
				return;
			}
		}
	}
}

void CPlayListEntry::OnTick()
{
	UpdateDisabledState();
}

void CPlayListEntry::SetEnabled()
{
	vgui::ivgui()->RemoveTickSignal( GetVPanel() );

	m_bDisabled = false;
	m_pModeButton->SetEnabled( true );
	m_pToolTipHack->SetVisible( false );
	m_pToolTipButtonHack->SetVisible( false );
	m_pDisabledIcon->SetVisible( false );
}

void CPlayListEntry::SetDisabled( bool bUsersCanAccess, const char* pszImage, const char* pszCommand, const wchar_t* pszTooltipLocalized )
{
	// While we're disabled, tick
	vgui::ivgui()->AddTickSignal( GetVPanel(), 1000 );

	m_bDisabled = true;

	m_pModeButton->SetEnabled( bUsersCanAccess );
	m_pToolTipHack->SetVisible( pszTooltipLocalized != NULL );
	m_pToolTipHack->SetDialogVariable( "tiptext", pszTooltipLocalized );
	m_pToolTipHack->SetTooltip( GetDashboardTooltip( k_eMediumFont ), NULL );

	m_pToolTipButtonHack->SetVisible( pszTooltipLocalized != NULL );
	m_pToolTipButtonHack->SetDialogVariable( "tiptext", pszTooltipLocalized );
	m_pToolTipButtonHack->SetTooltip( GetDashboardTooltip( k_eMediumFont ), NULL );

	m_pDisabledIcon->SetVisible( pszImage != NULL );
	if ( pszImage )
	{
		m_pDisabledIcon->SetSubImage( pszImage );
	}
		
	// Button behavior
	m_pDisabledIcon->SetEnabled( pszCommand != NULL );
	m_pDisabledIcon->SetMouseInputEnabled( pszCommand != NULL );
	m_pDisabledIcon->SetCommand( pszCommand );
	m_pDisabledIcon->GetImage()->SetVisible( pszImage != NULL );

	m_pToolTipButtonHack->InstallMouseHandler( pszCommand ? m_pDisabledIcon : m_pModeButton, true, true );
	m_pToolTipHack->InstallMouseHandler( pszCommand ? m_pDisabledIcon : m_pModeButton, true, true );
}


void CPlayListEntry::PerformLayout()
{
	BaseClass::PerformLayout();

	m_pModeButton->SetText( m_strButtonToken );
	m_pModeButton->SetCommand( m_strButtonCommand );
	SetDialogVariable( "desc_token", g_pVGuiLocalize->Find( m_strDescToken ) );

	ImagePanel* pModeImage = FindControl< ImagePanel >( "ModeImage" );
	if ( pModeImage )
	{
		pModeImage->SetImage( m_strImageName );
	}
}

DECLARE_BUILD_FACTORY( CEventPlayListEntry );
CEventPlayListEntry::CEventPlayListEntry( Panel* pParent, const char* pszName )
	: CPlayListEntry( pParent, pszName )
{
	ListenForGameEvent( "world_status_changed" );
}

void CEventPlayListEntry::PerformLayout()
{
	BaseClass::PerformLayout();

	// Pretend we got a OnPlaylistActive if our layout got invalided while the playlist was open
	const CMatchMakingDashboardSidePanel* pPlaylistPanel = GetDashboardPanel().GetTypedPanel< CMatchMakingDashboardSidePanel >( k_ePlayList );
	if ( GetMMDashboard()->BIsSidePanelShowing( pPlaylistPanel ) )
	{
		OnPlaylistActive();
	}
}

void CEventPlayListEntry::ApplySchemeSettings( IScheme *pScheme )
{	
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "resource/ui/MainMenuEventPlayListEntry.res" );

	UpdateEventMatchGroup();
}

void CEventPlayListEntry::OnThink()
{
	BaseClass::OnThink();

	// We count down, so we need to update
	UpdateExpireLabel();
}


void CEventPlayListEntry::UpdateExpireLabel()
{
	// Figure out what kind of coutndown to show.  If it's >= 2 days, just say
	// "X days away"
	// If it's less, then show hh:mm:ss until the actual expiration
	CRTime rtTwoDaysFromNow( CRTime::RTime32TimeCur() );
	rtTwoDaysFromNow = rtTwoDaysFromNow.DateAdd( 2, k_ETimeUnitDay );
	bool bTwoOrMoreDaysAway = m_rtExpireTime >= rtTwoDaysFromNow.GetRTime32();

	RTime32 rtTimeRemaining = m_rtExpireTime > CRTime::RTime32TimeCur() ? m_rtExpireTime - CRTime::RTime32TimeCur() : 0;

	// The event is over!  Update so we stop showing
	if ( rtTimeRemaining == 0 )
	{
		UpdateEventMatchGroup();
		return;
	}

	wchar_t wszTimeBuf[ 128 ];
	if ( bTwoOrMoreDaysAway )
	{
		// Cook up "X days remaining"
		V_wcsncpy( wszTimeBuf, LocalizeNumberWithToken( "#TF_Matchmaking_SpecialEvent_ExpireLong", rtTimeRemaining / ( 60 * 60 * 24 ) ), sizeof( wszTimeBuf ) );
	}
	else
	{
		// Cook up hh:mm:ss time remaining
		int nHrs = rtTimeRemaining / ( 60 * 60 );
		int nMins = ( ( rtTimeRemaining % ( 60 * 60 ) ) / 60 );
		int nSecs = rtTimeRemaining % 60 % 60;

		V_snwprintf( wszTimeBuf,
					 V_ARRAYSIZE( wszTimeBuf ),
					 g_pVGuiLocalize->Find( "#TF_Matchmaking_SpecialEvent_ExpireShort" ),
					 nHrs,
					 nMins,
					 nSecs );
	}

	SetDialogVariable( "expire", m_rtExpireTime == 0 ? L"" : wszTimeBuf );
}

void CEventPlayListEntry::FireGameEvent( IGameEvent *event )
{
	// When this changes, then we might need to change our state
	if ( FStrEq( event->GetName(), "world_status_changed" ) )
	{
		UpdateEventMatchGroup();
	}
}

void CEventPlayListEntry::UpdateDisabledState()
{
	// Special for the event entry.  We just stop right here, because we don't want to look into the match group.
	// We might not know what it is yet.
	if ( GTFGCClientSystem()->BClientOutOfDate() )
	{
		SetDisabled( false, NULL, NULL, g_pVGuiLocalize->Find( "#TF_Competitive_PartyMemberOutOfDate" ) );
		return;
	}

	BaseClass::UpdateDisabledState();
}

void CEventPlayListEntry::OnPlaylistActive()
{
	BaseClass::OnPlaylistActive();

	if ( IsLayoutInvalid() )
		return;

	if ( m_rtExpireTime <= CRTime::RTime32TimeCur() )
		return;

	if ( tf_special_event_hide.GetBool() == false )
	{
		tf_special_event_hide.SetValue( true );
		ShowDashboardExplanation( "SpecialEvents" );
	}

	if ( m_bFlashedOnce )
		return;

	Panel* pFlashColor = FindChildByName( "FlashColor", true );

	if ( pFlashColor )
	{
		float flWait = 0.4f;
		float flBlinkEdgeTime = 0.175f;

		// Reset
		g_pClientMode->GetViewportAnimationController()->RunAnimationCommand( pFlashColor, "alpha", 0, 0.0f, 0.0f, vgui::AnimationController::INTERPOLATOR_LINEAR, 0.8f, true, false );
		// Blink 1
		g_pClientMode->GetViewportAnimationController()->RunAnimationCommand( pFlashColor, "alpha", 100, flWait, flBlinkEdgeTime, vgui::AnimationController::INTERPOLATOR_LINEAR, 0.8f, false, false );
		g_pClientMode->GetViewportAnimationController()->RunAnimationCommand( pFlashColor, "alpha", 0, flWait + flBlinkEdgeTime, flBlinkEdgeTime, vgui::AnimationController::INTERPOLATOR_LINEAR, 0.8f, false, false );
		// Blink 2
		flWait += flBlinkEdgeTime * 2;
		g_pClientMode->GetViewportAnimationController()->RunAnimationCommand( pFlashColor, "alpha", 100, flWait, flBlinkEdgeTime, vgui::AnimationController::INTERPOLATOR_LINEAR, 0.8f, false, false );
		g_pClientMode->GetViewportAnimationController()->RunAnimationCommand( pFlashColor, "alpha", 0, flWait + flBlinkEdgeTime , flBlinkEdgeTime, vgui::AnimationController::INTERPOLATOR_LINEAR, 0.8f, false, false );
	}

	m_bFlashedOnce = true;
}

bool BEventActive()
{
	auto& msgWorldStatus = GTFGCClientSystem()->WorldStatus();
	return ( msgWorldStatus.event_match_group() != k_eTFMatchGroup_Invalid &&
		 CRTime( msgWorldStatus.event_expire_time() ) > CRTime::RTime32TimeCur() );
}

void CEventPlayListEntry::UpdateEventMatchGroup()
{
	// If there's a match group, and the expiration time is in the future, then
	// there's an active event!  We need to update our controls.
	if ( BEventActive() )
	{
		SetVisible( true );

		auto& msgWorldStatus = GTFGCClientSystem()->WorldStatus();
		if ( GTFGCClientSystem()->BClientOutOfDate() )
		{
			m_strImageName = "main_menu/main_menu_button_event"; // Generic image
		}
		else
		{
			m_eMatchGroup = GTFGCClientSystem()->WorldStatus().event_match_group();
			auto *pMatchDesc = GetMatchGroupDescription( m_eMatchGroup );
			auto *pPlayListData = pMatchDesc->GetPlayListEntryData();
			m_strImageName = pPlayListData->m_pszPlayListBackground;
		}

		m_strButtonToken = "#TF_Matchmaking_HeaderSpecialEvent";
		m_strDescToken = "#TF_Matchmaking_HeaderSpecialEventDesc";
		m_rtExpireTime = msgWorldStatus.event_expire_time();

		InvalidateLayout();
	}
	else
	{
		SetVisible( false );
		m_eMatchGroup = k_eTFMatchGroup_Invalid;
	}
}

DECLARE_BUILD_FACTORY( CTFPlaylistPanel );
CTFPlaylistPanel::CTFPlaylistPanel( Panel *parent, const char *panelName )
	: EditablePanel( parent, panelName )
{
	vgui::HScheme scheme = vgui::scheme()->LoadSchemeFromFileEx( enginevgui->GetPanel( PANEL_CLIENTDLL ), "resource/ClientScheme.res", "ClientScheme");
	SetScheme(scheme);
	SetProportional( true );

	ListenForGameEvent( "world_status_changed" );
}

CTFPlaylistPanel::~CTFPlaylistPanel()
{
}

void CTFPlaylistPanel::ApplySchemeSettings( vgui::IScheme *pScheme ) 
{
	BaseClass::ApplySchemeSettings( pScheme );

	KeyValuesAD kvConditions( "conditions" );

	if ( BEventActive() )
	{
		kvConditions->AddSubKey( new KeyValues( "if_event" ) );
	}

	LoadControlSettings( "resource/UI/MatchMakingPlayList.res", NULL, NULL, kvConditions );

	m_pCasual = FindControl< CPlayListEntry >( "CasualEntry" );
	m_pCompetitive = FindControl< CPlayListEntry >( "CompetitiveEntry" );
	m_pMvM = FindControl< CPlayListEntry >( "MvMEntry" );
	m_pEvent = FindControl< CEventPlayListEntry >( "EventEntry" );

	SetMouseInputEnabled( true );
}

void CTFPlaylistPanel::OnCommand( const char *command )
{
	if ( FStrEq( "play_competitive", command ) )
	{
		PostActionSignal( new KeyValues( "PlayCompetitive" ) );
		return;
	}
	else if ( FStrEq( "play_casual", command ) )
	{
		PostActionSignal( new KeyValues( "PlayCasual" ) );
		return;
	}
	else if ( FStrEq( "play_mvm", command ) ) 
	{
		PostActionSignal( new KeyValues( "PlayMvM" ) );
		return;
	}
	else if ( FStrEq( "play_training", command ) )
	{
		PostActionSignal( new KeyValues( "PlayTraining" ) );
	}
	else if ( FStrEq( "play_community", command ) )
	{
		PostActionSignal( new KeyValues( "PlayCommunity" ) );
		return;
	}
	else if ( FStrEq( "create_server", command ) )
	{
		PostActionSignal( new KeyValues( "CreateServer" ) );
		return;
	}
	else if ( FStrEq( "play_event", command ) )
	{
		PostActionSignal( new KeyValues( "PlayEvent" ) ) ;
		return;
	}
}

void CTFPlaylistPanel::OnThink()
{
	if ( m_bLastSawEventActive )
	{
		// While we have an event active, keep checking if it's there so we can clean up when it's gone.
		UpdateEventStatus();
	}
}

void CTFPlaylistPanel::SOEvent( const CSharedObject* pObject )
{
	if ( pObject->GetTypeID() == CTFParty::k_nTypeID )
	{
		UpdatePlaylistEntries();
		return;
	}

	if ( pObject->GetTypeID() == CEconGameAccountClient::k_nTypeID )
	{
		UpdatePlaylistEntries();
		return;
	}

	if ( pObject->GetTypeID() != CEconItem::k_nTypeID )
		return;

	CEconItem *pEconItem = (CEconItem *)pObject;

	// If the item is a competitive pass - update the main menu lock
	// From _items_main.txt
	const item_definition_index_t kCompetitivePassID = 1167;
	if ( pEconItem->GetItemDefIndex() == kCompetitivePassID )
	{
		UpdatePlaylistEntries();
	}
}

void CTFPlaylistPanel::FireGameEvent( IGameEvent* event )
{
	if ( FStrEq( event->GetName(), "world_status_changed" ) )
	{
		UpdateEventStatus();
		UpdatePlaylistEntries();
	}
}

void CTFPlaylistPanel::UpdatePlaylistEntries( void )
{
	if ( !m_pCasual || !m_pCompetitive || !m_pMvM )
		return;

	m_pCasual->UpdateDisabledState();
	m_pCompetitive->UpdateDisabledState();
	m_pMvM->UpdateDisabledState();
	m_pEvent->UpdateDisabledState();
}

void CTFPlaylistPanel::SidePanelActive()
{
	UpdatePlaylistEntries();

	m_pCasual->OnPlaylistActive();
	m_pCompetitive->OnPlaylistActive();
	m_pMvM->OnPlaylistActive();
	m_pEvent->OnPlaylistActive();
}

void CTFPlaylistPanel::UpdateEventStatus()
{
	bool bEventIsActive = BEventActive();
	// If an event came/went we need to reload to get the entries in the right position
	if ( m_bLastSawEventActive != bEventIsActive )
	{
		InvalidateLayout( true, true );
	}
	m_bLastSawEventActive = bEventIsActive;
}


CTFDashboardPlaylistPanel::CTFDashboardPlaylistPanel( Panel *parent, const char *panelName )
	: CMatchMakingDashboardSidePanel( parent, panelName, "resource/ui/MatchMakingDashboardPlayList.res", k_eSideRight )
{
	vgui::HScheme scheme = vgui::scheme()->LoadSchemeFromFileEx( enginevgui->GetPanel( PANEL_CLIENTDLL ), "resource/ClientScheme.res", "ClientScheme");
	SetScheme(scheme);
	SetProportional( true );

	m_pPlayList = new CTFPlaylistPanel( this, "playlist" );
	AddActionSignalTarget( m_pPlayList );

	ListenForGameEvent( "party_updated" );
}

CTFDashboardPlaylistPanel::~CTFDashboardPlaylistPanel()
{
	m_pPlayList->MarkForDeletion();
}

void CTFDashboardPlaylistPanel::AddActionSignalTarget( Panel *messageTarget )
{
	BaseClass::AddActionSignalTarget( messageTarget );
	m_pPlayList->AddActionSignalTarget( messageTarget );
}

void CTFDashboardPlaylistPanel::OnCommand( const char *command )
{
	if ( FStrEq( "view_match_settings", command ) )
	{
		PostActionSignal( new KeyValues( "ViewMatchSettings" ) );
		return;
	}

	BaseClass::OnCommand( command );
}

void CTFDashboardPlaylistPanel::FireGameEvent( IGameEvent *event )
{
	if ( FStrEq( event->GetName(), "party_updated" ) )
	{
		InvalidateLayout();
	}
}
