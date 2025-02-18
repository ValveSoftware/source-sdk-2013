//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//


#include "cbase.h"
#include "tf_matchmaking_dashboard_parent_manager.h"
#include "tf_matchmaking_dashboard_popup.h"
#include "tf_matchmaking_dashboard.h"
#include "tf_hud_disconnect_prompt.h"
#include "tf_gc_client.h"
#include "tf_gamerules.h"
#include "tf_partyclient.h"
#include "tf_lobby_server.h"
#include "clientmode_tf.h"
#include "econ_notifications.h"

using namespace vgui;
using namespace GCSDK;


class CJoinMatchNotification : public CEconNotification
{
public:
	CJoinMatchNotification( Panel* pDispatcher )
	{
		m_pDispatcher.Set( pDispatcher );
		m_bCreateMainMenuPanel = false;
	}

	virtual EType NotificationType() OVERRIDE { return CEconNotification::eType_MustTrigger; }

	virtual void Trigger() OVERRIDE
	{
		if ( m_pDispatcher )
		{
			m_pDispatcher->OnCommand( "join_match" );
		}
	}

	virtual const char *GetUnlocalizedHelpText() OVERRIDE
	{
		return "#Notification_Accept_Help";
	}

	PHandle m_pDispatcher;
};


class CMatchInviteNotification : public CTFDashboardNotification, public CGameEventListener
{
	DECLARE_CLASS_SIMPLE( CMatchInviteNotification, CTFDashboardNotification );
public:
	CMatchInviteNotification( PlayerGroupID_t groupID )
		: CTFDashboardNotification( CTFDashboardNotification::TYPE_LOBBY_INVITE,
									CTFDashboardNotification::CENTER,
									0.f,
									"NewMatchFound" )
		, m_GroupID( groupID )
		, m_flAutoJoinTime( 0.f )
	{
		m_pBGPanel = new EditablePanel( this, "BGPanel" );
		
		// If the lobby is here already, then we prompt differently
		m_eState = GTFGCClientSystem()->GetLiveMatchLobbyID() == m_GroupID	? STATE_LOBBY_PRESENT_PROMPT
																			: STATE_LOBBY_INVITE_PROMPT;
		ListenForGameEvent( "match_invites_updated" );
		ListenForGameEvent( "lobby_updated" );
		PlaySoundEntry( "MatchMaking.Join" );

		vgui::ivgui()->AddTickSignal( GetVPanel(), 100 );
	}

	~CMatchInviteNotification()
	{
		m_flAutoJoinTime = 0.f;
		NotificationQueue_Remove( m_nNotificationID ); 
	}

	bool BAutojoinTimerRunning()
	{
		return m_flAutoJoinTime != 0.f;
	}

	void ResetTimer()
	{
		m_flAutoJoinTime = Plat_FloatTime() + 10.f;
		InvalidateLayout();
	}

