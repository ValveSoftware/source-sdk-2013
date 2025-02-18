//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//


#include "cbase.h"
#include "tf_gc_client.h"
#include "tf_friends_panel.h"
#include "vgui_avatarimage.h"
#include "vgui_controls/Menu.h"
#include "vgui/IInput.h"
#include "tf_partyclient.h"
#include "tf_party.h"
#include "ienginevgui.h"
#include "clientmode_tf.h"

using namespace vgui;
using namespace GCSDK;

bool BSteamIDIsPlayingTF2( const CSteamID& steamID )
{
	if ( !steamapicontext || !steamapicontext->SteamFriends() )
		return false;

	FriendGameInfo_t gameInfo;
	if ( steamapicontext->SteamFriends()->GetFriendGamePlayed( steamID, &gameInfo ) )
	{
		if ( gameInfo.m_gameID.AppID() == (uint32)engine->GetAppID() )
		{
			return true;
		}
	}

	return false;
}

CSteamFriendPanel::CSteamFriendPanel( Panel *parent, const char *panelName )
	: BaseClass( parent, panelName )
{
	m_pAvatar = new CAvatarImagePanel( this, "avatar" );
	m_pAvatar->SetShouldScaleImage( true );
	m_pAvatar->SetShouldDrawFriendIcon( false );

	m_pStatusLabel = new Label( this, "StatusLabel", (const char*)NULL );
	m_pStatusLabel->AddActionSignalTarget( this );

	m_pInteractButton = new CExButton( this, "InteractButton", (const char*)NULL );
	m_pInteractButton->SetMouseClickEnabled( MOUSE_RIGHT, true );
}

void CSteamFriendPanel::SetSteamID( const CSteamID& steamID )
{
	m_steamID = steamID;
	m_pAvatar->SetPlayer( steamID, k_EAvatarSize64x64 );
	UpdateControls();
}

void CSteamFriendPanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );
	LoadControlSettings( "resource/UI/SteamFriendPanel.res" );
}

void CSteamFriendPanel::PerformLayout()
{
	UpdateControls();
	BaseClass::PerformLayout();
}

void CSteamFriendPanel::OnCommand( const char *command )
{
	if ( FStrEq( "open_menu", command ) )
	{
		if ( m_pContextMenu )
			delete m_pContextMenu;

		m_pContextMenu = new Menu( this, "ContextMenu" );
		MenuBuilder contextMenuBuilder( m_pContextMenu, this );
		const char *pszContextMenuBorder = "NotificationDefault";
		const char *pszContextMenuFont = "HudFontMediumSecondary";
		m_pContextMenu->SetBorder( scheme()->GetIScheme( GetScheme() )->GetBorder( pszContextMenuBorder ) );
		m_pContextMenu->SetFont( scheme()->GetIScheme( GetScheme() )->GetFont( pszContextMenuFont, IsProportional() ) );

		FriendGameInfo_t gameInfo;
		if ( steamapicontext->SteamFriends()->GetFriendGamePlayed( m_steamID, &gameInfo ) )
		{
			if ( gameInfo.m_gameID.AppID() == (uint32)engine->GetAppID() )
			{
				bool bTheyAreInACommunityServer = false;
				if ( bTheyAreInACommunityServer )
				{
					contextMenuBuilder.AddMenuItem( "#TF_Friends_JoinServer", new KeyValues( "Context_JoinServer" ), "server" );
				}

				// You can join someone if you're not in a party, or in a party of just you
				if ( GTFPartyClient()->BCanRequestToJoinPlayer( m_steamID ) && GTFPartyClient()->GetNumPartyMembers() == 1 )
				{
					contextMenuBuilder.AddMenuItem( "#TF_Friends_JoinParty", new KeyValues( "Context_JoinParty" ), "party" );
				}
			}
		}

		EPersonaState eState = steamapicontext->SteamFriends()->GetFriendPersonaState( m_steamID );
		if ( eState != k_EPersonaStateOffline )
		{
			// You can always invite your online friends
			if ( GTFPartyClient()->BCanInvitePlayer( m_steamID ) )
			{
				contextMenuBuilder.AddMenuItem( "#TF_Friends_InviteParty", new KeyValues( "Context_InviteParty" ), "party" );
			}
			contextMenuBuilder.AddMenuItem( "#TF_Friends_SendMessage", new KeyValues( "Context_SendMessage" ), "chat" );
		}

		// Position to the cursor's position
		int nX, nY;
		g_pVGuiInput->GetCursorPosition( nX, nY );
		m_pContextMenu->SetPos( nX - 1, nY - 1 );

		m_pContextMenu->SetVisible(true);
		m_pContextMenu->AddActionSignalTarget(this);
	}
}

