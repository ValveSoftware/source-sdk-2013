//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#include "cbase.h"

#include "tf_party.h"
#include "tf_partyclient.h"
#include "tf_matchcriteria.h"
#include "tf_item_inventory.h"
#include "vgui_controls/PropertySheet.h"
#include "vgui_controls/SectionedListPanel.h"
#include "vgui/IInput.h"
#include <vgui_controls/ImageList.h>
#include "vgui_avatarimage.h"
#include "tf_ladder_data.h"
#include "vgui_controls/Menu.h"
#include "tf_match_description.h"
#include "tf_badge_panel.h"
#include "tf_controls.h"

#include "tf_lobbypanel.h"
#include "tf_lobby_container_frame.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

ConVar tf_matchmaking_join_in_progress( "tf_matchmaking_join_in_progress", "0", FCVAR_DONTRECORD | FCVAR_ARCHIVE, "Saved preference for if the player wants to join games in progress." );

const int k_iPopIndex_Any = -1000;
const int k_iPopIndex_OnlyNotYetCompleted = -1001;
const int k_iPopIndex_AnyNormal = -1002;
const int k_iPopIndex_AnyIntermediate = -1003;
const int k_iPopIndex_AnyAdvanced = -1004;
const int k_iPopIndex_AnyExpert = -1005;
const int k_iPopIndex_AnyHaunted = -1006;

static void GetMvmChallengeSet( int idxChallenge, CMvMMissionSet &result )
{
	result.Clear();

	if ( idxChallenge >= 0 )
	{
		result.SetMissionBySchemaIndex( idxChallenge, true );
		return;
	}

	auto &groupCriteria = GTFPartyClient()->GetEffectiveGroupCriteria();
	bool bMannUP = IsMannUpGroup( groupCriteria.GetMatchGroup() );
#ifdef USE_MVM_TOUR
	int idxTour = groupCriteria.GetMannUpTourIndex();
	Assert( bMannUP || idxTour < 0 );
#endif // USE_MVM_TOUR

#ifdef USE_MVM_TOUR
	uint32 nNotCompletedChallenges = ~0U;
	CTFParty *pParty = GTFGCClientSystem()->GetParty();
	if ( pParty )
	{
		for ( int i = 0 ; i < pParty->GetNumMembers() ; ++i )
		{
			nNotCompletedChallenges &= ~pParty->Obj().members( i ).completed_missions();
		}
	}
	else
	{
		if ( idxTour >= 0 )
		{
			uint32 nTours = 0, nCompletedChallenge = 0;
			GTFGCClientSystem()->BGetLocalPlayerBadgeInfoForTour( idxTour, &nTours, &nCompletedChallenge );

			nNotCompletedChallenges = ~nCompletedChallenge;
		}
	}
#endif // USE_MVM_TOUR

	for ( int i = 0 ; i < GetItemSchema()->GetMvmMissions().Count() ; ++i )
	{
		const MvMMission_t &chal = GetItemSchema()->GetMvmMissions()[ i ];

		// Cannot select non-MannUp missions in mann up mode
#ifdef USE_MVM_TOUR
		int iBadgeSlot = (idxTour < 0) ? -1 : GetItemSchema()->GetMvmMissionBadgeSlotForTour( idxTour, i );
		if ( bMannUP && iBadgeSlot < 0 )
			continue;
#else // new mm
		bool bIsChallengeInMannUp = chal.m_unMannUpPoints > 0;
		if ( bMannUP && !bIsChallengeInMannUp )
			continue;
#endif // USE_MVM_TOUR

		// Does this challenge fit the search criteria?
		bool bSelect = false;
		switch ( idxChallenge )
		{
			case k_iPopIndex_Any:
					bSelect = true;
				break;
			case k_iPopIndex_OnlyNotYetCompleted:
#ifdef USE_MVM_TOUR
				if ( iBadgeSlot >= 0 )
				{
					int iChallengeBit = ( 1 << iBadgeSlot );
					if ( nNotCompletedChallenges & iChallengeBit )
					{
						bSelect = true;
					}
				}
#endif // USE_MVM_TOUR
				break;

			case k_iPopIndex_AnyNormal:
				bSelect = ( chal.m_eDifficulty == k_EMvMChallengeDifficulty_Normal );
				break;

			case k_iPopIndex_AnyIntermediate:
				bSelect = ( chal.m_eDifficulty == k_EMvMChallengeDifficulty_Intermediate );
				break;

			case k_iPopIndex_AnyAdvanced:
				bSelect = ( chal.m_eDifficulty == k_EMvMChallengeDifficulty_Advanced );
				break;

			case k_iPopIndex_AnyExpert:
				bSelect = ( chal.m_eDifficulty == k_EMvMChallengeDifficulty_Expert );
				break;

			case k_iPopIndex_AnyHaunted:
				bSelect = ( chal.m_eDifficulty == k_EMvMChallengeDifficulty_Haunted );
				break;

			default:
				Assert( false );
		}
		result.SetMissionBySchemaIndex( i, bSelect );
	}
}

#ifdef ENABLE_GC_MATCHMAKING

Color s_colorBannedPlayerListItem( 250, 50, 45, 255 );
Color s_colorPlayerListItem( 255, 255, 255, 255 );
Color s_colorChatRemovedFromQueue( 200, 10, 10, 255 );
Color s_colorChatAddedToQueue( 10, 200, 10, 255 );
Color s_colorChatPlayerJoinedParty( 255, 255, 255, 255 );
Color s_colorChatPlayerJoinedPartyName( 200, 200, 10, 255 );
Color s_colorChatPlayerLeftParty( 255, 255, 255, 255 );
Color s_colorChatPlayerLeftPartyName( 200, 200, 10, 255 );
Color s_colorChatPlayerChatName( 200, 200, 10, 255 );
Color s_colorChatPlayerChatText( 180, 180, 180, 255 );
Color s_colorChatDefault( 180, 180, 180, 255 );
Color s_colorChallengeForegroundEnabled( 255, 255, 255, 255 );
Color s_colorChallengeForegroundHaunted( 135, 79, 173, 255 );
Color s_colorChallengeForegroundDisabled( 100, 100, 100, 128 );
Color s_colorChallengeHeader( 250, 114, 45, 255 );