	virtual int GetYMargin() const OVERRIDE { return YRES(-5); }

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme ) OVERRIDE
	{
		KeyValuesAD pConditions( "conditions" );
		if ( m_eState == STATE_LOBBY_INVITE_PROMPT )
		{
			pConditions->AddSubKey( new KeyValues( "if_expected" ) );
		}
		LoadControlSettings( "resource/UI/MatchMakingDashboardPopup_NewMatch.res" );

		BaseClass::ApplySchemeSettings( pScheme );
	}

	ETFMatchGroup GetMatchGroup() const
	{
		if ( m_GroupID == GTFGCClientSystem()->GetLiveMatchLobbyID() )
			return GTFGCClientSystem()->GetLiveMatchGroup();
		else
		{
			return GetInviteForID().eMatchGroup;
		}
	}

	virtual void PerformLayout() OVERRIDE
	{
		BaseClass::PerformLayout();

		// Different things are visible at different times
		m_pBGPanel->SetControlVisible( "AbandonButton", m_eState == STATE_LOBBY_PRESENT_PROMPT );
		m_pBGPanel->SetControlVisible( "SmallJoinButton", m_eState == STATE_LOBBY_PRESENT_PROMPT );
		m_pBGPanel->SetControlVisible( "WideJoinButton", m_eState == STATE_LOBBY_INVITE_PROMPT );
		m_pBGPanel->SetControlVisible( "Spinner", m_eState == STATE_LOBBY_INVITE_ACCEPTING 
								   || m_eState == STATE_LOBBY_PRESENT_JOINING );
		m_pBGPanel->SetControlVisible( "JoiningLabel", m_eState == STATE_LOBBY_INVITE_ACCEPTING 
									   || m_eState == STATE_LOBBY_PRESENT_JOINING );
		m_pBGPanel->SetControlVisible( "DescLabel", m_eState == STATE_LOBBY_INVITE_PROMPT 
									 || m_eState == STATE_LOBBY_PRESENT_PROMPT );
		m_pBGPanel->SetControlVisible( "AutoJoinLabel", ( m_eState == STATE_LOBBY_INVITE_PROMPT 
										 || m_eState == STATE_LOBBY_PRESENT_PROMPT ) && m_flAutoJoinTime != 0.f );

		ETFMatchGroup eMatchGroup = GetMatchGroup();
		auto pMatchDesc = GetMatchGroupDescription( eMatchGroup );
		if ( pMatchDesc )
		{
			wchar_t wszBuff[ 512 ];
			KeyValues* pKV = new KeyValues( nullptr );
			pKV->SetWString( "matchtype", g_pVGuiLocalize->Find( pMatchDesc->GetNameLocToken() ) );
			// Craft the "Your Casual match is ready" string
			g_pVGuiLocalize->ConstructString_safe( wszBuff,
												   g_pVGuiLocalize->Find( "#TF_Matchmaking_RollingQueue_NewTypedMatchReady" ),
												   pKV );
			m_pBGPanel->SetDialogVariable( "match_type",
										   wszBuff );

			// Make sure we have our in-game notification
			if ( NotificationQueue_Get( m_nNotificationID ) == NULL )
			{
				CJoinMatchNotification* pNoti = new CJoinMatchNotification( this );
				pNoti->SetText( "#TF_Matchmaking_RollingQueue_MatchReadyInGame" ); 
				pNoti->SetKeyValues( pKV );
				pNoti->SetLifetime( 10.f );
				m_nNotificationID = NotificationQueue_Add( pNoti );
			}
		}
		else
		{
			Assert( false ); // This should not happen.  Use generic "Your match is ready"
			m_pBGPanel->SetDialogVariable( "match_type",
										   g_pVGuiLocalize->Find( "#TF_Matchmaking_RollingQueue_NewMatchReady" ) );
		}
	}

	virtual void FireGameEvent( IGameEvent *event ) OVERRIDE
	{
		if ( FStrEq( event->GetName(), "match_invites_updated" ) )
		{
			CheckForExpectedLobby();
		}
		else if ( FStrEq( event->GetName(), "lobby_updated" ) )
		{
			CheckForExpectedLobby();
		}
	}

	//
	// We use OnTick instead of OnThink to update the countdown because we might not be visible (ie. in-game)
	//
	virtual void OnTick() OVERRIDE
	{
		BaseClass::OnTick();

		if ( m_eState != STATE_LOBBY_INVITE_PROMPT && m_eState != STATE_LOBBY_PRESENT_PROMPT )
			return;

		if ( m_flAutoJoinTime != 0.f && Plat_FloatTime() > m_flAutoJoinTime )
		{
			AcceptMatch();
		}

		if ( m_flAutoJoinTime < Plat_FloatTime() )
			return;

		// Autojoin time
		wchar_t *pwszAutoJoinTime = NULL;

		// Update the countdown label
		double flTimeUntilAutoJoin = Max( 0., m_flAutoJoinTime - Plat_FloatTime() );
		pwszAutoJoinTime = LocalizeNumberWithToken( "TF_Matchmaking_RollingQueue_AutojoinWarning", ceil( flTimeUntilAutoJoin ) );

		m_pBGPanel->SetDialogVariable( "auto_join", pwszAutoJoinTime );

		ETFMatchGroup eMatchGroup = GetMatchGroup();
		auto pMatchDesc = GetMatchGroupDescription( eMatchGroup );
		CEconNotification* pNoti = NotificationQueue_Get( m_nNotificationID );

		// Update the KVs so the timer ticks down on the notification panel
		if ( pNoti && pMatchDesc )
		{
			wchar_t wszTimeBuff[ 3 ];
			_snwprintf( wszTimeBuff, 3, L"%.0f", flTimeUntilAutoJoin );
			KeyValuesAD pKV( "" );
			pKV->SetWString( "matchtype", g_pVGuiLocalize->Find( pMatchDesc->GetNameLocToken() ) );
			pKV->SetWString( "time", wszTimeBuff );
			pNoti->SetKeyValues( pKV );
		}
	}

	virtual void OnCommand( const char* pszCommand ) OVERRIDE
	{
		if ( FStrEq( pszCommand, "join_match" ) )
		{
			// Join it!
			AcceptMatch();
		}
		else if ( FStrEq( pszCommand, "abandon_match" ) )
		{
			CTFRejoinConfirmDialog* pAbandonDialog = BuildRejoinConfirmDialog();

			if ( pAbandonDialog )
			{
				pAbandonDialog->Show();
			}
		}
	}

	void AcceptInvite()
	{
		// Tell the GC we want this lobby
		if ( !GetInviteForID().bSentAcceptMsg )
		{
			GTFGCClientSystem()->RequestAcceptMatchInvite( m_GroupID );
			m_eState = STATE_LOBBY_INVITE_ACCEPTING;
			InvalidateLayout();
		}
	}

	MESSAGE_FUNC( OnConnect, "Connect" )
	{
		// Connect to the lobby
		GTFGCClientSystem()->JoinMMMatch();
		m_eState = STATE_LOBBY_PRESENT_JOINING;
		InvalidateLayout();
	}

	void Destroy()
	{
		MarkForDeletion();
	}