void CSteamFriendPanel::DoJoinParty()
{
	// TODO(Universal Parties): We should pass bExpectingExistingInvite when we are doing so
	GTFPartyClient()->BRequestJoinPlayer( m_steamID, /* bExpectingExistingInvite */ false );
}

void CSteamFriendPanel::DoJoinServer()
{
	// TODO: Prompt to disconnect, potentially, then join
}

void CSteamFriendPanel::DoInviteToParty()
{
	GTFPartyClient()->BInvitePlayerToParty( m_steamID, false );
}

void CSteamFriendPanel::DoSendMessage()
{
	steamapicontext->SteamFriends()->ActivateGameOverlayToUser( "chat", m_steamID );
}

void CSteamFriendPanel::UpdateControls()
{
	const Color& colorInTF2 = vgui::scheme()->GetIScheme( GetScheme() )->GetColor( "CreditsGreen", Color( 255, 255, 255, 255 ) );
	const Color& colorOnline = vgui::scheme()->GetIScheme( GetScheme() )->GetColor( "ProgressBarBlue", Color( 255, 255, 255, 255 ) );
	const Color& colorOther = vgui::scheme()->GetIScheme( GetScheme() )->GetColor( "QuestMap_InactiveGrey", Color( 255, 255, 255, 255 ) );

	CUtlString strName = "...";
	wchar_t wzRichPresenceBuf[256] = { 0 };
	wchar_t *pwzStatus = nullptr;


	// Setup controls
	if( m_steamID.IsValid() && steamapicontext && steamapicontext->SteamFriends() )
	{
		ISteamFriends *pSteamFriends = steamapicontext->SteamFriends();
		strName = pSteamFriends->GetFriendPersonaName( m_steamID );

		// Playing TF2 is sorted to the top
		FriendGameInfo_t gameInfo;
		if ( pSteamFriends->GetFriendGamePlayed( m_steamID, &gameInfo ) )
		{
			// If the friend is playing TF2, say so.  Other games we'll show as "Playing other game"
			if ( gameInfo.m_gameID.AppID() == (uint32)engine->GetAppID() )
			{
				const char *pszRichState = pSteamFriends->GetFriendRichPresence( m_steamID, "state" );
				const char *pszRichMatchGroupLoc = pSteamFriends->GetFriendRichPresence( m_steamID, "matchgrouploc" );
				const char *pszRichMap = pSteamFriends->GetFriendRichPresence( m_steamID, "currentmap" );
				// If they have at least state, see if we can build a status from the keys
				if ( pszRichState && pszRichState[0] &&
				     GetClientModeTFNormal()->BuildRichPresenceStatus( wzRichPresenceBuf, pszRichState,
				                                                       pszRichMatchGroupLoc, pszRichMap ) )
				{
					pwzStatus = wzRichPresenceBuf;
				}
				else
				{
					// Show generic
					pwzStatus = g_pVGuiLocalize->Find( "#TF_Friends_PlayingTF2" );
				}
				m_pStatusLabel->SetFgColor( colorInTF2 );
				m_pInteractButton->SetVisible( true );
			}
			else
			{
				pwzStatus = g_pVGuiLocalize->Find( "#TF_Friends_PlayingOther" );
				m_pStatusLabel->SetFgColor( colorOnline );
				m_pInteractButton->SetVisible( true );
			}
		}
		else
		{
			EPersonaState eState = pSteamFriends->GetFriendPersonaState( m_steamID );

			// Set label text
			switch ( eState )
			{
				case k_EPersonaStateOffline: pwzStatus = g_pVGuiLocalize->Find( "#TF_Friends_Offline" ); break;
				case k_EPersonaStateOnline: pwzStatus = g_pVGuiLocalize->Find( "#TF_Friends_Online" ); break;
				case k_EPersonaStateBusy: pwzStatus = g_pVGuiLocalize->Find( "#TF_Friends_Busy" ); break;
				case k_EPersonaStateAway: pwzStatus = g_pVGuiLocalize->Find( "#TF_Friends_Away" ); break;
				case k_EPersonaStateSnooze: pwzStatus = g_pVGuiLocalize->Find( "#TF_Friends_Snooze" ); break;
			}

			// Extra control settings
			switch( eState )
			{
				case k_EPersonaStateOffline:
					m_pStatusLabel->SetFgColor( colorOther );
					break;
				default:
					m_pStatusLabel->SetFgColor( colorOnline );
			}

			m_pInteractButton->SetVisible( eState != k_EPersonaStateOffline );
		}
	}

	SetDialogVariable( "name", strName );
	SetDialogVariable( "status", pwzStatus );
}