CBaseLobbyPanel::CBaseLobbyPanel( vgui::Panel *pParent, CBaseLobbyContainerFrame* pContainer ) 
	: vgui::PropertySheet( pParent, "LobbyPanel" ), m_sPersonaStateChangedCallback( this, &CBaseLobbyPanel::OnPersonaStateChanged )
	, m_pContainer( pContainer )
{
	//ListenForGameEvent( "lobby_updated" );
	ListenForGameEvent( "party_updated" );
	ListenForGameEvent( "mm_lobby_chat" );
	ListenForGameEvent( "mm_lobby_member_join" );
	ListenForGameEvent( "mm_lobby_member_leave" );

	m_iWritingPanel = 0;

	m_pSearchActiveGroupBox = NULL;
	m_pSearchActiveTitleLabel = NULL;
	m_pSearchActivePenaltyLabel = NULL;
	m_pPartyHasLowPriority = NULL;

	m_pJoinLateCheckButton = NULL;
	m_pJoinLateValueLabel = NULL;

	m_pInviteButton = NULL;
	m_pChatLog = NULL;
	//m_nFirstMapShown = -1;
	m_pImageList = NULL;

	m_iImageIsBanned = -1;
	m_iImageRadioButtonYes = -1;
	m_iImageRadioButtonNo = -1;
	m_iImageCheckBoxDisabled = -1;
	m_iImageCheckBoxYes = -1;
	m_iImageCheckBoxNo = -1;
	m_iImageCheckBoxMixed = -1;
	m_iImageNew = -1;
	m_iImageNo = -1;

	m_fontPlayerListItem = 0;

	// Party
	m_pPartyActiveGroupBox = new vgui::EditablePanel( this, "PartyActiveGroupBox" );
	vgui::EditablePanel *pPartyGroupPanel = new vgui::EditablePanel( m_pPartyActiveGroupBox, "PartyGroupBox" );
	m_pChatPlayerList = new vgui::SectionedListPanel( pPartyGroupPanel, "PartyPlayerList" );
	m_pChatTextEntry = new ChatTextEntry( m_pPartyActiveGroupBox, "ChatTextEntry" ); Assert( m_pChatTextEntry );
	m_pChatLog = new ChatLog( m_pPartyActiveGroupBox, "ChatLog" ); Assert( m_pChatLog );

	m_mapAvatarsToImageList.SetLessFunc( DefLessFunc(int) );

	m_eCurrentPartyState = CSOTFParty_State_UI;

	m_flRefreshPlayerListTime = -1.f;

	m_pToolTip = new CMainMenuToolTip( this );
 	vgui::EditablePanel* pToolTipEmbeddedPanel = new vgui::EditablePanel( this, "TooltipPanel" );		
	pToolTipEmbeddedPanel->SetKeyBoardInputEnabled( false );
	pToolTipEmbeddedPanel->SetMouseInputEnabled( false );
	m_pToolTip->SetEmbeddedPanel( pToolTipEmbeddedPanel );
	m_pToolTip->SetTooltipDelay( 0 );
}

CBaseLobbyPanel::~CBaseLobbyPanel()
{
	delete m_pImageList;
	m_pImageList = NULL;
}

void CBaseLobbyPanel::OnClickedOnPlayer()
{
	int iSelected = m_pChatPlayerList->GetSelectedItem();
	//m_pChatPlayerList->ClearSelection();
	if ( iSelected < 0 )
		return;
	CSteamID steamID = SteamIDFromDecimalString( m_pChatPlayerList->GetItemData( iSelected )->GetString( "steamid", "" ) );
	if ( !steamID.IsValid() )
		return;

	vgui::Menu *menu = new vgui::Menu(this, "ContextMenu");

	int x, y;
	vgui::input()->GetCursorPos(x, y);
	menu->SetPos(x, y);

	wchar_t wszLocalized[512];
	char szLocalized[512];
	g_pVGuiLocalize->ConstructString_safe( wszLocalized, g_pVGuiLocalize->Find( "#TF_ScoreBoard_Context_Trade" ), 0 );
	g_pVGuiLocalize->ConvertUnicodeToANSI( wszLocalized, szLocalized, sizeof( szLocalized ) );

	menu->AddMenuItem( szLocalized, new KeyValues( "TradeWithUser", "steamid", CFmtStr( "%llu", steamID.ConvertToUint64() ).Access() ), this );

	menu->SetVisible(true);
}

void CBaseLobbyPanel::SetMatchmakingModeBackground()
{
	tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s", __FUNCTION__ );

	// Get the background panel. 
	vgui::ImagePanel* pModeBackgroundImage = FindControl< vgui::ImagePanel >( "ModeBackgroundImage", true );
	if ( !pModeBackgroundImage )
		return;

	const char* pszImageName = NULL;
	const IMatchGroupDescription* pMatchDesc = GetMatchGroupDescription( GetMatchGroup() );
	if ( pMatchDesc && pMatchDesc->m_pProgressionDesc )
	{
		// Get the level of the local player, and pull the background image out of the level
		const LevelInfo_t& level = pMatchDesc->m_pProgressionDesc->YieldingGetLevelForSteamID( steamapicontext->SteamUser()->GetSteamID() );
		pszImageName = level.m_pszLobbyBackgroundImage;
	}

	// Set the image name, if we got one, into the panel
	if ( pszImageName )
	{
		pModeBackgroundImage->SetImage( pszImageName );
	}

	// Only show if we got one (MvM doesn't have any)
	pModeBackgroundImage->SetVisible( pszImageName != NULL );
}

