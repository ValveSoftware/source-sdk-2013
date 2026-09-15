//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//


#include "cbase.h"
#include "tf_shareddefs.h"
#include "tf_matchmaking_dashboard.h"
#include "tf_gamerules.h"
#include "ienginevgui.h"
#include "clientmode_tf.h"
#include "tf_hud_disconnect_prompt.h"
#include "tf_gc_client.h"
#include "tf_party.h"
#include <vgui/ISurface.h>
#include "vgui_avatarimage.h"
#include "tf_matchmaking_dashboard_party_member.h"
#include "tf_matchmaking_dashboard_parent_manager.h"
#include "tf_partyclient.h"
#include "tf_controls.h"
#include "softline.h"
#include "hud_controlpointicons.h"

using namespace vgui;
using namespace GCSDK;

CDashboardPartyMember::EMemberState SteamIDOfPartySlot( int nSlot, CSteamID& steamID )
{
	if ( !GTFPartyClient()->BHaveActiveParty() )
	{
		return CDashboardPartyMember::MEMBER_NONE;
	}

	if ( nSlot < GTFPartyClient()->GetNumPartyMembers() )
	{
		steamID = GTFPartyClient()->GetPartyMember( nSlot );
		return CDashboardPartyMember::MEMBER_PRESENT;
	}

	nSlot -= GTFPartyClient()->GetNumPartyMembers();

	if ( nSlot < GTFPartyClient()->GetNumOutgoingInvites() )
	{
		steamID = GTFPartyClient()->GetOutgoingInvite( nSlot );
		return CDashboardPartyMember::MEMBER_PENDING_INCOMING_JOIN_REQUEST;
	}

	nSlot -= GTFPartyClient()->GetNumIncomingInvites();

	if ( nSlot < GTFPartyClient()->GetNumIncomingJoinRequests() )
	{
		steamID = GTFPartyClient()->GetIncomingJoinRequest( nSlot );
		return CDashboardPartyMember::MEMBER_PENDING_INCOMING_JOIN_REQUEST;
	}

	//if ( nSlot < pParty->GetNumMembers() )
	//{
	//	// An occupied slot
	//	steamID = pParty->GetMember( nSlot );
	//	return CDashboardPartyMember::MEMBER_PRESENT;
	//}

	//// Our pending invites are pushed to after our occupied slots
	//nSlot -= pParty->GetNumMembers();

	//if ( nSlot >= 0 && nSlot < pParty->GetNumPendingPlayers() )
	//{
	//	// A pending invite
	//	steamID = pParty->GetPendingPlayer( nSlot );
	//	switch ( pParty->GetPendingPlayerType( nSlot ) )
	//	{
	//	case IPlayerGroup::ePending_Invite:
	//		return CDashboardPartyMember::MEMBER_PENDING_OUTGOING_INVITE;
	//	case IPlayerGroup::ePending_JoinRequest:
	//		return CDashboardPartyMember::MEMBER_PENDING_INCOMING_JOIN_REQUEST;
	//	}
	//}

	// Just an empty slot with nothing happening
	steamID = k_steamIDNil;
	return CDashboardPartyMember::MEMBER_NONE;
}

DECLARE_BUILD_FACTORY( CDashboardPartyMember );
CDashboardPartyMember::CDashboardPartyMember( Panel *parent, const char *panelName )
	: EditablePanel( parent, panelName )
{
	m_pAvatar = new CAvatarImagePanel( this, "avatar" );
	m_pAvatar->SetShouldDrawFriendIcon( false );

	SetPostChildPaintEnabled( true );

	ListenForGameEvent( "party_updated" );
	ListenForGameEvent( "world_status_changed" );
}

void CDashboardPartyMember::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "resource/UI/DashboardPartyMember.res" );
	m_pAvatar->SetMouseInputEnabled( false );
	m_pInteractButton = FindControl< CExImageButton >( "InteractButton" );
	m_pInteractButton->SetMouseClickEnabled( MOUSE_RIGHT, true );

}

void CDashboardPartyMember::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );
	m_nDisplayPartySlot = inResourceData->GetInt( "party_slot" );
}

wchar_t* FindStringSafe( const char* pszToken )
{
	static wchar_t* wszEmptyString = L"";
	auto index = g_pVGuiLocalize->FindIndex( pszToken );
	if ( index == INVALID_LOCALIZE_STRING_INDEX )
		return wszEmptyString;

	return g_pVGuiLocalize->GetValueByIndex( index );
}