private:
	void AcceptMatch()
	{
		switch( m_eState )
		{
			case STATE_LOBBY_INVITE_PROMPT:
			{
				AcceptInvite();
				break;
			}
			case STATE_LOBBY_PRESENT_PROMPT:
			{
				BeginConnect();
				break;
			}

			default:
				// Shouldn't be hit
				Assert( false );
		}
	}

	void CheckForExpectedLobby()
	{
		if ( m_eState == STATE_LOBBY_INVITE_ACCEPTING )
		{
			// Did it become a lobby?
			auto unLiveLobbyID = GTFGCClientSystem()->GetLiveMatchLobbyID();
			// Hey that's us!
			if ( unLiveLobbyID == m_GroupID )
			{
				BeginConnect();
				return;
			}

			// Did it vanish or fail to accept without becoming a lobby?
			int idxInvite = GTFGCClientSystem()->GetMatchInviteIdxByLobbyID( m_GroupID );
			// Vanished without becoming a lobby, dashboard will destroy us
			if ( idxInvite == -1 )
				{ return; }

			// Stopped accepting
			if ( !GTFGCClientSystem()->GetMatchInvite( idxInvite ).bSentAcceptMsg )
			{
				Msg( "CLobbyInviteManager: Invite %llu failed to accept, returning to prompt\n", m_GroupID );
				m_eState = STATE_LOBBY_INVITE_PROMPT;
				ResetTimer();
				InvalidateLayout();
			}
		}
	}

	//
	// Ask GCClientSystem for the invite that corresponds to our groupID.  Only valid if we're representing an invite
	// and not a lobby.
	//
	CTFGCClientSystem::MatchInvite_t GetInviteForID() const
	{
		int nNumInvites = GTFGCClientSystem()->GetNumMatchInvites();
		for( int idxInvite=0; idxInvite < nNumInvites; ++idxInvite )
		{
			auto invite = GTFGCClientSystem()->GetMatchInvite( idxInvite );
			if ( invite.nLobbyID == m_GroupID )
			{
				return invite;
			}
		}

		Assert( false );
		return CTFGCClientSystem::MatchInvite_t{ 0, k_eTFMatchGroup_Invalid, false };
	}

	// Animate thing out and get ready to connect
	void BeginConnect()
	{
		// Make the main dashboard sides go away
		PostActionSignal( new KeyValues( "CloseSideStack" ) ); // Not passing in "side", meaning all sides
		// Delay the actual connect a smidge so everything can animate.
		PostMessage( this, new KeyValues( "Connect" ), 0.5f );
		InvalidateLayout();
	}

	enum EInviteState_t
	{
		STATE_LOBBY_INVITE_PROMPT,
		STATE_LOBBY_INVITE_ACCEPTING,
		STATE_LOBBY_PRESENT_PROMPT,
		STATE_LOBBY_PRESENT_JOINING,
	};

	double m_flAutoJoinTime;
	EditablePanel* m_pBGPanel = NULL;
	int m_nNotificationID = 0;
	PlayerGroupID_t m_GroupID = 0u;
	EInviteState_t m_eState;
};

//
// This class makes sure that we have a panel to represent each lobby invite or present lobby.  Also handles cleaning
// up the panels once the associated lobby/invite is dealt with (connected or abandonded).
//
class CLobbyInviteManager : public CAutoGameSystemPerFrame
{
public:
	CLobbyInviteManager() 
		: m_mapInvitePanels( DefLessFunc( PlayerGroupID_t ) )
	{}

	virtual void Update( float frametime ) OVERRIDE
	{
		bool bResetTimers = false;

		bResetTimers |= EnsurePanelsForInvites();
		bResetTimers |= EnsurePanelForExistingLobby();
		bResetTimers |= CleanupOrphanPanels();

		// Give the user time to think about their options
		if ( bResetTimers )
		{
			ResetTimers();
		}
	}

private:

	//
	// Need a panel for any lobby that's present in our SOCache
	//
	bool EnsurePanelForExistingLobby()
	{
		auto unLobbyID = GTFGCClientSystem()->GetLiveMatchLobbyID();
		bool bNeedsPanel = !GTFGCClientSystem()->BConnectedToMatchServer( true ) 
			&& GTFGCClientSystem()->BHaveLiveMatch()
			&& unLobbyID != 0u;

		bool bHasPanel = BHasPanelForLobby( unLobbyID );
		if ( bNeedsPanel && !bHasPanel )
		{
			CreateNewPanel( unLobbyID );
			return true;
		}

		return false;
	}

	//
	// Walks all the invites from the GC and makes sure we have a panel for those that we need to respond to
	//
	bool EnsurePanelsForInvites()
	{
		bool bPanelCreated = false;
		int nNumInvites = GTFGCClientSystem()->GetNumMatchInvites();
		for( int idxInvite=0; idxInvite < nNumInvites; ++idxInvite )
		{
			auto invite = GTFGCClientSystem()->GetMatchInvite( idxInvite );

			bool bGroupHasInvite = BHasPanelForLobby( invite.nLobbyID );

			if ( !bGroupHasInvite )
			{
				// Create a new panel for this invite
				CreateNewPanel( invite.nLobbyID );
				bPanelCreated = true;
			}
		}

		return bPanelCreated;
	}

	//
	// Resets the timers on all of the panels
	//
	void ResetTimers()
	{
		bool bResetSomeone = false;
		FOR_EACH_MAP_FAST( m_mapInvitePanels, idx )
		{
			if ( m_mapInvitePanels[ idx ]->BAutojoinTimerRunning() )
			{
				m_mapInvitePanels[ idx ]->ResetTimer();
				bResetSomeone = true;
			}
		}

		if ( !bResetSomeone && m_mapInvitePanels.Count() == 1 )
		{
			m_mapInvitePanels[ m_mapInvitePanels.FirstInorder() ]->ResetTimer();
		}
	}

	//
	// Remove any panels not associated with an invite or the present lobby
	//
	bool CleanupOrphanPanels()
	{
		auto unLiveLobbyID = GTFGCClientSystem()->GetLiveMatchLobbyID();
		bool bRemovedAny = false;

		FOR_EACH_MAP_FAST( m_mapInvitePanels, idx )
		{
			auto unPanelLobbyID = m_mapInvitePanels.Key( idx );

			if ( unLiveLobbyID == unPanelLobbyID && !GTFGCClientSystem()->BConnectedToMatchServer( true )  )
			{
				continue;
			}

			int nNumInvites = GTFGCClientSystem()->GetNumMatchInvites();
			bool bInviteMatch = false;
			for( int idxInvite=0; idxInvite < nNumInvites; ++idxInvite )
			{
				auto invite = GTFGCClientSystem()->GetMatchInvite( idxInvite );
				if ( invite.nLobbyID == unPanelLobbyID )
				{
					// WTB double-continue
					bInviteMatch = true;
					break;
				}
			}

			if ( bInviteMatch )
				continue;

			m_mapInvitePanels[ idx ]->Destroy();
			m_mapInvitePanels.RemoveAt( idx );
			idx = 0;
			bRemovedAny = true;
		}

		return bRemovedAny;
	}

	bool BHasPanelForLobby( PlayerGroupID_t id ) const
	{
		return m_mapInvitePanels.Find( id ) != m_mapInvitePanels.InvalidIndex();
	}

	//
	// Creates a new invite panel for the passedin invite
	//
	CMatchInviteNotification* CreateNewPanel( PlayerGroupID_t unGroupID )
	{
		// Better not be double-inserting!
		Assert( m_mapInvitePanels.Find( unGroupID ) == m_mapInvitePanels.InvalidIndex() );

		CMatchInviteNotification* pPanel = new CMatchInviteNotification( unGroupID );
		if ( m_mapInvitePanels.Count() == 0 )
		{
			pPanel->ResetTimer();
		}
		pPanel->MakeReadyForUse(); // Because the notifications get positioned RIGHT AWAY using their height
		m_mapInvitePanels.Insert( unGroupID, pPanel );
		return pPanel;
	}

	//
	// Destroys the panel specified by invite
	//
	void RemoveExistingPanel( PlayerGroupID_t unGroupID )
	{
		// The panel should be there!
		auto idx = m_mapInvitePanels.Find( unGroupID );
		Assert( idx != m_mapInvitePanels.InvalidIndex() );

		if ( idx != m_mapInvitePanels.InvalidIndex() )
		{
			m_mapInvitePanels[ idx ]->Destroy();
			m_mapInvitePanels.RemoveAt( idx );
		}
	}

	// Map of group IDs to invite panels.
	CUtlMap< PlayerGroupID_t, CMatchInviteNotification* > m_mapInvitePanels;
};

static CLobbyInviteManager s_LobbyInviteManager;