//-----------------------------------------------------------------------------
void CBaseLobbyPanel::FireGameEvent( IGameEvent *event )
{
	if ( !IsVisible() || !m_pContainer->IsVisible() )
		return;

	const char *pszEventName = event->GetName();
	if ( !Q_stricmp( pszEventName, "party_updated" ) )
	{

		CTFParty *pParty = GTFGCClientSystem()->GetParty();
		if ( pParty )
		{
			if ( m_eCurrentPartyState != pParty->GetState() )
			{
				wchar_t wszLocalized[512];

				switch ( pParty->GetState() )
				{
					case CSOTFParty_State_UI:
						m_pChatLog->InsertColorChange( s_colorChatRemovedFromQueue );
						g_pVGuiLocalize->ConstructString_safe( wszLocalized, g_pVGuiLocalize->Find( "#TF_Matchmaking_RemovedFromQueue" ), 0 );
						m_pChatLog->InsertString( wszLocalized );
						break;

					case CSOTFParty_State_FINDING_MATCH:
						m_pChatLog->InsertColorChange( s_colorChatAddedToQueue );
						g_pVGuiLocalize->ConstructString_safe( wszLocalized, g_pVGuiLocalize->Find( "#TF_Matchmaking_AddedToQueue" ), 0 );
						m_pChatLog->InsertString( wszLocalized );
						break;

					default:
						Assert( false );
					case CSOTFParty_State_IN_MATCH:
						break;
				}
				m_eCurrentPartyState = pParty->GetState();
			}
		}
		else
		{
			m_eCurrentPartyState = CSOTFParty_State_UI;
		}

		UpdatePlayerList();
		WriteGameSettingsControls();
		return;
	}

	if ( !Q_stricmp( pszEventName, "mm_lobby_chat" ) )
	{
		CSteamID steamID = SteamIDFromDecimalString( event->GetString( "steamid", "0" ) );

		const char *pszText = event->GetString( "text", "" );
		int l = V_strlen( pszText );
		if ( l > 0 )
		{
			int nBufSize = l * sizeof(wchar_t) + 4;
			wchar_t *wText = (wchar_t *)stackalloc( nBufSize );
			V_UTF8ToUnicode( pszText, wText, nBufSize );
			// TODO(Universal Parties):
			// switch ( event->GetInt( "type", CTFGCClientSystem::k_eLobbyMsg_UserChat ) )
			// {
			// 	default:
			// 		Assert( !"Unknown chat message type" );
			// 	case CTFGCClientSystem::k_eLobbyMsg_SystemMsgFromLeader:
			// 		m_pChatLog->InsertColorChange( s_colorChatDefault );
			// 		m_pChatLog->InsertString( wText );
			// 		m_pChatLog->InsertString("\n");
			// 		break;
			//
			// 	case CTFGCClientSystem::k_eLobbyMsg_UserChat:
			// 	{
			//
			// 		wchar_t wCharPlayerName[ 128 ];
			// 		GetPlayerNameForSteamID( wCharPlayerName, sizeof(wCharPlayerName), steamID );
			// 		m_pChatLog->InsertColorChange( s_colorChatPlayerChatName );
			// 		m_pChatLog->InsertString( wCharPlayerName );
			// 		m_pChatLog->InsertString( ": " );
			// 		m_pChatLog->InsertColorChange( s_colorChatPlayerChatText );
			// 		m_pChatLog->InsertString( wText );
			// 		m_pChatLog->InsertString("\n");
			// 	} break;
			// }
		}

		return;
	}

	if ( !Q_stricmp( pszEventName, "mm_lobby_member_join" ) )
	{
		CSteamID steamID = SteamIDFromDecimalString( event->GetString( "steamid", "0" ) );

		bool bSolo = false;
		if ( steamID == steamapicontext->SteamUser()->GetSteamID() )
		{
			m_pChatLog->SetText("");
			bSolo = ( event->GetInt( "solo", 0 ) != 0 );
		}

		wchar_t wszLocalized[512];

		// An empty lobby by ourselves?
		if ( bSolo )
		{
			m_pChatLog->InsertColorChange( s_colorChatDefault );

			g_pVGuiLocalize->ConstructString_safe( wszLocalized, g_pVGuiLocalize->Find( "#TF_Matchmaking_StartSearchChat" ), 0 );
			m_pChatLog->InsertString( wszLocalized );

			g_pVGuiLocalize->ConstructString_safe( wszLocalized, g_pVGuiLocalize->Find( "#TF_Matchmaking_InviteFriendsChat" ), 0 );
			m_pChatLog->InsertString( wszLocalized );
		}
		else
		{
			wchar_t wCharPlayerName[128];
			m_pChatLog->InsertColorChange( s_colorChatPlayerJoinedPartyName );
			GetPlayerNameForSteamID( wCharPlayerName, sizeof( wCharPlayerName ), steamID );
			m_pChatLog->InsertColorChange( s_colorChatPlayerJoinedParty );

			g_pVGuiLocalize->ConstructString_safe( wszLocalized, g_pVGuiLocalize->Find( "#TF_Matchmaking_PlayerJoinedPartyChat" ), 1, wCharPlayerName );
			m_pChatLog->InsertString( wszLocalized );
		}

		UpdatePlayerList();

		return;
	}

	if ( !Q_stricmp( pszEventName, "mm_lobby_member_leave" ) )
	{
		CSteamID steamID = SteamIDFromDecimalString( event->GetString( "steamid", "0" ) );
		wchar_t wCharPlayerName[ 128 ];
		GetPlayerNameForSteamID( wCharPlayerName, sizeof(wCharPlayerName), steamID );
		m_pChatLog->InsertColorChange( s_colorChatPlayerLeftParty );

		wchar_t wszLocalized[512];
		g_pVGuiLocalize->ConstructString_safe( wszLocalized, g_pVGuiLocalize->Find( "#TF_Matchmaking_PlayerLeftPartyChat" ), 1, wCharPlayerName );
		m_pChatLog->InsertString( wszLocalized );

		UpdatePlayerList();
		WriteGameSettingsControls();

		return;
	}

	Assert( false );
}

void CBaseLobbyPanel::OnCommand( const char *command )
{
	if ( FStrEq( command, "invite" ) )
	{
		// TODO(Universal Parties): GTFGCClientSystem()->RequestActivateInvite();
	}
	else if ( FStrEq( command, "open_charinfo" ) )
	{
		engine->ClientCmd_Unrestricted( "open_econui_backpack" );
	}
	else
	{
		// What other commands are there?
		Assert( false );
	}
}


bool CBaseLobbyPanel::IsAnyoneBanned( RTime32 &rtimeExpire ) const
{
	bool bBanned = false;
	RTime32 rtimeHighest = 0;

	for( int i=0; i<m_vecPlayers.Count(); ++i )
	{
		if ( m_vecPlayers[i].m_bIsBanned )
		{
			bBanned = true;

			if ( m_vecPlayers[i].m_rtimeBanExpire > rtimeHighest )
			{
				rtimeHighest = m_vecPlayers[i].m_rtimeBanExpire;
			}
		}
	}

	rtimeExpire = rtimeHighest;
	return bBanned;
}