void CDashboardPartyMember::PerformLayout()
{
	BaseClass::PerformLayout();

	auto pParty = GTFPartyClient()->GetActiveParty();

	// Update our slot state and steamID.
	UpdatePartyMemberSteamID();

	CUtlString strTipText;
	bool bLine = false;
	// Helper lambda
	auto lambdaAddTipText = [&]( const char* pszText, bool bNewLine = true )
	{
		if ( bLine && bNewLine )
			strTipText.Append( "\n" );
		bLine = true;
		strTipText.Append( pszText );
	};

	bool bShowBannedIcon = false;
	bool bOffline = false;
	bool bOutOfDate = false;

	// Setup the tooltip
	if ( m_eMemberState != MEMBER_NONE )
	{
		// First their name
		lambdaAddTipText( SteamFriends()->GetFriendPersonaName( m_steamIDPartyMember ) );

		bool bHyphenAdded = false;

		// Leader state
		if ( BMemberIsLeader() )
		{
			lambdaAddTipText( " - ", false );
			bHyphenAdded = true;
			lambdaAddTipText( CStrAutoEncode( FindStringSafe( "#TF_Matchmaking_PartyMember_Leader" ) ).ToString(), false );
		}

		if ( m_eMemberState == MEMBER_PRESENT )
		{
			auto memberStatus = GTFPartyClient()->GetPartyMemberStatus( m_nBackendPartySlot );
			bOffline = !memberStatus.bOnline;

			if ( bOffline )
			{
				lambdaAddTipText( CStrAutoEncode( FindStringSafe( "#TF_Matchmaking_PartyMember_Offline" ) ).ToString(), true );
			}

			bOutOfDate = memberStatus.bOutOfDate;

			if ( bOutOfDate )
			{
				lambdaAddTipText( CStrAutoEncode( FindStringSafe( "#TF_Matchmaking_PartyMember_OutOfDate" ) ).ToString(), true );
			}
		}

		// Casual ban, if they are
		if ( ( pParty && m_nDisplayPartySlot < pParty->GetNumMembers() && pParty->BMembersIsBanned( m_steamIDPartyMember, eMMPenaltyPool_Casual ) ) ||
			 ( BIsLocalPlayerSlot() && GTFGCClientSystem()->BIsBannedFromMatchmaking( eMMPenaltyPool_Casual ) ) )
		{
			lambdaAddTipText( CStrAutoEncode( FindStringSafe( "#TF_Matchmaking_PartyMember_CasualBanned" ) ).ToString() );
			bShowBannedIcon = true;
		}

		// Comp ban, if they are
		if ( ( pParty && m_nDisplayPartySlot < pParty->GetNumMembers() && pParty->BMembersIsBanned( m_steamIDPartyMember, eMMPenaltyPool_Ranked ) ) || 
			( BIsLocalPlayerSlot() && GTFGCClientSystem()->BIsBannedFromMatchmaking( eMMPenaltyPool_Ranked ) ) )
		{
			lambdaAddTipText( CStrAutoEncode( FindStringSafe( "#TF_Matchmaking_PartyMember_CompetitiveBanned" ) ).ToString() );
			bShowBannedIcon = true;
		}
		
		m_pAvatar->SetPlayer( m_steamIDPartyMember, k_EAvatarSize64x64 );


	}

	// The avatar is shown when we know who could be in the slot
	m_pAvatar->SetVisible( m_eMemberState != MEMBER_NONE );
	// Leader icon to indicate the leader.  Don't show when it's a party of 1
	SetControlVisible( "LeaderIcon", BMemberIsLeader() && 
								     pParty != NULL &&
								     pParty->GetNumMembers() > 1, true );

	// If an invite is pending, show a spinner
	SetControlVisible( "Spinner", m_eMemberState == MEMBER_PENDING_OUTGOING_INVITE || m_eMemberState == MEMBER_PENDING_INCOMING_JOIN_REQUEST, true );

	// Offline status
	{
		// Offline takes priority over banned
		bShowBannedIcon = bShowBannedIcon && !bOffline;

		// Banned players have a special icon
		SetControlVisible( "BannedIcon", bShowBannedIcon );

		// Offline players have a special icon
		SetControlVisible( "OfflineIcon", bOffline );
		
	}

	// Out of date status
	{
		// Offline takes priority over out of date
		bOutOfDate = bOutOfDate && !bOffline;

		SetControlVisible( "OutOfDateIcon", bOutOfDate );
	}

	SetControlVisible( "StatusDimmer", bOffline || bShowBannedIcon || bOutOfDate );

	// Place the tooltip text into the button now that we're done aggregating status
	if ( strTipText.IsEmpty() )
	{
		m_pInteractButton->SetTooltip( NULL, NULL );
	}
	else
	{
		m_pInteractButton->SetTooltip( GetDashboardTooltip( k_eSmallFont ), strTipText );
	}
}