DECLARE_BUILD_FACTORY( CSteamFriendsListPanel );
CSteamFriendsListPanel::CSteamFriendsListPanel( Panel *parent, const char *panelName )
	: BaseClass( parent, panelName )
	, m_sPersonaStateChangedCallback( this, &CSteamFriendsListPanel::OnPersonaStateChanged )
	, m_sRichPresenceStateChangedCallback( this, &CSteamFriendsListPanel::OnRichPresenceChanged )
{
	m_mapFriendsPanels.SetLessFunc( DefLessFunc( CSteamID ) );
	m_mapKnownFriends.SetLessFunc( DefLessFunc( CSteamID ) );
}

CSteamFriendsListPanel::~CSteamFriendsListPanel()
{
	if ( m_pKVFriendPanel )
	{
		m_pKVFriendPanel->deleteThis();
		m_pKVFriendPanel = NULL;
	}
}

void CSteamFriendsListPanel::PerformLayout()
{
	PositionFriendsList();

	BaseClass::PerformLayout();
}

void CSteamFriendsListPanel::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );

	tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s", __FUNCTION__ );
	FOR_EACH_MAP_FAST( m_mapFriendsPanels, i )
	{
		m_mapFriendsPanels[ i ]->MarkForDeletion();
	}
	m_mapFriendsPanels.Purge();
	InvalidateLayout();

	KeyValues* pKVFriendPanelSettings = inResourceData->FindKey( "friendpanel_kv" );
	if ( pKVFriendPanelSettings )
	{
		if ( m_pKVFriendPanel )
		{
			m_pKVFriendPanel->deleteThis();
			m_pKVFriendPanel = NULL;
		}

		m_pKVFriendPanel = pKVFriendPanelSettings->MakeCopy();
	}

	DirtyPotentialFriendsList();
}

void CSteamFriendsListPanel::OnThink()
{
	if ( !enginevgui->IsGameUIVisible() )
		return;

	if ( m_bListDirty )
	{
		ProcessFriends();
	}

	if ( !m_bListDirty && m_bPanelsDirty )
	{
		// Only update the friends when visible, and only one at a time
		UpdateFriendsList();
	}

	if ( m_fNextSortTime < Plat_FloatTime() && !m_bPanelsDirty )
	{
		m_bListNeedsResort = true;
		m_fNextSortTime = FLT_MAX;
	}

	if ( m_bListNeedsResort && !m_bPanelsDirty )
	{
		InvalidateLayout( true );
		m_bListNeedsResort = false;
	}
}

