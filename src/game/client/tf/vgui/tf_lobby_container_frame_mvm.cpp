//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#include "cbase.h"
#include "tf_party.h"
#include "tf_partyclient.h"
#include "tf_matchcriteria.h"
#include "vgui_controls/PropertySheet.h"
#include "vgui_controls/SectionedListPanel.h"
#include "tf_lobbypanel_mvm.h"

#include "tf_lobby_container_frame_mvm.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

static bool BIsCurrentCriteriaMannUp()
{
	return IsMannUpGroup( GTFPartyClient()->GetEffectiveGroupCriteria().GetMatchGroup() );
}

//-----------------------------------------------------------------------------
CLobbyContainerFrame_MvM::CLobbyContainerFrame_MvM()
	: CBaseLobbyContainerFrame( "LobbyContainerFrame" )
{
	// Our internal lobby panel
	m_pContents = new CLobbyPanel_MvM( this, this );
	m_pContents->MoveToFront();
	m_pContents->AddActionSignalTarget( this );
	AddPage( m_pContents, "#TF_Matchmaking_HeaderMvM" );
	GetPropertySheet()->SetNavToRelay( m_pContents->GetName() );
	m_pContents->SetVisible( true );
}

//-----------------------------------------------------------------------------
CLobbyContainerFrame_MvM::~CLobbyContainerFrame_MvM( void )
{
}

//-----------------------------------------------------------------------------
void CLobbyContainerFrame_MvM::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	m_pStartPartyButton = dynamic_cast<vgui::Button *>(FindChildByName( "StartPartyButton", true )); Assert( m_pStartPartyButton );
	m_pPlayNowButton = dynamic_cast<vgui::Button *>(FindChildByName( "PlayNowButton", true )); Assert( m_pPlayNowButton );
	m_pPracticeButton = dynamic_cast<vgui::Button *>(FindChildByName( "PracticeButton", true )); Assert( m_pPracticeButton );
}

//-----------------------------------------------------------------------------
/* static */ bool CLobbyContainerFrame_MvM::TypeCanHandleMatchGroup( ETFMatchGroup eMatchGroup )
{
	// All we know about
	return ( eMatchGroup == k_eTFMatchGroup_MvM_MannUp ||
	         eMatchGroup == k_eTFMatchGroup_MvM_Practice );
}

//-----------------------------------------------------------------------------
bool CLobbyContainerFrame_MvM::VerifyPartyAuthorization() const
{
	// They want to Mann Up.  Confirm that everybody in the party has a ticket.
	// if they are in a party of one, we provide slightly more specific UI.
	auto &criteria = GTFPartyClient()->GetEffectiveGroupCriteria();
	bool bBraggingRights = criteria.GetMatchGroup() == k_eTFMatchGroup_MvM_MannUp;

	// Early out. Anyone can play for free
	if ( !bBraggingRights )
		return true;

	// Solo
	CTFParty *pParty = GTFGCClientSystem()->GetParty();
	if ( pParty == NULL || pParty->GetNumMembers() <= 1 )
	{
		if ( bBraggingRights && !GTFGCClientSystem()->BLocalPlayerInventoryHasMvmTicket() )
		{
			ShowEconRequirementDialog( "#TF_MvM_RequiresTicket_Title", "#TF_MvM_RequiresTicket", CTFItemSchema::k_rchMvMTicketItemDefName );
			return false;
		}
	}
	// Group
	else
	{
		wchar_t wszLocalized[512];
		char szLocalized[512];
		wchar_t wszCharPlayerName[128];

		bool bAnyMembersWithoutAuth = false;

		if ( bBraggingRights )
		{
			for ( int i = 0 ; i < pParty->GetNumMembers() ; ++i )
			{
				if ( !pParty->Obj().members( i ).owns_ticket() )
				{
					bAnyMembersWithoutAuth = true;

					V_UTF8ToUnicode( steamapicontext->SteamFriends()->GetFriendPersonaName( pParty->GetMember( i ) ), wszCharPlayerName, sizeof( wszCharPlayerName ) );
					g_pVGuiLocalize->ConstructString_safe( wszLocalized, g_pVGuiLocalize->Find( "#TF_Matchmaking_MissingTicket" ), 1, wszCharPlayerName );
					g_pVGuiLocalize->ConvertUnicodeToANSI( wszLocalized, szLocalized, sizeof( szLocalized ) );

					// TODO(Universal Parties): (if this frame kept) GTFGCClientSystem()->SendSteamLobbyChat( CTFGCClientSystem::k_eLobbyMsg_SystemMsgFromLeader, szLocalized );
				}
			}
		}

		if ( bAnyMembersWithoutAuth )
		{
			if ( bBraggingRights )
			{
				ShowMessageBox( "#TF_MvM_RequiresTicket_Title", "#TF_MvM_RequiresTicketParty", "#GameUI_OK" );
				return false;
			}
		}
	}

	return true;
}