const CBaseLobbyPanel::LobbyPlayerInfo* CBaseLobbyPanel::GetLobbyPlayerInfo( CSteamID &steamID ) const
{
	for ( int i = 0; i < m_vecPlayers.Count(); ++i )
	{
		if ( m_vecPlayers[i].m_steamID == steamID )
		{
			return &m_vecPlayers[i];
		}
	}

	Assert( false );
	return NULL;
}

bool CBaseLobbyPanel::IsAnyoneLowPriority( RTime32 &rtimeExpire ) const
{
	bool bLowPriority = false;
	RTime32 rtimeHighest = 0;

	for ( int i = 0; i < m_vecPlayers.Count(); ++i )
	{
		if ( m_vecPlayers[i].m_bIsLowPriority )
		{
			bLowPriority = true;

			if ( m_vecPlayers[i].m_rtimeLowPriorityExpire > rtimeHighest )
			{
				rtimeHighest = m_vecPlayers[i].m_rtimeLowPriorityExpire;
			}
		}
	}

	rtimeExpire = rtimeHighest;
	return bLowPriority;
}

void CBaseLobbyPanel::UpdateControls()
{
	WriteGameSettingsControls();
	WriteStatusControls();
	UpdatePlayerList();
}

void CBaseLobbyPanel::OnCheckButtonChecked( vgui::Panel *panel )
{
	if ( m_iWritingPanel > 0 )
		return;
	if ( panel == m_pJoinLateCheckButton )
	{
		if ( GTFPartyClient()->BIsPartyLeader() && GCClientSystem()->BConnectedtoGC() )
		{
			tf_matchmaking_join_in_progress.SetValue( m_pJoinLateCheckButton->IsSelected() ? 1 : 0 );
			GTFPartyClient()->MutLocalGroupCriteria().SetLateJoin( m_pJoinLateCheckButton->IsSelected() );
		}
		else
		{
			WriteGameSettingsControls();
		}
	}
}

void CBaseLobbyPanel::OnTradeWithUser( KeyValues* params )
{
	CSteamID steamID = SteamIDFromDecimalString( params->GetString( "steamid", "" ) );
	if ( !steamID.IsValid() )
		return;
	steamapicontext->SteamFriends()->ActivateGameOverlayToUser( "jointrade", steamID );
}