void CSteamFriendsListPanel::PositionFriendsList()
{
	tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s", __FUNCTION__ );

	CUtlVector< CSteamFriendPanel* > vecSortedFriends;
	GetSortedFriends( vecSortedFriends );

	FOR_EACH_VEC( vecSortedFriends, i )
	{
		CSteamFriendPanel* pFriend = vecSortedFriends[ i ];
		int nX = ( ( i % m_nNumColumns ) * ( pFriend->GetWide() + m_nColumnGap ) ) + m_nXInset;
		int nY = ( ( i / m_nNumColumns ) * ( pFriend->GetTall() + m_nRowGap ) ) + m_nYInset - GetScrollAmount();

		pFriend->SetPos( nX, nY );
	
		pFriend->SetVisible( true );
	}
}

void CSteamFriendsListPanel::UpdateFriendsList()
{
	if ( !steamapicontext || !steamapicontext->SteamFriends() )
		return;

	if ( !m_bPanelsDirty )
		return;

	tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s", __FUNCTION__ );
	
	auto pSteamFriends = steamapicontext->SteamFriends();
	bool bDone = false;
	
	const double flStart = Plat_FloatTime();
	while( m_nLastProcessedPotentialFriend < m_mapKnownFriends.Count() &&
		   Plat_FloatTime() - flStart < 0.0016 )
	{
		PotentialFriend_t potentialFriend = m_mapKnownFriends[ m_nLastProcessedPotentialFriend ];

		// This is the last friend!  We'll need a re-sort after this
		if ( m_nLastProcessedPotentialFriend == m_mapKnownFriends.Count() - 1 )
		{
			bDone = true;
		}

		++m_nLastProcessedPotentialFriend;
			
		CSteamID steamIDFriend = potentialFriend.m_steamID;

		// Check if we've already got a panel for this guy
		auto nExistingIndex = m_mapFriendsPanels.Find( steamIDFriend );

		// Already have a panel.  Maybe delete it
		if ( nExistingIndex != m_mapFriendsPanels.InvalidIndex() )
		{
			// If we're no longer friends with this steamID, remove their panel
			if ( pSteamFriends->GetFriendRelationship( steamIDFriend ) != k_EFriendRelationshipFriend ||
					potentialFriend.bShow == false )
			{
				m_mapFriendsPanels[ nExistingIndex ]->MarkForDeletion();
				m_mapFriendsPanels.RemoveAt( nExistingIndex );
				m_bListNeedsResort = true;
			}
		}
		else
		{
			// Some friends we might not want to show
			if ( potentialFriend.bShow == false )
				continue;

			CSteamFriendPanel* pNewFriendPanel = new CSteamFriendPanel( this, CFmtStr( "%s", pSteamFriends->GetFriendPersonaName( steamIDFriend ) ) );
			if ( m_pKVFriendPanel )
			{
				pNewFriendPanel->ApplySettings( m_pKVFriendPanel );
			}
			pNewFriendPanel->SetSteamID( steamIDFriend );
			pNewFriendPanel->MakeReadyForUse();
			pNewFriendPanel->SetVisible( false );
			m_mapFriendsPanels.Insert( steamIDFriend, pNewFriendPanel );
			m_fNextSortTime = Plat_FloatTime() + 0.1;
		}
	}

	if ( bDone )
	{
		m_bPanelsDirty = false;

		// Reconcile panels to friends
		FOR_EACH_MAP_FAST( m_mapFriendsPanels, idxPanel )
		{
			auto pFriendPanel = m_mapFriendsPanels[ idxPanel ];
			auto idxFriend = m_mapKnownFriends.Find( pFriendPanel->GetFriendSteamID() );

			// Panel does not represent a potential friend.  Let's clean it up
			if ( idxFriend == m_mapKnownFriends.InvalidIndex() || !m_mapKnownFriends[ idxFriend ].bShow )
			{
				m_mapFriendsPanels[ idxPanel ]->MarkForDeletion();
				m_mapFriendsPanels.RemoveAt( idxPanel );
				idxPanel = 0;
				m_bListNeedsResort = true;
			}
		}
	}
}