void CLobbyContainerFrame_MvM::HandleBackPressed()
{
	Assert( GTFPartyClient()->BIsPartyLeader() );

	if ( GTFPartyClient()->BInQueue() )
	{
		GTFPartyClient()->CancelQueueRequest();
		GTFPartyClient()->SetLocalUIState( k_eTFMatchmakingSyncedUIState_MvM_Selecting_Missions );
		return;
	}

	switch ( GTFPartyClient()->GetLeaderUIState() )
	{
		case k_eTFMatchmakingSyncedUIState_MvM_Selecting_Mode:
			GTFGCClientSystem()->EndModalMM();
			ShowPanel( false );

			return;

#ifdef USE_MVM_TOUR
		case k_eTFMatchmakingSyncedUIState_MvM_Selecting_Tour:
			GTFPartyClient()->SetLocalUIState( k_eTFMatchmakingSyncedUIState_MvM_Selecting_Mode );
			return;
#endif // USE_MVM_TOUR

		case k_eTFMatchmakingSyncedUIState_MvM_Selecting_Missions:
#ifdef USE_MVM_TOUR
			if ( BIsCurrentCriteriaMannUp() )
			{
				GTFPartyClient()->SetLocalUIState( k_eTFMatchmakingSyncedUIState_MvM_Selecting_Tour );
			}
			else
			{
				GTFPartyClient()->SetLocalUIState( k_eTFMatchmakingSyncedUIState_MvM_Selecting_Mode );
			}
#else // new mm
			GTFPartyClient()->SetLocalUIState( k_eTFMatchmakingSyncedUIState_MvM_Selecting_Mode );
#endif // USE_MVM_TOUR
			return;

		default:
			Msg( "Unexpected UI step %d", (int)GTFPartyClient()->GetLeaderUIState() );
			break;
	}
	// Unhandled case
	BaseClass::HandleBackPressed();
}

//-----------------------------------------------------------------------------
void CLobbyContainerFrame_MvM::OnCommand( const char *command )
{
	if ( FStrEq( command, "learn_more" ) )
	{
		if ( steamapicontext && steamapicontext->SteamFriends() )
		{
			steamapicontext->SteamFriends()->ActivateGameOverlayToWebPage( "http://www.teamfortress.com/mvm/" );
		}
		return;
	}
	else if ( FStrEq( command, "mannup" ) )
	{
		GTFPartyClient()->MutLocalGroupCriteria().SetMatchGroup( k_eTFMatchGroup_MvM_MannUp );
#ifdef USE_MVM_TOUR
		GTFPartyClient()->SetLocalUIState( k_eTFMatchmakingSyncedUIState_MvM_Selecting_Tour );
#else // new mm
		GTFPartyClient()->SetLocalUIState( k_eTFMatchmakingSyncedUIState_MvM_Selecting_Missions );
#endif // USE_MVM_TOUR
		return;
	}
	else if ( FStrEq( command, "practice" ) )
	{
		GTFPartyClient()->MutLocalGroupCriteria().SetMatchGroup( k_eTFMatchGroup_MvM_Practice );
		GTFPartyClient()->SetLocalUIState( k_eTFMatchmakingSyncedUIState_MvM_Selecting_Missions );
		return;
	}
	else if ( FStrEq( command, "next" ) )
	{
		switch ( GTFPartyClient()->GetLeaderUIState() )
		{
			case k_eTFMatchmakingSyncedUIState_MvM_Selecting_Mode:
#ifdef USE_MVM_TOUR
				if ( BIsCurrentCriteriaMannUp() )
				{
					GTFPartyClient()->SetLocalUIState( k_eTFMatchmakingSyncedUIState_MvM_Selecting_Tour );
				}
				else
				{
					GTFPartyClient()->SetLocalUIState( k_eTFMatchmakingSyncedUIState_MvM_Selecting_Missions );
				}
				break;

			case k_eTFMatchmakingSyncedUIState_MvM_Selecting_Tour:
#endif // USE_MVM_TOUR
				GTFPartyClient()->SetLocalUIState( k_eTFMatchmakingSyncedUIState_MvM_Selecting_Missions );
				break;

			case k_eTFMatchmakingSyncedUIState_MvM_Selecting_Missions:
				StartSearch();
				break;

			default:
				AssertMsg( false, "Unexpected UI leader state %d", (int)GTFPartyClient()->GetLeaderUIState() );
				break;
		}
		return;
	}

	BaseClass::OnCommand( command );
}

//-----------------------------------------------------------------------------
void CLobbyContainerFrame_MvM::OnKeyCodePressed(vgui::KeyCode code)
{
	//ButtonCode_t nButtonCode = GetBaseButtonCode( code );

	// TODO Brett: handle this
	//if ( nButtonCode == KEY_XBUTTON_Y )
	//{
	//	static_cast< CLobbyPanel_MvM* >( m_pContents )->ToggleSquadSurplusCheckButton();
	//}

	BaseClass::OnKeyCodePressed( code );
}