void CDashboardPartyMember::PostChildPaint()
{
	BaseClass::PostChildPaint();

	if ( m_eMemberState != MEMBER_NONE )
	{
		// When in a party, draw a line with the member color
		BGeneralPaintSetup( GetMMDashboard()->GetPartyMemberColor( m_nDisplayPartySlot ) );

		int nYInset = YRES( 1 );
		int nXInset = 1;
		Vertex_t vtxStart( Vector2D( nXInset, GetTall() - nYInset ), Vector2D( 0, 0 ) );
		Vertex_t vtxEnd( Vector2D( GetWide() - nXInset, GetTall() - nYInset ), Vector2D( 0, 0 ) );

		SoftLine::DrawPolygonLine( vtxStart, vtxEnd, 2 ); 
	}
}

void CDashboardPartyMember::FireGameEvent( IGameEvent *pEvent )
{
	if ( FStrEq( pEvent->GetName(), "party_updated" ) ||
		 FStrEq( pEvent->GetName(), "world_status_changed" ) )
	{
		// TODO: If a member joined or left
		InvalidateLayout();
	}
}

bool CDashboardPartyMember::BMemberIsLeader() const
{
	auto pParty = GTFPartyClient()->GetActiveParty();
	if ( pParty )
	{
		return pParty->GetLeader() == m_steamIDPartyMember;
	}

	return false;
}

void CDashboardPartyMember::UpdatePartyMemberSteamID()
{
	EMemberState eOldState = m_eMemberState;
	m_eMemberState = MEMBER_NONE;

	if ( !steamapicontext || !steamapicontext->SteamUser() )
		return;

	CSteamID steamIDLocalPlayer = steamapicontext->SteamUser()->GetSteamID();

	if ( m_nDisplayPartySlot == 0 )
	{
		// 0 is special.  0 is always us.
		m_steamIDPartyMember = steamIDLocalPlayer;
		m_eMemberState = MEMBER_PRESENT;
	}
	else
	{
		// Get whoever is in our slot
		m_eMemberState = SteamIDOfPartySlot( m_nDisplayPartySlot, m_steamIDPartyMember );	

		// If we just found ourselves in this slot, then get who is in slot 0 so that
		// we always appear to be slot 0
		if ( m_steamIDPartyMember == steamIDLocalPlayer )
		{
			m_eMemberState = SteamIDOfPartySlot( 0, m_steamIDPartyMember );	
		}
	}

	m_nBackendPartySlot = 0;
	if ( GTFPartyClient()->BHaveActiveParty() )
	{
		for ( int nSlot = 0; nSlot < GTFPartyClient()->GetNumPartyMembers(); ++nSlot )
		{
			if ( m_steamIDPartyMember == GTFPartyClient()->GetPartyMember( nSlot ) )
			{
				m_nBackendPartySlot = nSlot;
				break;
			}
		}
	}

	if ( eOldState != m_eMemberState && !BIsLocalPlayerSlot() )
	{
		auto lambdaCreateSwoop = [&]()
		{
			if ( BIsLocalPlayerSlot() )
				return;

			int nX, nY;
			GetPos( nX, nY );
			nY += GetTall();
			ParentLocalToScreen( nX, nY );

			CreateSwoop( nX, nY, GetWide(), YRES( 200 ), 0.f, false );
		};

		switch( m_eMemberState )
		{
			case MEMBER_NONE: break;
			case MEMBER_PENDING_OUTGOING_INVITE:
			{
				// Just a swoop
				lambdaCreateSwoop();
				break;
			}

			case MEMBER_PENDING_INCOMING_JOIN_REQUEST:
			{
				// Swoop!
				lambdaCreateSwoop();
				break;
			}

			case MEMBER_PRESENT:
			{
				// Swoop!
				lambdaCreateSwoop();
				break;
			}
		}
	}
}