void CSteamFriendsListPanel::GetSortedFriends( CUtlVector< CSteamFriendPanel* >& vecSortedFriends ) const
{
	if ( !steamapicontext || !steamapicontext->SteamFriends() )
		return;

	tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s", __FUNCTION__ );

	FOR_EACH_MAP_FAST( m_mapFriendsPanels, i )
	{
		vecSortedFriends.AddToTail( m_mapFriendsPanels[ i ] );
	}

	auto pSteamFriends = steamapicontext->SteamFriends();

	// Sort the list as follows:
	// Playing TF > Online > Busy > Away > Snooze > Offline
	// Within the above buckets alphabetize
	vecSortedFriends.SortPredicate( [&pSteamFriends]( const CSteamFriendPanel* pLeft, const CSteamFriendPanel* pRight )
	{
		EPersonaState eLeftState = pSteamFriends->GetFriendPersonaState( pLeft->GetFriendSteamID() );
		EPersonaState eRightState = pSteamFriends->GetFriendPersonaState( pRight->GetFriendSteamID() );

		bool bLeftPlayingTF = BSteamIDIsPlayingTF2( pLeft->GetFriendSteamID() );
		bool bRightPlayingTF = BSteamIDIsPlayingTF2( pRight->GetFriendSteamID() );

		if ( bLeftPlayingTF != bRightPlayingTF )
		{
			if ( bLeftPlayingTF )
				return true;
			else
				return false;
		}

		std::string strLeftName = pSteamFriends->GetFriendPersonaName( pLeft->GetFriendSteamID() );
		std::string strRightName = pSteamFriends->GetFriendPersonaName( pRight->GetFriendSteamID() );

		bool bAlphaLeft = V_strcasecmp( strLeftName.c_str(), strRightName.c_str() ) < 0;

		if ( eLeftState != eRightState )
		{
			if ( eLeftState == k_EPersonaStateOnline )	return true;
			if ( eRightState == k_EPersonaStateOnline ) return false;
			if ( eLeftState == k_EPersonaStateBusy )	return true;
			if ( eRightState == k_EPersonaStateBusy )	return false;
			if ( eLeftState == k_EPersonaStateAway )	return true;
			if ( eRightState == k_EPersonaStateAway )	return false;
			if ( eLeftState == k_EPersonaStateSnooze )	return true;
			if ( eRightState == k_EPersonaStateSnooze )	return false;
		}

		return bAlphaLeft;
	} );
}

void CSteamFriendsListPanel::DirtyPotentialFriendsList()
{
	m_bListDirty = true;
	m_nLastProcessedFriendIndex = 0;	
	m_mapKnownFriends.Purge();
}

void CSteamFriendsListPanel::DirtyFriendsPanelsList()
{
	m_bPanelsDirty = true;
	m_nLastProcessedPotentialFriend = 0;
}

void CSteamFriendsListPanel::ProcessFriends()
{
	if ( !m_bListDirty )
		return;

	tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s", __FUNCTION__ );
	if ( !steamapicontext || !steamapicontext->SteamFriends() )
		return;

	auto pSteamFriends = steamapicontext->SteamFriends();
	uint32 nNumFriends = pSteamFriends->GetFriendCount( k_EFriendFlagImmediate );

	double flStart = Plat_FloatTime();
	while( Plat_FloatTime() - flStart < 0.0016 )
	{
		if ( m_nLastProcessedFriendIndex >= nNumFriends )
		{
			PruneKnownFriends();
			m_bListDirty = false;
			return;
		}

		// Collect all friends
		CSteamID steamIDFriend = pSteamFriends->GetFriendByIndex( m_nLastProcessedFriendIndex, k_EFriendFlagImmediate );
		EPersonaState eState = pSteamFriends->GetFriendPersonaState( steamIDFriend );
		if ( eState != k_EPersonaStateOffline )
		{
			PotentialFriend_t& potentialFriend = m_mapKnownFriends[ m_mapKnownFriends.Insert( steamIDFriend ) ];
			potentialFriend.m_steamID = steamIDFriend;
			potentialFriend.eState = eState;
		}
	
		++m_nLastProcessedFriendIndex;

		InvalidateLayout();
	}
}