//-----------------------------------------------------------------------------
void CLobbyContainerFrame_MvM::WriteControls()
{
	// Make sure we want to be in matchmaking.  (If we don't, the frame should hide us pretty quickly.)
	// We might get an event or something right at the transition point occasionally when the UI should
	// not be visible
	if ( !GTFGCClientSystem()->BUserInModalMMUI() )
		{ return; }
	const char *pszBackButtonText = "#TF_Matchmaking_Back";
	const char *pszNextButtonText = NULL;

	CMvMMissionSet challenges;
	GTFPartyClient()->GetEffectiveGroupCriteria().GetMvMMissionSet( challenges );

	auto eLeaderState = GTFPartyClient()->GetLeaderUIState();

	bool bShowPlayNowButtons = false;

	if ( GCClientSystem()->BConnectedtoGC() )
	{
		if ( GTFPartyClient()->BIsPartyLeader()  )
		{
			if ( GTFPartyClient()->BInQueue() )
			{
				pszBackButtonText = "#TF_Matchmaking_CancelSearch";
			}
			else
			{
				switch ( eLeaderState )
				{
					case k_eTFMatchmakingSyncedUIState_Configuring_Mode:
					{
						if ( !m_pStartPartyButton->IsVisible() )
						{
							pszBackButtonText = "#TF_Matchmaking_LeaveParty";
						}
						else
						{
							pszBackButtonText = "#TF_Matchmaking_Back";
						}

						bShowPlayNowButtons = GTFPartyClient()->BIsPartyLeader();
					}
					break;

					case k_eTFMatchmakingSyncedUIState_MvM_Selecting_Tour:
					{
#ifdef USE_MVM_TOUR
						pszBackButtonText = "#TF_Matchmaking_Back";
						pszNextButtonText = "#TF_MvM_SelectChallenge";

						auto idxTour = GTFPartyClient()->GetEffectiveGroupCriteria().GetMannUpTourIndex();
						SetNextButtonEnabled( idxTour >= 0 );
#else // new mm
						AssertMsg( 0, "This is legacy code. We don't have concept of tour anymore." );
#endif // USE_MVM_TOUR
					}
					break;

					case k_eTFMatchmakingSyncedUIState_MvM_Selecting_Missions:
						pszBackButtonText = "#TF_Matchmaking_Back";

						pszNextButtonText = "#TF_Matchmaking_StartSearch";
						SetNextButtonEnabled( !challenges.IsEmpty() );

						break;

					case k_eTFMatchmakingSyncedUIState_None:
						// Still being setup
						break;

					default:
						AssertMsg( false, "Unknown wizard step %d", (int)eLeaderState );
						break;
				}
			}
		}
		else
		{
			pszBackButtonText = "#TF_Matchmaking_LeaveParty";
			m_pNextButton->SetEnabled( false );
		}
	}

	m_pPlayNowButton->SetVisible( bShowPlayNowButtons );
	m_pPracticeButton->SetVisible( bShowPlayNowButtons );
	SetControlVisible( "LearnMoreButton", eLeaderState == k_eTFMatchmakingSyncedUIState_Configuring_Mode );

	// Set appropriate page title
	if ( BIsCurrentCriteriaMannUp() ||
	     eLeaderState == k_eTFMatchmakingSyncedUIState_Configuring_Mode )
	{
		GetPropertySheet()->SetTabTitle( 0, "#TF_MvM_HeaderCoop" );
	}
	else
	{
		GetPropertySheet()->SetTabTitle( 0, "#TF_MvM_HeaderPractice" );
	}

	// Check if we already have a party, then make sure and show it
	if ( m_pStartPartyButton->IsVisible() && m_pContents->NumPlayersInParty() > 1 )
	{
		m_pContents->SetControlVisible( "PartyActiveGroupBox", true );
	}

	SetControlVisible( "PlayWithFriendsExplanation", ShouldShowPartyButton() );

	// TODO Brett: handle toggling
	//static_cast< CLobbyPanel_MvM* >( m_pContents )->SetMannUpTicketCount( GTFGCClientSystem()->GetLocalPlayerInventoryMvmTicketCount() );
	//static_cast< CLobbyPanel_MvM* >( m_pContents )->SetSquadSurplusCount( GTFGCClientSystem()->GetLocalPlayerInventorySquadSurplusVoucherCount() );

	m_pBackButton->SetText( pszBackButtonText );
	if ( pszNextButtonText )
	{
		m_pNextButton->SetText( pszNextButtonText );
		m_pNextButton->SetVisible( true );
	}
	else
	{
		m_pNextButton->SetVisible( false );
	}

	BaseClass::WriteControls();
}