void CDashboardPartyMember::OnCommand( const char *command )
{
	if ( FStrEq( command, "interact" ) )
	{
		// They clicked on the portrait.  We want to pop up a context menu filled
		// with the different actions they can take on this user.  First we need
		// to figure out what they're allowed to do.

		Menu* pMenu = GetMMDashboard()->ClearAndGetDashboardContextMenu();

		MenuBuilder contextMenuBuilder( pMenu, this );
		// We're going to get goofy with the if blocks, but the ordering is important to look good

		auto *pParty = GTFPartyClient()->GetActiveParty();
		if ( pParty )
		{
			// For slots that have members in them, but not our slot
			if ( !BIsLocalPlayerSlot() && ( m_nDisplayPartySlot < pParty->GetNumMembers() + pParty->GetNumPendingPlayers() ) )
			{
				switch ( m_eMemberState )
				{
					case MEMBER_PRESENT:
					{
						// You can always send a message to party members
						contextMenuBuilder.AddMenuItem( "#TF_Friends_SendMessage", new KeyValues( "Context_SendMessage" ), "party" );

						// If we're the leader, we can kick
						if ( GTFPartyClient()->BIsPartyLeader() )
						{
							contextMenuBuilder.AddMenuItem( "#TF_MM_KickFromParty", new KeyValues( "Context_KickFromParty" ), "party" );
							contextMenuBuilder.AddMenuItem( "#TF_MM_PromoteToLeader", new KeyValues( "Context_PromoteToLeader" ), "party" );
						}

						// TODO: Make this be a thing
						bool bIsInAJoinableMatch = false;
						if ( bIsInAJoinableMatch )
						{
							contextMenuBuilder.AddMenuItem( "#TF_Friends_JoinServer", new KeyValues( "Context_JoinServer" ), "party" );
						}
						break;
					}

					case MEMBER_PENDING_INCOMING_JOIN_REQUEST:
					case MEMBER_PENDING_OUTGOING_INVITE:
					case MEMBER_NONE:
					{
						// TODO: Invite dialog that shows friends list?
					}
				}
			}
		}

		contextMenuBuilder.AddMenuItem( "#TF_MM_OpenSettings", new KeyValues( "Context_OpenSettings" ), "self" );

		if ( pParty && pParty->GetNumMembers() > 1 )
		{
			// Always the option to leave party if in a party, regardless of who you click on
			contextMenuBuilder.AddMenuItem( "#TF_Matchmaking_RollingQueue_LeaveParty", new KeyValues( "Context_LeaveParty" ), "self" );
		}

		// Position to the bottom left of us
		int nX = 0, nY = GetTall();
		LocalToScreen( nX, nY );
		pMenu->SetPos( nX, nY );	
		
		pMenu->SetVisible(true);
		pMenu->AddActionSignalTarget(this);
		pMenu->MoveToFront();

		return;
	}
}

void CDashboardPartyMember::DoLeavyParty()
{
	Assert( GTFPartyClient()->GetActiveParty() );
	if ( GTFPartyClient()->GetActiveParty() )
	{
		GTFPartyClient()->LeaveActiveParty();
	}
}

void CDashboardPartyMember::DoKickFromParty()
{
	Assert( m_eMemberState == MEMBER_PRESENT );
	Assert( !BIsLocalPlayerSlot() );
	Assert( GTFPartyClient()->BControllingPartyActions() );
	if ( m_eMemberState == MEMBER_PRESENT &&
		 !BIsLocalPlayerSlot() &&
		 GTFPartyClient()->BControllingPartyActions() )
	{
		GTFPartyClient()->BKickPartyMember( m_steamIDPartyMember );
	}
}

void CDashboardPartyMember::DoJoinServer()
{
	Assert( m_eMemberState == MEMBER_PRESENT );
	Assert( !BIsLocalPlayerSlot() );
	if ( m_eMemberState == MEMBER_PRESENT && !BIsLocalPlayerSlot() )
	{
		// TODO: Potentially prompt to disconnect, then join
	}
}

void CDashboardPartyMember::DoSendMessage()
{
	Assert( m_eMemberState == MEMBER_PRESENT );
	Assert( !BIsLocalPlayerSlot() );
	if ( m_eMemberState == MEMBER_PRESENT && !BIsLocalPlayerSlot() )
	{
		steamapicontext->SteamFriends()->ActivateGameOverlayToUser( "chat", m_steamIDPartyMember );
	}
}

void CDashboardPartyMember::DoOpenSettings()
{
	// Open up the settings!
	PostMessage( GetMMDashboard(), new KeyValues( "Context_OpenSettings" ) );	
}

void CDashboardPartyMember::DoPromoteToLeader()
{
	Assert( GTFPartyClient()->BControllingPartyActions() );
	Assert( !BIsLocalPlayerSlot() );
	GTFPartyClient()->BPromoteToLeader( m_steamIDPartyMember );
}