void CBaseLobbyPanel::OnItemLeftClick( vgui::Panel* panel )
{
	if ( m_iWritingPanel > 0 )
		return;
	m_pChatTextEntry->RequestFocus();
	if ( panel == m_pChatPlayerList )
	{
		OnClickedOnPlayer();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseLobbyPanel::WriteStatusControls()
{
	tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s", __FUNCTION__ );

	enum EDisabledState
	{
		DISABLED_NONE,
		DISABLED_NO_GC,
		DISABLED_MATCH_IN_PROGRESS
	};

	EDisabledState eDisabled = DISABLED_NONE;

	if ( GTFGCClientSystem()->BHaveLiveMatch() )
		{ eDisabled = DISABLED_MATCH_IN_PROGRESS; }

	if ( !GTFGCClientSystem()->BHealthyGCConnection() )
		{ eDisabled = DISABLED_NO_GC; }

	SetControlVisible( "NoGCGroupBox", eDisabled == DISABLED_NO_GC, true );
	SetControlVisible( "MatchInProgressGroupBox", eDisabled == DISABLED_MATCH_IN_PROGRESS, true );

	if ( GTFPartyClient()->BInQueue() )
	{
		m_pSearchActiveGroupBox->SetVisible(  true );

		const CMsgMatchmakingProgress &progress = GTFGCClientSystem()->m_msgMatchmakingProgress;
		wchar_t wszCount[32];
		CUtlVector<vgui::Label *> vecNearbyFields;
		vgui::Label *pNearbyColumnHead = dynamic_cast<vgui::Label *>( FindChildByName( "NearbyColumnHead", true ) );
		Assert( pNearbyColumnHead );
		vecNearbyFields.AddToTail( pNearbyColumnHead );

		#define DO_FIELD( protobufname, labelname, bNearby ) \
			vgui::Label *p##labelname = dynamic_cast<vgui::Label *>( FindChildByName( #labelname, true ) ); \
			Assert( p##labelname ); \
			if ( p##labelname ) \
			{ \
				if ( progress.has_##protobufname() ) \
				{ \
					_snwprintf( wszCount, ARRAYSIZE( wszCount ), L"%d", progress.protobufname() ); \
					p##labelname->SetText( wszCount ); \
					if ( bNearby ) bHasAnyNearbyData = true; \
				} \
				else \
				{ \
					p##labelname->SetText( "#TF_Matchmaking_NoData" ); \
				} \
				if ( bNearby ) vecNearbyFields.AddToTail( p##labelname ); \
			}

		bool bHasAnyNearbyData = false;
		DO_FIELD( matching_worldwide_searching_players, PlayersSearchingMatchingWorldwideValue, false )
		DO_FIELD( matching_near_you_searching_players, PlayersSearchingMatchingNearbyValue, true )
		DO_FIELD( matching_worldwide_active_players, PlayersInGameMatchingWorldwideValue, false )
		DO_FIELD( matching_near_you_active_players, PlayersInGameMatchingNearbyValue, true )
		DO_FIELD( matching_worldwide_empty_gameservers, EmptyGameserversMatchingWorldwideValue, false )
		DO_FIELD( matching_near_you_empty_gameservers, EmptyGameserversMatchingNearbyValue, true )
		DO_FIELD( total_worldwide_searching_players, PlayersSearchingTotalWorldwideValue, false )
		DO_FIELD( total_near_you_searching_players, PlayersSearchingTotalNearbyValue, true )
		DO_FIELD( total_worldwide_active_players, PlayersInGameTotalWorldwideValue, false )
		DO_FIELD( total_near_you_active_players, PlayersInGameTotalNearbyValue, true )

		FOR_EACH_VEC( vecNearbyFields, i )
		{
			vecNearbyFields[i]->SetVisible( bHasAnyNearbyData );
		}

		// Show the low priority message?  This only really applies to ladder games for now.
		RTime32 rtimeExpire = 0;
		bool bShowTimer = IsAnyoneLowPriority( rtimeExpire );
		if ( bShowTimer )
		{
			CRTime timeExpire( rtimeExpire );
			timeExpire.SetToGMT( false );
			char time_buf[k_RTimeRenderBufferSize];
			if ( m_pPartyHasLowPriority )
			{
				m_pPartyHasLowPriority->SetDialogVariable( "penaltytimer", CFmtStr( "Expires: %s", ( ( rtimeExpire > 0 ) ? timeExpire.Render( time_buf ) : "" ) ) );
			}
			if ( m_pSearchActivePenaltyLabel )
			{
				m_pSearchActivePenaltyLabel->SetText( "#TF_Matchmaking_PartyLowPriority" );
			}
		}
		if ( m_pPartyHasLowPriority )
		{
			m_pPartyHasLowPriority->SetVisible( bShowTimer );
		}
		if ( m_pSearchActivePenaltyLabel )
		{
			m_pSearchActivePenaltyLabel->SetVisible( bShowTimer );
		}

		// HOLY CHEESEBALL BUSY INDICATOR
		const wchar_t *pwszEllipses = &L"....."[ 4 - ( (unsigned)Plat_FloatTime() % 5U ) ];
		wchar_t wszLocalized[512];
		g_pVGuiLocalize->ConstructString_safe( wszLocalized, g_pVGuiLocalize->Find( "#TF_Matchmaking_Searching" ), 1, pwszEllipses );
		if ( m_pSearchActiveTitleLabel )
		{
			m_pSearchActiveTitleLabel->SetText( wszLocalized );
		}
	}
	else
	{
		m_pSearchActiveGroupBox->SetVisible( false );
	}
}

void CBaseLobbyPanel::WriteGameSettingsControls()
{
	tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s", __FUNCTION__ );

	// Make sure we want to be in matchmaking.  (If we don't, the frame should hide us pretty quickly.)
	// We might get an event or something right at the transition point occasionally when the UI should
	// not be visible
	if ( !GTFGCClientSystem()->BUserInModalMMUI() )
		{ return; }

	SetMatchmakingModeBackground();

	bool bLeader = GTFPartyClient()->BIsPartyLeader();
	bool bInUIState = BIsPartyInUIState();
	bool bLateJoinSelected = GTFPartyClient()->GetEffectiveGroupCriteria().GetLateJoin();

	m_pJoinLateCheckButton->ToggleButton::SetSelected( bLateJoinSelected ); // !KLUDGE! call base to avoid firing the signal

	bool bShowLateJoin = ShouldShowLateJoin();
	m_pJoinLateCheckButton->SetVisible( bShowLateJoin && bLeader );
	m_pJoinLateValueLabel->SetText( bLateJoinSelected ? "#TF_Matchmaking_SearchForAll" : "#TF_Matchmaking_SearchForNew" );
	m_pJoinLateValueLabel->SetVisible( bShowLateJoin && !bLeader );
	//m_pJoinLateValueLabel->SetEnabled( bInUIState );

	m_pInviteButton->SetVisible( bInUIState && ( m_vecPlayers.Count() < k_nTFPartyMaxSize ) );

	m_pChatTextEntry->RequestFocus();
}

//-----------------------------------------------------------------------------
// Purpose: Updates the player list
//-----------------------------------------------------------------------------
void CBaseLobbyPanel::UpdatePlayerList()
{
	tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s", __FUNCTION__ );

	if ( !IsVisible() || !m_pContainer->IsVisible() )
		return;

	m_pChatPlayerList->ClearSelection();
	m_pChatPlayerList->RemoveAll();
	m_vecPlayers.RemoveAll();

	ETFMatchGroup eMatchGroup = GTFPartyClient()->GetEffectiveGroupCriteria().GetMatchGroup();
	bool bLadderGame = IsLadderGroup( eMatchGroup );
	const IMatchGroupDescription *pMatchDesc = GetMatchGroupDescription( GetMatchGroup() );
	EMMPenaltyPool ePenaltyPool = pMatchDesc ? pMatchDesc->m_params.m_ePenaltyPool : eMMPenaltyPool_Invalid;

	if ( steamapicontext == NULL || steamapicontext->SteamUser() == NULL )
		return;

	// Locate party, if we have one.
	CTFParty *pParty = GTFGCClientSystem()->GetParty();

	if ( pParty == NULL )
	{
		LobbyPlayerInfo p;
		p.m_steamID = steamapicontext->SteamUser()->GetSteamID();
		p.m_sName = steamapicontext->SteamFriends()->GetPersonaName();
		p.m_bHasTicket = GTFGCClientSystem()->BLocalPlayerInventoryHasMvmTicket();
		p.m_bSquadSurplus = GTFPartyClient()->GetLocalPlayerCriteria().GetSquadSurplus();
#ifdef USE_MVM_TOUR
		int idxTour = GTFPartyClient()->GetEffectiveGroupCriteria().GetMannUpTourIndex();
		if ( idxTour < 0 || !GTFGCClientSystem()->BGetLocalPlayerBadgeInfoForTour( idxTour, &p.m_nBadgeLevel, &p.m_nCompletedChallenges ) )
		{
			p.m_nBadgeLevel = 0;
			p.m_nCompletedChallenges = 0;
		}
#endif // USE_MVM_TOUR
		p.m_pAvatarImage = NULL;
		p.m_bHasCompetitiveAccess = GTFGCClientSystem()->BHasCompetitiveAccess();
		CSOTFLadderData *pData = GetLocalPlayerLadderData( eMatchGroup );
		p.m_unLadderRank = ( pData ? pData->Obj().rank() : 1u );

		uint32 unExperienceLevel = 1u;
		const IProgressionDesc *pProgressionDesc = pMatchDesc ? pMatchDesc->m_pProgressionDesc : NULL;
		if ( pData && pProgressionDesc && pMatchDesc->m_params.m_eMatchType == MATCH_TYPE_CASUAL )
		{
			LevelInfo_t levelInfo = pProgressionDesc->GetLevelForExperience( pData->Obj().experience() );
			unExperienceLevel = levelInfo.m_nLevelNum;
		}
		p.m_unExperienceLevel = unExperienceLevel;

		CEconGameAccountClient *pGameAccountClient = NULL;
		if ( InventoryManager() && TFInventoryManager()->GetLocalTFInventory() && TFInventoryManager()->GetLocalTFInventory()->GetSOC() )
		{
			pGameAccountClient = TFInventoryManager()->GetLocalTFInventory()->GetSOC()->GetSingleton<CEconGameAccountClient>();
		}

		p.m_bIsBanned = false;
		p.m_rtimeBanExpire = 0;
		p.m_rtimeLowPriorityExpire = 0;
		p.m_bIsLowPriority = false;
		if ( pGameAccountClient && ePenaltyPool != eMMPenaltyPool_Invalid )
		{
			switch ( ePenaltyPool )
			{
			case eMMPenaltyPool_Casual:
				p.m_rtimeBanExpire = pGameAccountClient->Obj().matchmaking_casual_ban_expiration();
				p.m_rtimeLowPriorityExpire = pGameAccountClient->Obj().matchmaking_casual_low_priority_expiration();
				p.m_bIsBanned = p.m_rtimeBanExpire > CRTime::RTime32TimeCur();
				p.m_bIsLowPriority = p.m_rtimeLowPriorityExpire > CRTime::RTime32TimeCur();
				break;
			case eMMPenaltyPool_Ranked:
				p.m_rtimeBanExpire = pGameAccountClient->Obj().matchmaking_ranked_ban_expiration();
				p.m_rtimeLowPriorityExpire = pGameAccountClient->Obj().matchmaking_ranked_low_priority_expiration();
				p.m_bIsBanned = p.m_rtimeBanExpire > CRTime::RTime32TimeCur();
				p.m_bIsLowPriority = p.m_rtimeLowPriorityExpire > CRTime::RTime32TimeCur();
				break;
			default: Assert( false );
			}
		}

		// !TEST!
		//p.m_bIsBanned = true;

		//for (int i = 0 ; i < 6 ; ++i) // !TEST!
		m_vecPlayers.AddToTail( p );
	}
	else
	{
		for ( int i = 0 ; i < pParty->GetNumMembers() ; ++i )
		{
			LobbyPlayerInfo p;
			p.m_steamID = pParty->GetMember( i );
			p.m_sName = steamapicontext->SteamFriends()->GetFriendPersonaName( p.m_steamID );
			if ( p.m_sName.IsEmpty() )
				continue;
			p.m_bHasTicket = pParty->Obj().members( i ).owns_ticket();
			p.m_nBadgeLevel = pParty->Obj().members( i ).badge_level();
			p.m_nCompletedChallenges = pParty->Obj().members( i ).completed_missions();
			p.m_bSquadSurplus = pParty->GetMemberMatchCriteria( i ).GetSquadSurplus();
			p.m_pAvatarImage = nullptr;
			p.m_bIsBanned = pParty->Obj().members( i ).is_banned();
			p.m_bHasCompetitiveAccess = pParty->Obj().members( i ).competitive_access();
			p.m_unLadderRank = pParty->Obj().members( i ).ladder_rank();
			p.m_rtimeBanExpire = pParty->Obj().matchmaking_ban_time();
			p.m_rtimeLowPriorityExpire = pParty->Obj().matchmaking_low_priority_time();
			p.m_bIsLowPriority = pParty->Obj().members( i ).is_low_priority();

			uint32 unExperienceLevel = 1u;
			const IProgressionDesc *pProgressionDesc = pMatchDesc ? pMatchDesc->m_pProgressionDesc : NULL;
			if ( pProgressionDesc && pMatchDesc->m_params.m_eMatchType == MATCH_TYPE_CASUAL )
			{
				LevelInfo_t levelInfo = pProgressionDesc->GetLevelForExperience( pParty->Obj().members( i ).experience() );
				unExperienceLevel = levelInfo.m_nLevelNum;
			}
			p.m_unExperienceLevel = unExperienceLevel;

			if ( p.m_steamID == pParty->GetLeader() )
			{
				m_vecPlayers.AddToHead( p );
			}
			else
			{
				m_vecPlayers.AddToTail( p );
			}
		}
	}

	for( int i = 0; i < m_vecPlayers.Count(); ++i )
	{
		KeyValues *pKeyValues = new KeyValues( "data" );
		CUtlString sName;
		if ( i == 0 && m_vecPlayers.Count() > 1 )
		{
			sName.Format( "%s (leader)", m_vecPlayers[i].m_sName.String() ); // !FIXME! Localize
		}
		else
		{
			sName = m_vecPlayers[i].m_sName;
		}
		pKeyValues->SetString( "name", sName );

		pKeyValues->SetString( "steamid", CFmtStr( "%llu", m_vecPlayers[i].m_steamID.ConvertToUint64() ).Access() );
		pKeyValues->SetInt( "badge_level", Max( 1U, m_vecPlayers[i].m_nBadgeLevel ) );

		ApplyChatUserSettings( m_vecPlayers[ i ], pKeyValues );

		pKeyValues->SetInt( "is_banned", ( m_vecPlayers[i].m_bIsBanned || m_vecPlayers[i].m_bIsLowPriority ) ? m_iImageIsBanned : 0 );

		// See if the avatar's changed
		int iAvatar = steamapicontext->SteamFriends()->GetSmallFriendAvatar( m_vecPlayers[i].m_steamID );
		int iIndex = m_mapAvatarsToImageList.Find( iAvatar );
		if ( iIndex == m_mapAvatarsToImageList.InvalidIndex() )
		{
			CAvatarImage *pImage = new CAvatarImage();
			pImage->SetAvatarSteamID( m_vecPlayers[i].m_steamID );
			pImage->SetDrawFriend( false ); // you can only invite friends, this isn't that useful
			pImage->SetAvatarSize( m_iAvatarWidth, m_iAvatarWidth );
			int iImageIndex = m_pImageList->AddImage( pImage );

			iIndex = m_mapAvatarsToImageList.Insert( iAvatar, iImageIndex );
		}
		pKeyValues->SetInt( "avatar", m_mapAvatarsToImageList[iIndex] );
		CAvatarImage *pAvIm = (CAvatarImage *)m_pImageList->GetImage( m_mapAvatarsToImageList[iIndex] );
		pAvIm->UpdateFriendStatus();
		m_vecPlayers[i].m_pAvatarImage = pAvIm;
		if ( bLadderGame )
		{
			if ( !m_vecPlayers[i].m_bHasCompetitiveAccess )
			{
				pKeyValues->SetInt( "has_competitive_access", m_iImageNo );
			}
		}

		pKeyValues->SetInt( "ladder_rank", m_vecPlayers[i].m_unLadderRank );
		pKeyValues->SetInt( "experience_level", m_vecPlayers[i].m_unExperienceLevel );

		int itemID = m_pChatPlayerList->AddItem( 0, pKeyValues );
		m_pChatPlayerList->SetItemFont( itemID, m_fontPlayerListItem );
		m_pChatPlayerList->SetItemFgColor( itemID, ( m_vecPlayers[i].m_bIsBanned || m_vecPlayers[i].m_bIsLowPriority ) ? s_colorBannedPlayerListItem : s_colorPlayerListItem );

		pKeyValues->deleteThis();
	}

	// force the list to PerformLayout() now so we can update our medal images
	m_pChatPlayerList->InvalidateLayout( true );

	int iPanelCount = 0;
	// This only works in 6v6 Comp and 12v12 Casual for now
	if ( ( GetMatchGroup() == k_eTFMatchGroup_Ladder_6v6 ) || ( GetMatchGroup() == k_eTFMatchGroup_Casual_12v12 ) )
	{
		int nColumn = m_pChatPlayerList->GetColumnIndexByName( 0, "rank" );

		for ( int nRow = 0; nRow < m_pChatPlayerList->GetItemCount(); nRow++ )
		{
			KeyValues *pKeyValues = m_pChatPlayerList->GetItemData( nRow );
			if ( !pKeyValues )
				continue;

			CSteamID steamID = SteamIDFromDecimalString( pKeyValues->GetString( "steamid", "0" ) );
			if ( !steamID.IsValid() )
				continue;

 			uint32 unLevel = pKeyValues->GetInt( "ladder_rank" );
			if ( GetMatchGroup() == k_eTFMatchGroup_Casual_12v12 )
			{
				unLevel = pKeyValues->GetInt( "experience_level" );
			}

			// Create a panel if we need one
			if ( iPanelCount >= m_vecChatBadges.Count() )
			{
				m_vecChatBadges.AddToTail();
				m_vecChatBadges[iPanelCount].m_pBadgeModel = vgui::SETUP_PANEL( new CTFBadgePanel( m_pChatPlayerList, "Model" ) );
				m_vecChatBadges[iPanelCount].m_pBadgeModel->SetZPos( 9999 );
				m_vecChatBadges[iPanelCount].m_nShownLevel = 0u;
			}

			// Move it into place and resize.  This is terrible, but VGUI has forced my hand
			int nX, nY, nWide, nTall;
			m_pChatPlayerList->GetMaxCellBounds( nRow, nColumn, nX, nY, nWide, nTall );
			int nSideLength = Max( nWide, nTall );
			nX = ( nX + ( nWide / 2 ) ) - ( nSideLength / 2 );
			nY = ( nY + ( nTall / 2 ) ) - ( nSideLength / 2 );

			m_vecChatBadges[iPanelCount].m_pBadgeModel->SetBounds( nX, nY, nSideLength, nSideLength );

			// Different dude in the slot or their level is different?  Update the medal model
			if ( m_vecChatBadges[iPanelCount].m_steamIDOwner != steamID ||
				m_vecChatBadges[iPanelCount].m_nShownLevel != unLevel )
			{
				m_vecChatBadges[iPanelCount].m_steamIDOwner = steamID;

				const LevelInfo_t& level = pMatchDesc->m_pProgressionDesc->GetLevelByNumber( unLevel );
				m_vecChatBadges[iPanelCount].m_pBadgeModel->SetupBadge( pMatchDesc->m_pProgressionDesc, level, &steamID );

				wchar_t wszOutString[128];
				char szLocalized[512];
				wchar_t wszCount[16];
				_snwprintf( wszCount, ARRAYSIZE( wszCount ), L"%d", level.m_nLevelNum );
				const wchar_t *wpszFormat = g_pVGuiLocalize->Find( pMatchDesc->m_pProgressionDesc->m_pszLevelToken );
				g_pVGuiLocalize->ConstructString_safe( wszOutString, wpszFormat, 2, wszCount, g_pVGuiLocalize->Find( level.m_pszLevelTitle ) );
				g_pVGuiLocalize->ConvertUnicodeToANSI( wszOutString, szLocalized, sizeof( szLocalized ) );

				m_vecChatBadges[iPanelCount].m_pBadgeModel->SetTooltip( m_pToolTip, szLocalized );
   				m_vecChatBadges[iPanelCount].m_nShownLevel = level.m_nLevelNum;
				m_vecChatBadges[iPanelCount].m_pBadgeModel->InvalidateLayout( true, true );
				m_vecChatBadges[iPanelCount].m_pBadgeModel->SetVisible( true );
			}

			iPanelCount++;
		}
	}

	for ( ; iPanelCount < m_vecChatBadges.Count(); ++iPanelCount )
	{
		m_vecChatBadges[iPanelCount].m_pBadgeModel->SetVisible( false );
		m_vecChatBadges[iPanelCount].m_nShownLevel = 0u; // Will cause the badge to refresh when it gets a player
	}
}


//-----------------------------------------------------------------------------
void CBaseLobbyPanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( GetResFile() );

	m_pSearchActiveGroupBox = dynamic_cast<vgui::EditablePanel *>(FindChildByName( "SearchActiveGroupBox", true )); Assert( m_pSearchActiveGroupBox );
		m_pSearchActiveTitleLabel = dynamic_cast<vgui::Label *>(FindChildByName( "SearchActiveTitle", true )); Assert( m_pSearchActiveTitleLabel );
		m_pSearchActivePenaltyLabel = dynamic_cast<vgui::Label *>( FindChildByName( "PartyHasLowPriorityLabel", true ) ); Assert( m_pSearchActivePenaltyLabel );

		m_pPartyHasLowPriority = dynamic_cast<vgui::EditablePanel *>( FindChildByName( "PartyHasLowPriorityGroupBox", true ) ); Assert( m_pPartyHasLowPriority );

	m_pJoinLateCheckButton = dynamic_cast<vgui::CheckButton *>(FindChildByName( "JoinLateCheckButton", true )); Assert( m_pJoinLateCheckButton );
	m_pJoinLateValueLabel = dynamic_cast<vgui::Label *>(FindChildByName( "JoinLateValueLabel", true )); Assert( m_pJoinLateValueLabel );

	m_pInviteButton = dynamic_cast<vgui::Button *>(FindChildByName( "InviteButton", true )); Assert( m_pInviteButton );

	delete m_pImageList;
	m_pImageList = new vgui::ImageList( false );

	m_iImageIsBanned = m_pImageList->AddImage( vgui::scheme()->GetImage( "pve/mvm_timeout_active", true ) );
		m_pImageList->GetImage( m_iImageIsBanned )->SetSize( m_iBannedWidth, m_iBannedWidth );
	m_iImageCheckBoxDisabled = m_pImageList->AddImage( vgui::scheme()->GetImage( "pve/mvm_check_box_disabled", true ) );
		m_pImageList->GetImage( m_iImageCheckBoxDisabled )->SetSize( m_iChallengeCheckBoxWidth, m_iChallengeCheckBoxWidth * ( 3.75f / 4.0f ) );
	m_iImageCheckBoxYes = m_pImageList->AddImage( vgui::scheme()->GetImage( "pve/mvm_check_box_yes", true ) );
		m_pImageList->GetImage( m_iImageCheckBoxYes )->SetSize( m_iChallengeCheckBoxWidth, m_iChallengeCheckBoxWidth * ( 3.75f / 4.0f ) );
	m_iImageCheckBoxNo = m_pImageList->AddImage( vgui::scheme()->GetImage( "pve/mvm_check_box_no", true ) );
		m_pImageList->GetImage( m_iImageCheckBoxNo )->SetSize( m_iChallengeCheckBoxWidth, m_iChallengeCheckBoxWidth * ( 3.75f / 4.0f )  );
	m_iImageCheckBoxMixed = m_pImageList->AddImage( vgui::scheme()->GetImage( "pve/mvm_check_box_mixed", true ) );
		m_pImageList->GetImage( m_iImageCheckBoxMixed )->SetSize( m_iChallengeCheckBoxWidth, m_iChallengeCheckBoxWidth * ( 3.75f / 4.0f )  );
	m_iImageRadioButtonYes = m_pImageList->AddImage( vgui::scheme()->GetImage( "pve/mvm_radio_button_yes", true ) );
		m_pImageList->GetImage( m_iImageRadioButtonYes )->SetSize( m_iChallengeCheckBoxWidth, m_iChallengeCheckBoxWidth * ( 3.75f / 4.0f )  );
	m_iImageRadioButtonNo = m_pImageList->AddImage( vgui::scheme()->GetImage( "pve/mvm_radio_button_no", true ) );
		m_pImageList->GetImage( m_iImageRadioButtonNo )->SetSize( m_iChallengeCheckBoxWidth, m_iChallengeCheckBoxWidth * ( 3.75f / 4.0f )  );
	m_iImageNew = m_pImageList->AddImage( vgui::scheme()->GetImage( "new", true ) );
		m_pImageList->GetImage( m_iImageNew )->SetSize( m_iNewWidth, m_iNewWidth * ( 3.75f / 4.0f )  );

	m_iImageNo = m_pImageList->AddImage( vgui::scheme()->GetImage( "hud/vote_no", true ) );

	m_mapAvatarsToImageList.RemoveAll();
	m_pChatPlayerList->SetImageList( m_pImageList, false );
	m_pChatPlayerList->SetVisible( true );


	//
	// Populate the challenge list
	//
	m_pJoinLateCheckButton->AddActionSignalTarget( this );
	m_pInviteButton->AddActionSignalTarget( this );
	m_pChatPlayerList->AddActionSignalTarget( this );

	m_fontPlayerListItem = pScheme->GetFont( "DefaultSmall", true );

	//
	// Populate the player list
	//

	m_pChatPlayerList->SetVerticalScrollbar( false );
	m_pChatPlayerList->RemoveAll();
	m_pChatPlayerList->RemoveAllSections();
	m_pChatPlayerList->AddSection( 0, "Players" );
	m_pChatPlayerList->SetSectionAlwaysVisible( 0, true );
	m_pChatPlayerList->SetSectionFgColor( 0, Color( 255, 255, 255, 255 ) );
	m_pChatPlayerList->SetBgColor( Color( 0, 0, 0, 0 ) );
	m_pChatPlayerList->SetBorder( NULL );
	m_pChatPlayerList->SetClickable( false );
	//m_pChatPlayerList->SetClickable( true ); // enable context menu to trade / kick?


#if 0 // TODO(Universal Parties): Is this totally dead code?
	bool bPartyLeader = GTFPartyClient()->BIsPartyLeader() && GCClientSystem()->BConnectedtoGC();
	if ( bPartyLeader )
	{
		extern bool TF_IsHolidayActive( int eHoliday );
		bool bHalloween = TF_IsHolidayActive( kHoliday_Halloween );
		static bool bForcedOnce = false;
		if ( bHalloween && !bForcedOnce )
		{
			GTFGCClientSystem()->SetQuickplayGameType( kGameCategory_Event247 );
			bForcedOnce = true;
		}
	}
#endif

	GTFPartyClient()->MutLocalGroupCriteria().SetLateJoin( tf_matchmaking_join_in_progress.GetBool() );
}

void CBaseLobbyPanel::PerformLayout()
{
	BaseClass::PerformLayout();

	WriteGameSettingsControls();
	UpdatePlayerList();
}

void CBaseLobbyPanel::OnItemContextMenu( vgui::Panel* panel )
{
	if ( m_iWritingPanel > 0 )
		return;
	m_pChatTextEntry->RequestFocus();
	if ( panel == m_pChatPlayerList )
	{
		OnClickedOnPlayer();
		return;
	}
}

#endif // #ifdef ENABLE_GC_MATCHMAKING