void CSteamFriendsListPanel::PruneKnownFriends()
{
	const uint32 knMaxFriendPanels = 64;

	// Everyone gets to show, initially
	FOR_EACH_MAP_FAST( m_mapKnownFriends, i )
	{
		m_mapKnownFriends[ i ].bShow = true;
	}

	// Some people have A LOT of friends (>1000), but we don't want to have A LOT of panels.
	// Go through and prune friends who are offline, snooze, away, busy until we've
	// got a reasonable amount of friends to show.  We always show your online friends.
	auto lambdaPruneIfState = [ & ]( EPersonaState eState )
	{
		if ( m_mapKnownFriends.Count() <= knMaxFriendPanels )
			return;

		FOR_EACH_MAP_FAST( m_mapKnownFriends, i )
		{
			if ( m_mapKnownFriends[ i ].eState == eState )
			{
				m_mapKnownFriends[ i ].bShow = false;
			}
		}
	};

	lambdaPruneIfState( EPersonaState::k_EPersonaStateOffline );
	lambdaPruneIfState( EPersonaState::k_EPersonaStateSnooze );
	lambdaPruneIfState( EPersonaState::k_EPersonaStateAway );
	lambdaPruneIfState( EPersonaState::k_EPersonaStateBusy );

	// STILL too many friends? Prune everyone who isn't playing TF2
	if ( m_mapKnownFriends.Count() > knMaxFriendPanels )
	{
		FOR_EACH_MAP_FAST( m_mapKnownFriends, i )
		{
			if ( !BSteamIDIsPlayingTF2( m_mapKnownFriends[ i ].m_steamID ) )
			{
				m_mapKnownFriends[ i ].bShow = false;
			}
		}
	}

	DirtyFriendsPanelsList();
}

void CSteamFriendsListPanel::FriendStateChange( CSteamID steamIDFriend )
{
	if ( m_bListDirty )
	{
		// Start over so it catches the change
		DirtyPotentialFriendsList();
		return;
	}

	if ( m_bPanelsDirty )
	{
		// Start over so it catches the change
		DirtyFriendsPanelsList();
		return;
	}

	// Don't create a panel for ourselves!
	if ( !steamapicontext->SteamUser() || 
		 steamIDFriend == steamapicontext->SteamUser()->GetSteamID().ConvertToUint64() )
		return;

	auto idxFriend = m_mapKnownFriends.Find( steamIDFriend );
	if ( idxFriend == m_mapKnownFriends.InvalidIndex() )
	{
		DirtyPotentialFriendsList();
		return;
	}

	auto pSteamFriends = steamapicontext->SteamFriends();
	if ( !pSteamFriends )
		return;

	m_mapKnownFriends[ idxFriend ].eState = pSteamFriends->GetFriendPersonaState( m_mapKnownFriends[ idxFriend ].m_steamID );
	// Need to re-prune, which will re-dirty the panels and cause them to update
	PruneKnownFriends();

	auto idxPanel = m_mapFriendsPanels.Find( steamIDFriend );
	if ( idxPanel == m_mapFriendsPanels.InvalidIndex() )
	{
		DirtyFriendsPanelsList();
		return;
	}

	// Update rich presence text
	m_mapFriendsPanels[ idxPanel ]->InvalidateLayout();

	m_bListNeedsResort = true;
}

void CSteamFriendsListPanel::OnPersonaStateChanged( PersonaStateChange_t *info )
{
	// These flags indicate we need to re-do everything
	if ( info->m_nChangeFlags & ( k_EPersonaChangeRelationshipChanged | k_EPersonaChangeComeOnline | k_EPersonaChangeGoneOffline ) )
	{
		// We just got/lost a friend
		DirtyPotentialFriendsList();
		return;
	}

	FriendStateChange( info->m_ulSteamID );
}

void CSteamFriendsListPanel::OnRichPresenceChanged( FriendRichPresenceUpdate_t *info )
{
	FriendStateChange( info->m_steamIDFriend );
}