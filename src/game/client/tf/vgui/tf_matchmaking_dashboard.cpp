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
#include "vgui/IInput.h"
#include "clientmode_tf.h"
#include "tf_hud_disconnect_prompt.h"
#include "tf_gc_client.h"
#include "tf_party.h"
#include "tf_partyclient.h"
#include <vgui/ISurface.h>
#include "vgui_avatarimage.h"
#include "tf_hud_mainmenuoverride.h"
#include "tf_pvp_rank_panel.h"
#include "vgui_controls/Menu.h"
#include "tf_lobby_server.h"
#include <vgui_controls/AnimationController.h>
#include "tf_matchmaking_dashboard_popup.h"
#include "tf_matchmaking_dashboard_parent_manager.h"
#include "tf_matchmaking_dashboard_side_panel.h"
#include "tf_matchmaking_dashboard_explanations.h"
#include "tf_matchmaking_dashboard_mvm_criteria.h"

using namespace vgui;
using namespace GCSDK;

extern ConVar tf_mm_next_map_vote_time;
ConVar tf_mm_dashboard_slide_panel_step( "tf_mm_dashboard_slide_panel_step", "20", FCVAR_ARCHIVE );
ConVar tf_casual_welcome_hide( "tf_casual_welcome_hide", "0", FCVAR_ARCHIVE | FCVAR_HIDDEN );
ConVar tf_comp_welcome_hide( "tf_comp_welcome_hide", "0", FCVAR_ARCHIVE | FCVAR_HIDDEN );
ConVar tf_notifications_push_bottom( "tf_notifications_push_bottom", "0", FCVAR_ARCHIVE );
ConVar tf_queue_spinner_color( "tf_queue_spinner_color", "0", FCVAR_ARCHIVE );
ConVar tf_dashboard_slide_time( "tf_dashboard_slide_time", "0.25", FCVAR_ARCHIVE, "How long it takes for the dashboard side panels to slide in and out", true, 0.0, true, 1.0 );


//-----------------------------------------------------------------------------
// Purpose: Prompt the user and ask if they really want to start training (if they are in a game)
//-----------------------------------------------------------------------------
class CTFConfirmTrainingDialog : public CConfirmDialog
{
	DECLARE_CLASS_SIMPLE( CTFConfirmTrainingDialog, CConfirmDialog );
public:
	CTFConfirmTrainingDialog( const char *pText, const char *pTitle, vgui::Panel *parent ) : BaseClass(parent), m_pText( pText ), m_pTitle( pTitle ) {}

	virtual const wchar_t *GetText()
	{
		return g_pVGuiLocalize->Find( m_pText );
	}

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme ) OVERRIDE
	{
		BaseClass::ApplySchemeSettings( pScheme );

		// Set the X to be bright, and the rest dull
		if ( m_pConfirmButton )
		{
			m_pConfirmButton->SetText( "#TF_Training_Prompt_ConfirmButton" );
		}
		if ( m_pCancelButton )
		{
			m_pCancelButton->SetText( "#TF_Training_Prompt_CancelButton" );
		}

		CExLabel *pTitle = dynamic_cast< CExLabel* >( FindChildByName( "TitleLabel" ) );
		if ( pTitle )
		{
			pTitle->SetText( m_pTitle );
		}
	}
protected:
	const char *m_pText;
	const char *m_pTitle;
};

Panel* GetBackgroundDimmer()
{
	class CDimmerButton : public Button
	{
	public:
		CDimmerButton()
			:  Button( NULL, "DashboardDimmer", (const char*)NULL )
		{}

		virtual void ApplySchemeSettings( vgui::IScheme *pScheme ) OVERRIDE
		{
			Button::ApplySchemeSettings( pScheme );

			SetCommand( "dimmer_clicked" );
			SetMouseInputEnabled( true );
			SetKeyBoardInputEnabled( false ); // This can never be true
			SetVisible( true );
			SetProportional( true );
			SetBounds( 0, 0, g_pClientMode->GetViewport()->GetWide(), g_pClientMode->GetViewport()->GetTall() - YRES( 59 ) ); // Magically touch the top of the footer
			SetZPos( 1000 );
			Color black( 0, 0, 0, 255 );
			SetDefaultColor( black, black );
			SetArmedColor( black, black );
			SetSelectedColor( black, black );
			SetDepressedColor( black, black );
			SetBgColor( black );
			SetAlpha( 0 );
		}
	};

	Panel* pDimmer = new CDimmerButton();
	pDimmer->AddActionSignalTarget( GetMMDashboard() );

	return pDimmer;
}
REGISTER_FUNC_FOR_DASHBOARD_PANEL_TYPE( GetBackgroundDimmer, k_eBGDimmer );

class CTooltipEditablePanel : public EditablePanel
{
public:
	DECLARE_CLASS_SIMPLE( CTooltipEditablePanel, EditablePanel );
	CTooltipEditablePanel( EMMTooltips eTipType ) 
		: EditablePanel( NULL, "TooltipPanel" ) 
		, m_eTipType( eTipType )
	{
		vgui::HScheme scheme = vgui::scheme()->LoadSchemeFromFileEx( enginevgui->GetPanel( PANEL_CLIENTDLL ), "resource/ClientScheme.res", "ClientScheme");
		SetProportional( true );
		SetScheme(scheme);
	}

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme ) OVERRIDE
	{
		BaseClass::ApplySchemeSettings( pScheme );

		KeyValuesAD pKVCond( "conditions" );
		switch ( m_eTipType )
		{
			case k_eSmallFont:	pKVCond->AddSubKey( new KeyValues( "if_small" ) ); break;
			case k_eMediumFont:	pKVCond->AddSubKey( new KeyValues( "if_medium" ) ); break;
			case k_eLargeFont:	pKVCond->AddSubKey( new KeyValues( "if_large" ) ); break;
		}
		
		LoadControlSettings( "resource/UI/MatchMakingTooltip.res", NULL, NULL, pKVCond );
	}

	virtual void OnThink() OVERRIDE
	{
		BaseClass::OnThink();

		SetMouseInputEnabled( false );
	}

private:
	EMMTooltips m_eTipType;
};

Panel* GetDashboardToolTipPanel( EMMTooltips eTipType )
{	
	return new CTooltipEditablePanel( eTipType );
}

CTFTextToolTip* GetDashboardTooltip( EMMTooltips eTipType )
{
	// 
	// Special class so the tooltips will always update their position.
	//
	class CTFDashboardTooltip : public CTFTextToolTip
	{
	public:
		CTFDashboardTooltip() : CTFTextToolTip( NULL, NULL )
		{}

		virtual void PerformLayout() OVERRIDE
		{
			// Make these always update their position
			_isDirty = true;
			CTFTextToolTip::PerformLayout();

			if ( !_makeVisible )
				m_pEmbeddedPanel->SetVisible( false );
		}
	};

	static CTFDashboardTooltip s_ToolTip[ k_eTooltipsCount ];;
	CTFTextToolTip* pToolTip = &s_ToolTip[ eTipType ];
	pToolTip->SetTooltipDelay( 0 );
	pToolTip->SetMaxWide( 300 );

	pToolTip->SetEmbeddedPanel( GetDashboardPanel().GetTypedPanel< EditablePanel >( EMMDashboadSidePanel( k_eToolTipSmallFont + eTipType ) ) );
	return pToolTip;
}
REGISTER_FUNC_FOR_DASHBOARD_PANEL_TYPE( []()->Panel* { return GetDashboardToolTipPanel( k_eSmallFont ); }, k_eToolTipSmallFont );
REGISTER_FUNC_FOR_DASHBOARD_PANEL_TYPE( []()->Panel* { return GetDashboardToolTipPanel( k_eMediumFont ); }, k_eToolTipMediumFont );
REGISTER_FUNC_FOR_DASHBOARD_PANEL_TYPE( []()->Panel* { return GetDashboardToolTipPanel( k_eLargeFont ); }, k_eToolTipLargeFont );

CDashboardSingletonManager& GetDashboardPanel()
{
	static CDashboardSingletonManager s_manager;
	return s_manager;
}

CDashboardSingletonManager::CDashboardSingletonManager()
{
	memset( m_pFnCreation, NULL, sizeof( m_pFnCreation ) );
	memset( m_pPanels, NULL, sizeof( m_pPanels ) );
}

Panel* CDashboardSingletonManager::GetPanel( EMMDashboadSidePanel ePanel )
{
	// Needs to be a function to make the panel
	Assert( m_pFnCreation[ ePanel ] );

	if ( !m_pPanels[ ePanel ] && m_pFnCreation[ ePanel ] )
	{
		m_pPanels[ ePanel ] = (*m_pFnCreation[ ePanel ])();
		GetMMDashboardParentManager()->AddPanel( m_pPanels[ ePanel ] );
	}

	// Need to have made the panel
	Assert( m_pPanels[ ePanel ] );
	return m_pPanels[ ePanel ];
}

void CDashboardSingletonManager::RegisterFnForSingleton( EMMDashboadSidePanel ePanel, pFnPanelCreationFunc pFn )
{
	// We shouldn't be overwriting a function ptr!
	Assert( m_pFnCreation[ ePanel ] == NULL );
	m_pFnCreation[ ePanel ] = pFn;
}

void CDashboardSingletonManager::RecreateAll()
{
	for( int i=0; i < k_ePanelCount; ++i )
	{
		if ( m_pPanels[ i ] )
		{
			GetMMDashboardParentManager()->RemovePanel( m_pPanels[ i ] );
			m_pPanels[ i ]->MarkForDeletion();
			m_pPanels[ i ] = NULL;
			GetPanel( (EMMDashboadSidePanel)i );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Snag the singleton CTFMatchmakingDashboard
//-----------------------------------------------------------------------------
CTFMatchmakingDashboard* GetMMDashboard()
{
	static CTFMatchmakingDashboard* s_pDashboardPanel = NULL;
	if ( !s_pDashboardPanel )
	{
		s_pDashboardPanel = new CTFMatchmakingDashboard();
		GetMMDashboardParentManager()->AddPanel( s_pDashboardPanel );
	}

	return s_pDashboardPanel;
}


CTFMatchmakingDashboard::CTFMatchmakingDashboard()
	: CExpandablePanel( NULL, "MMDashboard" )
{
	SetKeyBoardInputEnabled( false );
	ivgui()->AddTickSignal( GetVPanel(), 0 );

	vgui::HScheme scheme = vgui::scheme()->LoadSchemeFromFileEx( enginevgui->GetPanel( PANEL_CLIENTDLL ), "resource/ClientScheme.res", "ClientScheme");
	SetScheme(scheme);
	SetProportional( true );

	m_pTopBar = new EditablePanel( this, "TopBar" );
	m_pQueuePanel = new EditablePanel( m_pTopBar, "QueueContainer" );
	m_pJoinPartyLobbyPanel = new EditablePanel( m_pTopBar, "JoinPartyLobbyContainer" );
	m_pPlayButton = new CExImageButton( m_pTopBar, "FindAGameButton", (const char*)NULL );
	m_pResumeButton = new CExImageButton( m_pTopBar, "ResumeButton", (const char*)NULL );
	m_pQuitButton = new CExImageButton( m_pTopBar, "QuitButton", (const char*)NULL );
	m_pDisconnectButton = new CExImageButton( m_pTopBar, "DisconnectButton", (const char*)NULL );

	ListenForGameEvent( "gameui_hidden" );
	ListenForGameEvent( "gameui_activated" );
	ListenForGameEvent( "client_disconnect" );
	ListenForGameEvent( "party_criteria_changed" );
	ListenForGameEvent( "party_queue_state_changed" );
	ListenForGameEvent( "party_invites_changed" );
	ListenForGameEvent( "party_updated" );
	ListenForGameEvent( "lobby_updated" );
	ListenForGameEvent( "match_invites_updated" );
}

CTFMatchmakingDashboard::~CTFMatchmakingDashboard()
{
}

void CTFMatchmakingDashboard::ApplySchemeSettings( vgui::IScheme *pScheme ) 
{
	BaseClass::ApplySchemeSettings( pScheme );

	SetMouseInputEnabled( true );
	LoadControlSettings( "resource/UI/MatchMakingDashboard.res" );

	// This cannot ever be true or else things get weird when in-game
	SetKeyBoardInputEnabled( false );

	GetMMDashboardParentManager()->UpdateParenting();

	for( int i=0; i < ARRAYSIZE( m_colorPartyMembers ); ++i )
	{
		m_colorPartyMembers[ i ] = pScheme->GetColor( CFmtStr( "PartyMember%d", i +1 ), Color( 255, 255, 255, 255 ) );
	}

	UpdateDisconnectAndResume();
}

const Color& CTFMatchmakingDashboard::GetPartyMemberColor( int nSlot ) const
{
	Assert( nSlot >= 0 );
	Assert( nSlot < ARRAYSIZE( m_colorPartyMembers ) );
	return m_colorPartyMembers[ nSlot ];
}

Menu* CTFMatchmakingDashboard::ClearAndGetDashboardContextMenu()
{
	if ( m_pContextMenu )
	{
		m_pContextMenu->MarkForDeletion();
		m_pContextMenu = NULL;
	}

	m_pContextMenu = new Menu( this, "DashboardContextMenu" );
	const char *pszContextMenuBorder = "NotificationDefault";
	const char *pszContextMenuFont = "HudFontMediumSecondary";
	m_pContextMenu->SetBorder( scheme()->GetIScheme( GetScheme() )->GetBorder( pszContextMenuBorder ) );
	m_pContextMenu->SetFont( scheme()->GetIScheme( GetScheme() )->GetFont( pszContextMenuFont, IsProportional() ) );

	return m_pContextMenu;
}

void CTFMatchmakingDashboard::Reload()
{
	PopStack( 100, k_eSideLeft );
	PopStack( 100, k_eSideRight );
	InvalidateLayout( true, true );
	// Twice.  This fixes the party avatars not sizing correctly, and I don't 
	// want to figure out why they're not sizing correctly.  This really only
	// get called on resolution change, which is so infrequent that we can eat
	// the cost.
	InvalidateLayout( true, true );
}

void ConfirmQuit( bool bConfirmed, void *pContext )
{
	if ( bConfirmed )
	{
		engine->ClientCmd_Unrestricted( "quit" );
	}
}

void CTFMatchmakingDashboard::OnCommand( const char *command )
{
	if ( FStrEq( command, "disconnect" ) )
	{
		CTFDisconnectConfirmDialog *pDialog = BuildDisconnectConfirmDialog();
		if ( pDialog )
		{
			pDialog->Show();
		}
	}
	else if ( FStrEq( command, "toggle_chat" ) )
	{
		auto pChat = GetDashboardPanel().GetTypedPanel< CExpandablePanel >( k_eChat );
		pChat->ToggleCollapse();
	}
	else if (FStrEq("create_server", command))
	{
		OnCreateServer();
		return;
	}
	else if ( FStrEq( command, "find_game" ) )
	{
		OnPlayCommunity();
		return;
		PopStack( 100, k_eSideRight ); // All y'all
		PushSlidePanel( GetDashboardPanel().GetTypedPanel< CMatchMakingDashboardSidePanel >( k_ePlayList ) );
		CHudMainMenuOverride *pMMOverride = (CHudMainMenuOverride*)( gViewPortInterface->FindPanelByName( PANEL_MAINMENUOVERRIDE ) );
		pMMOverride->CheckTrainingStatus();
	}
	else if ( FStrEq( command, "quit" ) )
	{
		if ( engine->IsInGame() )
		{
			PromptOrFireCommand( "disconnect" );
		}
		else
		{
			CHudMainMenuOverride *pMMOverride = (CHudMainMenuOverride*)( gViewPortInterface->FindPanelByName( PANEL_MAINMENUOVERRIDE ) );
			ShowConfirmDialog( "#MMenu_PromptQuit_Title", "#MMenu_PromptQuit_Body", "#TF_Coach_Yes", "#TF_Coach_No", ConfirmQuit, pMMOverride );
		}
	}
	else if ( FStrEq( command, "dimmer_clicked" ) )
	{
		ClearAllStacks();
		UpdateDimmer();
	}
	else if ( FStrEq( command, "leave_queue" ) ) 
	{
		if ( GTFPartyClient()->BControllingPartyActions() && GTFPartyClient()->GetNumQueuedMatchGroups() == 1 )
		{
			ETFMatchGroup eQueuedMatchGroup = GTFPartyClient()->GetQueuedMatchGroupByIdx( 0 );
			GTFPartyClient()->CancelMatchQueueRequest( eQueuedMatchGroup );
		}
		else if ( GTFPartyClient()->BInStandbyQueue() )
		{
			GTFPartyClient()->CancelStandbyQueueRequest();
		}
	}
	else if ( FStrEq( command, "toggle_party_settings" ) ) 
	{
		OpenPartyOptionsMenu();
	}
	else if ( FStrEq( command, "resume_game" ) )
	{
		// The main menu knows what to do
		GetClientModeTFNormal()->GameUI()->SendMainMenuCommand(  "ResumeGame" );
	}
	else if ( FStrEq( command, "join_party_match" ) )
	{
		GTFPartyClient()->RequestQueueForStandby();
	}
	else if ( FStrEq( command, "queue_logo_clicked" ) )
	{
		// Change from Red <-> Blue
		tf_queue_spinner_color.SetValue( tf_queue_spinner_color.GetInt() == 0 ? 1 : 0 );
		UpdateQueuePanel();

		Panel* pSpinner = m_pQueuePanel->FindChildByName( "CTFLogoPanel" );
		if ( pSpinner )
		{
			// Do a radius pop
			g_pClientMode->GetViewportAnimationController()->RunAnimationCommand( pSpinner, "radius", 15.f , 0.0f, 0.02f, vgui::AnimationController::INTERPOLATOR_LINEAR, 0.8f, true, false );
			g_pClientMode->GetViewportAnimationController()->RunAnimationCommand( pSpinner, "radius", 10.f, 0.02f, 0.1f, vgui::AnimationController::INTERPOLATOR_DEACCEL, 0.8f, false, false );

			// Speed up, then decay back
			KeyValuesAD pkvInfo( "velocity" );
			pSpinner->RequestInfo( pkvInfo );
			float flVel = pkvInfo->GetFloat( "velocity" ) + 100.f;
			g_pClientMode->GetViewportAnimationController()->RunAnimationCommand( pSpinner, "velocity", flVel, 0.0f, 0.02f, vgui::AnimationController::INTERPOLATOR_LINEAR, 0.8f, true, false );
			float flDecayTime = logf( 1.f + ( flVel / 50.f ) );
			g_pClientMode->GetViewportAnimationController()->RunAnimationCommand( pSpinner, "velocity", 100.f, 0.05f, flDecayTime, vgui::AnimationController::INTERPOLATOR_DEACCEL, 0.8f, false, false );
		}
	}
	else if ( FStrEq( command, "manage_queues" ) )
	{
		m_pContextMenu = ClearAndGetDashboardContextMenu();
		MenuBuilder contextMenuBuilder( m_pContextMenu, this );

		// Add context menus for each match group we're queued in
		for( int i = ETFMatchGroup_MIN; i < ETFMatchGroup_ARRAYSIZE; ++i )
		{
			if ( !ETFMatchGroup_IsValid( i ) )
				continue;

			ETFMatchGroup eMatchGroup = (ETFMatchGroup)i;
			auto pMatchGroup = GetMatchGroupDescription( eMatchGroup );

			if ( !pMatchGroup )
				continue;

			if ( GTFPartyClient()->BInQueueForMatchGroup( eMatchGroup ) )
			{
				wchar_t wszBuff[ 512 ];
				g_pVGuiLocalize->ConstructString_safe( wszBuff, g_pVGuiLocalize->Find( "#TF_MM_QueueState_LeaveQueue" ), 1, g_pVGuiLocalize->Find( pMatchGroup->GetNameLocToken() ) );
				contextMenuBuilder.AddMenuItem( wszBuff, new KeyValues( "Command", "command", CFmtStr( "leavematchgroup%d", i ) ), "leave" );
			}
		}

		Panel* pOptionsButton = m_pQueuePanel->FindChildByName( "MultiQueuesManageButton", true );
		// Position the menu onto the gear button
		int nX, nY;
		g_pVGuiInput->GetCursorPosition( nX, nY );
		m_pContextMenu->SetPos( nX - 1, nY - 1 );

		if ( pOptionsButton )
		{
			nX = pOptionsButton->GetWide() * 0.5;
			nY = pOptionsButton->GetTall() * 0.5;
			pOptionsButton->LocalToScreen( nX, nY );

			m_pContextMenu->SetPos( nX, nY );	
		}

		m_pContextMenu->SetVisible(true);
		m_pContextMenu->AddActionSignalTarget(this);
		m_pContextMenu->MoveToFront();
	}
	else if ( Q_strnicmp( "leavematchgroup", command, sizeof( "leavematchgroup" ) ) )
	{
		// Leave a specific match group
		int nMatchgroup = atoi( command + sizeof( "leavematchgroup" ) - 1 );
		if ( !ETFMatchGroup_IsValid( nMatchgroup ) )
			return;

		ETFMatchGroup eMatchGroup = (ETFMatchGroup)nMatchgroup;
		GTFPartyClient()->CancelMatchQueueRequest( eMatchGroup );
	}
}

void CTFMatchmakingDashboard::OpenPartyOptionsMenu()
{
	m_pContextMenu = ClearAndGetDashboardContextMenu();
	MenuBuilder contextMenuBuilder( m_pContextMenu, this );

	if ( GTFPartyClient()->GetActiveParty() != NULL )
	{
		contextMenuBuilder.AddMenuItem( "#TF_Matchmaking_RollingQueue_LeaveParty", new KeyValues( "Context_LeaveParty" ), "party" );
	}

	contextMenuBuilder.AddMenuItem( "#TF_MM_OpenSettings", new KeyValues( "Context_OpenSettings" ), "settings" );

	Panel* pOptionsButton = FindChildByName( "PartySettingsButton", true );
	// Position to the cursor's position
	int nX, nY;
	g_pVGuiInput->GetCursorPosition( nX, nY );
	m_pContextMenu->SetPos( nX - 1, nY - 1 );

	if ( pOptionsButton )
	{
		nX = 0;
		nY = pOptionsButton->GetTall();
		LocalToScreen( nX, nY );

		m_pContextMenu->SetPos( nX, nY );	
	}

	m_pContextMenu->SetVisible(true);
	m_pContextMenu->AddActionSignalTarget(this);
	m_pContextMenu->MoveToFront();
}

void CTFMatchmakingDashboard::OnLeaveParty()
{
	GTFPartyClient()->LeaveActiveParty();
}

void CTFMatchmakingDashboard::OnOpenSettings()
{
	PushSlidePanel( GetDashboardPanel().GetTypedPanel< CMatchMakingDashboardSidePanel >( k_eMMSettings ) );
}

//-----------------------------------------------------------------------------
// Purpose: Figure out if we should be visible and require mouse input
//-----------------------------------------------------------------------------
void CTFMatchmakingDashboard::OnTick()
{
	// if the root panel has gone away just remove ourselves
	if ( !enginevgui->GetPanel( PANEL_CLIENTDLL ) )
	{
		MarkForDeletion();
		return;
	}

	if ( m_pContextMenu )
	{
		m_pContextMenu->MoveToFront();
	}

	// THIS IS A HACK and works wonderfully
	GetMMDashboardParentManager()->UpdateParenting();

	// Update the Y-Pos of all the "children" of the dashboard
	{
		// These are the panels that dangle off the topbar
		EMMDashboadSidePanel arPanelsDanglingOffTopBar[] =	{ k_eChat
															, k_eNextMapWinnerPopup
															, k_eNextMapVotePopup };

		int nNewYPos = Max( GetYPos() + GetTall() - YRES(10), YRES(-5) );
		for( int i=0; i < ARRAYSIZE( arPanelsDanglingOffTopBar ); ++i )
		{
			auto* pDangler = GetDashboardPanel().GetPanel( arPanelsDanglingOffTopBar[ i ] );
			pDangler->SetPos( pDangler->GetXPos(), nNewYPos );
		}
	}

	bool bShouldBeVisible = false;

	bool bInEndOfMatch = TFGameRules() && TFGameRules()->State_Get() == GR_STATE_GAME_OVER;
	const IMatchGroupDescription* pMatchDesc = GetMatchGroupDescription( TFGameRules() ? TFGameRules()->GetCurrentMatchGroup() : k_eTFMatchGroup_Invalid );

	if ( engine->IsInGame() )
	{
		bShouldBeVisible = enginevgui->IsGameUIVisible();
		bShouldBeVisible |= GTFGCClientSystem()->BConnectedToMatchServer( false )
			&& bInEndOfMatch
			&& pMatchDesc
			&& pMatchDesc->BUsesDashboard();
	}
	else
	{
		bShouldBeVisible = true;
	}


	if ( BIsExpanded() && !bShouldBeVisible )
	{
		SetCollapsed( true );
	}
	else if ( !BIsExpanded() && bShouldBeVisible )
	{
		SetCollapsed( false );
		UpdateDisconnectAndResume();
		auto pChat = GetDashboardPanel().GetTypedPanel< CExpandablePanel >( k_eChat );
		if ( pChat->BIsExpanded() )
		{
			// Immediately be collapsed
			pChat->SetCollapsed( true, true );
			// Casually expand like it's no thing
			pChat->SetCollapsed( false );
		}
	}

	SetKeyBoardInputEnabled( false );
	SetMouseInputEnabled( BIsExpanded() );
}

void CTFMatchmakingDashboard::FireGameEvent( IGameEvent *event )
{
	if ( FStrEq( event->GetName(), "party_criteria_changed" ) )
	{
		UpdateQueuePanel();
		UpdateJoinPartyLobbyPanel();
	}
	else if ( FStrEq( event->GetName(), "gameui_activated" ) ) 
	{
		UpdateFindAGameButton();
		UpdateDisconnectAndResume();
		UpdateDimmer();
	}
	else if ( FStrEq( event->GetName(), "gameui_hidden" ) )
	{
		ClearAllStacks();
	}
	else if ( FStrEq( event->GetName(), "client_disconnect" ) )
	{
		UpdateDisconnectAndResume();
		UpdateJoinPartyLobbyPanel();
	}
	else if ( FStrEq( event->GetName(), "party_queue_state_changed" ) )
	{
		if ( GTFPartyClient()->BInAnyMatchQueue() )
		{
			PlaySoundEntry( "MatchMaking.Queue" );
		}

		UpdateQueuePanel();
		UpdateJoinPartyLobbyPanel();
	}
	else if ( FStrEq( event->GetName(), "party_invites_changed" ) )
	{
		UpdatePartyInvites();
	}
	else if ( FStrEq( event->GetName(), "party_updated" ) )
	{
		UpdatePartyInvites();
		UpdateJoinPartyLobbyPanel();
	}
	else if ( FStrEq( event->GetName(), "lobby_updated" ) )
	{
		UpdateJoinPartyLobbyPanel();
	}
	else if ( FStrEq( event->GetName(), "match_invites_updated" ) )
	{
		UpdateJoinPartyLobbyPanel();
	}
}

void SpewInvitePanelThing( bool bCreated, const CInviteNotification* pInvite )
{
#if defined (STAGING_ONLY) || defined (DEBUG)
	auto key = pInvite->GetKey();
	CFmtStr str( "%s %s %s invite for %s", 
				 bCreated ? "Created" : "Deleted",
				 std::get<2>( key ) ? "INCOMING" : "OUTGOING",
				 std::get<1>( key ) == CTFParty::EPendingType::ePending_Invite ? "INVITE" : "JOIN_REQUEST",
				 SteamFriends()->GetFriendPersonaName( std::get<0>( key ) ) );
	ConColorMsg( bCreated ? Color( 100, 255, 190, 255 ) : Color( 255, 100, 190, 255 ), "%s\n", str.Get() );
#endif // defined (STAGING_ONLY) || defined (DEBUG)
}

void CTFMatchmakingDashboard::UpdatePartyInvites()
{
	auto lambdaHasNotification = [ & ]( const InviteKey_t& key )
	{
		FOR_EACH_VEC( m_vecInviteHandles, i )
		{
			if ( m_vecInviteHandles[ i ] && m_vecInviteHandles[ i ]->GetKey() == key )
				return true;
		}

		return false;
	};

	CSteamID steamIDLocal = SteamUser()->GetSteamID();

	// Loop through panels and ensure there's an invite for each panel, removing if needed
	FOR_EACH_VEC_BACK( m_vecInviteHandles, i )
	{
		bool bFound = false;

		if ( m_vecInviteHandles[ i ].Get() != NULL && GTFPartyClient()->BControllingPartyActions() )
		{
			//
			// Try to find the matching invite
			//
			auto key = m_vecInviteHandles[ i ]->GetKey();

			// Incoming invite
			for( int j=0; j < GTFPartyClient()->GetNumIncomingInvites(); ++j )
			{
				auto pInvite = GTFPartyClient()->GetIncomingInvite( j );
				if ( key == InviteKey_t( pInvite->GetInviter(), pInvite->GetType(), true ) )
				{
					bFound = true;
					break;
				}
			}

			// Outgoing invite
			for( int j=0; !bFound && j < GTFPartyClient()->GetNumOutgoingInvites(); ++j )
			{
				CSteamID invitee = GTFPartyClient()->GetOutgoingInvite( j );
				if ( key == InviteKey_t( invitee, IPlayerGroup::ePending_Invite, false ) )
				{
					bFound = true;
					break;
				}
			}

			// Outgoing Join
			for( int j=0; !bFound && j < GTFPartyClient()->GetNumOutgoingJoinRequests(); ++j )
			{
				auto pInvite = GTFPartyClient()->GetOutgoingJoinRequest( j );
				if ( key == InviteKey_t( pInvite->GetInviter(), pInvite->GetType(), false ) )
				{
					bFound = true;
					break;
				}
			}

			// Incoming Join
			for( int j=0; !bFound && j < GTFPartyClient()->GetNumIncomingJoinRequests(); ++j )
			{
				CSteamID joiner = GTFPartyClient()->GetIncomingJoinRequest( j );
				if ( key == InviteKey_t( joiner, IPlayerGroup::ePending_JoinRequest, true ) )
				{
					bFound = true;
					break;
				}
			}
		}

		// Invite is gone.  Make the panel go away
		if ( !bFound )
		{
			if ( m_vecInviteHandles[ i ].Get() != NULL )
			{
				SpewInvitePanelThing( false, m_vecInviteHandles[ i ] );
				m_vecInviteHandles[ i ]->SetToExpire();
			}
			m_vecInviteHandles.Remove( i );
		}
	}


	auto lambdaEnsurePanel = [ & ]( const CSteamID& steamID, CTFParty::EPendingType eType, bool bIncoming, bool bRequirePartyControl )
	{
		if ( bRequirePartyControl && !GTFPartyClient()->BControllingPartyActions() )
			return;

		// Doesn't exist!  Create a panel
		if ( !lambdaHasNotification( InviteKey_t{ steamID, eType, bIncoming } ) )
		{
			CInviteNotification* pNewNotification = new CInviteNotification( steamID, eType, bIncoming );
			m_vecInviteHandles[ m_vecInviteHandles.AddToTail() ].Set( pNewNotification );
			NotificationQueue_Add( pNewNotification );
			SpewInvitePanelThing( true, pNewNotification );
		}
	};

	//
	// Now that all the stale panels are gone, check if there's any invites that we
	// don't have a panel for.
	//
	// Loop through INCOMING INVITES and make sure there's a panel for each
	for( int i=0; i < GTFPartyClient()->GetNumIncomingInvites(); ++i )
	{
		auto pInvite = GTFPartyClient()->GetIncomingInvite( i );
		CSteamID steamIDInviter = pInvite->GetInviter();
		lambdaEnsurePanel( steamIDInviter, pInvite->GetType(), true, false );
	}

	// Loop through OUTGOING INVITES and make sure there's a panel for each
	for( int i=0; i < GTFPartyClient()->GetNumOutgoingInvites(); ++i )
	{
		CSteamID steamIDInvitee = GTFPartyClient()->GetOutgoingInvite( i );
		lambdaEnsurePanel( steamIDInvitee, IPlayerGroup::ePending_Invite, false, true );
	}

	// Loop through OUTGOING JOIN REQUESTS and make sure there's a panel for each
	for( int i=0; i < GTFPartyClient()->GetNumOutgoingJoinRequests(); ++i )
	{
		auto pInvite = GTFPartyClient()->GetOutgoingJoinRequest( i );
		CSteamID steamIDInviter = pInvite->GetInviter();
		lambdaEnsurePanel( steamIDInviter, pInvite->GetType(), false, true );
	}


	// Loop through INCOMING JOIN REQUESTS and make sure there's a panel for each
	for( int i=0; i < GTFPartyClient()->GetNumIncomingJoinRequests(); ++i )
	{
		CSteamID steamIDJoiner = GTFPartyClient()->GetIncomingJoinRequest( i );
		lambdaEnsurePanel( steamIDJoiner, IPlayerGroup::ePending_JoinRequest, true, true );
	}
}

void CTFMatchmakingDashboard::OnPlayCompetitive()
{
	if ( tf_comp_welcome_hide.GetBool() == false && !GTFPartyClient()->BInQueueForMatchGroup( k_eTFMatchGroup_Ladder_6v6 ) )
	{
		tf_comp_welcome_hide.SetValue( 1 );
		ShowDashboardExplanation( "CompIntro" );
	}

	// Show comp panel
	PushSlidePanel( GetDashboardPanel().GetTypedPanel< CMatchMakingDashboardSidePanel >( k_eCompetitive ) );
	GTFPartyClient()->SetLocalUIState( k_eTFSyncedMMMenuStep_Configuring_Mode, k_eTFMatchGroup_Ladder_6v6 );
}

void CTFMatchmakingDashboard::OnPlayCasual()
{
	if ( tf_casual_welcome_hide.GetBool() == false && !GTFPartyClient()->BInQueueForMatchGroup( k_eTFMatchGroup_Casual_12v12 ) )
	{
		tf_casual_welcome_hide.SetValue( 1 );
		ShowDashboardExplanation( "CasualIntro" );
	}

	PushSlidePanel( GetDashboardPanel().GetTypedPanel< CMatchMakingDashboardSidePanel >( k_eCasual ) );
	GTFPartyClient()->SetLocalUIState( k_eTFSyncedMMMenuStep_Configuring_Mode, k_eTFMatchGroup_Casual_12v12 );
}

void CTFMatchmakingDashboard::OnPlayMvM()
{
	// Open the choice panel
	PushSlidePanel( GetDashboardPanel().GetTypedPanel< CMatchMakingDashboardSidePanel >( k_eMvM_Mode_Select ) );
}

void CTFMatchmakingDashboard::OnPlayMvM_MannUp()
{
	// Make sure the MvM panel goes first.  We're not going to show it, just ensure it's created
	GetDashboardPanel().GetPanel( k_eMvM_Mode_Select );

	auto pConfigPanel = GetDashboardPanel().GetTypedPanel< CTFDashboardMvMPanel >( k_eMvM_Mode_Configure );
	PushSlidePanel( pConfigPanel );

#ifdef USE_MVM_TOUR
	// If no tour selected, then show the tour selection screen,
	// or else show the mission selection
	ETFSyncedMMMenuStep eMenuStep = GTFPartyClient()->GetLocalGroupCriteria().GetMannUpTourIndex() == k_iMvmTourIndex_Empty 
								  ? k_eTFSyncedMMMenuStep_MvM_Selecting_Tour
								  : k_eTFSyncedMMMenuStep_MvM_Selecting_Missions;

	GTFPartyClient()->SetLocalUIState( eMenuStep, k_eTFMatchGroup_MvM_MannUp );
#else // new mm
	GTFPartyClient()->SetLocalUIState( k_eTFSyncedMMMenuStep_MvM_Selecting_Missions, k_eTFMatchGroup_MvM_MannUp );
#endif // USE_MVM_TOUR
}

void CTFMatchmakingDashboard::OnPlayMvM_BootCamp()
{
	// Make sure the MvM panel goes first.  We're not going to show it, just ensure it's created
	GetDashboardPanel().GetPanel( k_eMvM_Mode_Select );

	auto pConfigPanel = GetDashboardPanel().GetTypedPanel< CTFDashboardMvMPanel >( k_eMvM_Mode_Configure );
	PushSlidePanel( pConfigPanel );

	GTFPartyClient()->SetLocalUIState( k_eTFSyncedMMMenuStep_MvM_Selecting_Missions, k_eTFMatchGroup_MvM_Practice );
}

void CTFMatchmakingDashboard::OnPlayTraining()
{
	ClearAllStacks();

	if ( engine->IsInGame() )
	{
		const char *pText = "#TF_Training_Prompt";
		const char *pTitle = "#TF_Training_Prompt_Title";
		if ( TFGameRules() && TFGameRules()->IsInTraining() )
		{
			pTitle = "#TF_Training_Restart_Title";
			pText = "#TF_Training_Restart_Text";
		}

		CTFConfirmTrainingDialog *pConfirm = vgui::SETUP_PANEL( new CTFConfirmTrainingDialog( pText, pTitle, this ) );
		if ( pConfirm )
		{
			pConfirm->Show();
		}
	}
	else
	{
		GetClientModeTFNormal()->GameUI()->SendMainMenuCommand( "engine training_showdlg" );
	}
}


void CTFMatchmakingDashboard::OnPlayCommunity()
{
	ClearAllStacks();
	// Just call the command directly
	engine->ClientCmd_Unrestricted( "gamemenucommand openserverbrowser" );
}

void CTFMatchmakingDashboard::OnCreateServer()
{
	ClearAllStacks();
	// Just call the command directly
	engine->ClientCmd_Unrestricted( "gamemenucommand OpenCreateMultiplayerGameDialog" );
}

void CTFMatchmakingDashboard::OnPlayEvent()
{
	// First make sure that there's an event going on, and they didn't
	// just spoof the message
	ETFMatchGroup eEventMatchGroup = GTFGCClientSystem()->WorldStatus().event_match_group();
	CRTime rtimeExpireTime( GTFGCClientSystem()->WorldStatus().event_expire_time() );

	if ( eEventMatchGroup == k_eTFMatchGroup_Invalid )
		return;

	if ( rtimeExpireTime < CRTime::RTime32TimeCur() )
		return;

	if ( eEventMatchGroup == k_eTFMatchGroup_Event_Placeholder
		 && tf_comp_welcome_hide.GetBool() == false 
		 && !GTFPartyClient()->BInQueueForMatchGroup( eEventMatchGroup ) )
	{
		tf_comp_welcome_hide.SetValue( 1 );
		ShowDashboardExplanation( "Comp12v12Intro" );
	}

	// Show event panel
	CMatchMakingDashboardSidePanel* pSidePanel = GetDashboardPanel().GetTypedPanel< CMatchMakingDashboardSidePanel >( k_eEventMatch );
	if ( !pSidePanel )
		return;

	PushSlidePanel( pSidePanel );

	GTFPartyClient()->SetLocalUIState( k_eTFSyncedMMMenuStep_Configuring_Mode, eEventMatchGroup );
}

void CTFMatchmakingDashboard::OnShowCompAccess()
{
	PushSlidePanel( GetDashboardPanel().GetTypedPanel< CMatchMakingDashboardSidePanel >( k_eCompAccess ) );
}

void CTFMatchmakingDashboard::OnViewMatchSettings()
{
	const TFSyncedMMUIState& uiState = GTFPartyClient()->GetLeaderUIState();
	auto eMatchGroup = uiState.match_group();

	switch ( eMatchGroup )
	{
		case k_eTFMatchGroup_Casual_12v12:
		{
			OnPlayCasual();
		}
		break;

		case k_eTFMatchGroup_Ladder_6v6:
		{
			OnPlayCompetitive();
		}
		break;

		case k_eTFMatchGroup_MvM_MannUp:
		{
			OnPlayMvM_MannUp();
		}
		break;

		case k_eTFMatchGroup_MvM_Practice:
		{
			OnPlayMvM_BootCamp();
		}
		break;
	}
}

void CTFMatchmakingDashboard::OnCloseSideStack( KeyValues* pParams )
{
	int nSide = pParams->GetInt( "side", -1 );
	switch( nSide )
	{
		case k_eSideLeft:
		case k_eSideRight:
		{
			EStackSide_t eSide( (EStackSide_t)nSide );
			PopStack( 100, eSide );
			break;
		}

		default:
			ClearAllStacks();
	}
}

void CTFMatchmakingDashboard::OnNavigateSideStack( Panel* panel )
{
	int nPopAmount = 0;

	CMatchMakingDashboardSidePanel* pSlidePanel = assert_cast< CMatchMakingDashboardSidePanel* >( panel );
	if ( pSlidePanel )
	{
		auto& vecStack = GetStackForSide( pSlidePanel->GetSide() );
		FOR_EACH_VEC_BACK( vecStack, i )
		{
			if ( vecStack[ i ].Get() == panel && nPopAmount )
			{
				PopStack( nPopAmount, pSlidePanel->GetSide() );
				return;
			}
			++nPopAmount;
		}
	}
	else
	{
		Assert( false );
	}
}

void CTFMatchmakingDashboard::OnNotificationCreated( Panel *pPanel )
{
	CTFDashboardNotification* pNotification = dynamic_cast< CTFDashboardNotification* >( pPanel );
	if ( !pNotification )
		return;

	int nY = m_pTopBar->GetYPos() + m_pTopBar->GetTall() - pNotification->GetTall() - YRES( 10 );

	if ( tf_notifications_push_bottom.GetBool() )
	{
		if ( m_vecNotifications.Count() )
		{
			nY = m_vecNotifications.Tail()->GetYPos();
		}

		nY += YRES( 30 );
	}

	// Be right where the chat is, but tucked up a bit so we nicely lerp down
	pNotification->SetPos( GetXPos(), nY );

	m_vecNotifications[ m_vecNotifications.AddToTail() ].Set( pNotification );
	PositionNotifications();
}

void CTFMatchmakingDashboard::OnNotificationCleared( Panel* pPanel )
{
	bool bCleared = false;
	FOR_EACH_VEC_BACK( m_vecNotifications, i )
	{
		if( m_vecNotifications[ i ] == pPanel || m_vecNotifications[ i ].Get() == NULL )
		{
			m_vecNotifications.Remove( i );
			bCleared = true;
		}
	}

	Assert( bCleared );
	PositionNotifications();
}

void CTFMatchmakingDashboard::PositionNotifications()
{
	if ( !g_pClientMode->GetViewport() )
		return;

	auto pAnim = g_pClientMode->GetViewportAnimationController();

	int nStartY = m_pTopBar->GetYPos() + m_pTopBar->GetTall() - YRES( 10 );
	int nY[ CTFDashboardNotification::EAlignment::NUM_ALIGNMENTS ];
	for( int i=0; i < ARRAYSIZE( nY ); ++i )
	{
		nY[ i ] = nStartY;
	}

	m_vecNotifications.SortPredicate( []( const CTFDashboardNotification* pLeft, const CTFDashboardNotification* pRight )
	{
		if ( pLeft->GetType() != pRight->GetType() )
		{
			return pLeft->GetType() < pRight->GetType();
		}

		return pLeft->GetCreationTime() < pRight->GetCreationTime();
	} );

	auto lambdaPosition = [&]( int i )
	{
		// Position the panels end-to-end vertically
		CTFDashboardNotification* pPopup = m_vecNotifications[ i ];
		int nYMargin = pPopup->GetYMargin();
		pAnim->RunAnimationCommand( pPopup, "ypos", nY[ pPopup->GetAlignment() ] + nYMargin, 0.f, 0.2f, AnimationController::INTERPOLATOR_SIMPLESPLINE, 0, true, true );
		nY[ pPopup->GetAlignment() ] += pPopup->GetTall() + nYMargin;

		// Align however the type specifies
		switch ( pPopup->GetAlignment() )
		{
			case CTFDashboardNotification::EAlignment::LEFT:
			{
				pAnim->RunAnimationCommand( pPopup, "xpos", 0, 0.f, 0.0f, AnimationController::INTERPOLATOR_SIMPLESPLINE, 0, true, true );
				break;
			}

			case CTFDashboardNotification::EAlignment::CENTER:
			{
				pAnim->RunAnimationCommand( pPopup, "xpos", GetWide() / 2 - pPopup->GetWide() / 2, 0.f, 0.0f, AnimationController::INTERPOLATOR_SIMPLESPLINE, 0, true, true );
				break;
			}

			case CTFDashboardNotification::EAlignment::RIGHT:
			{
				pAnim->RunAnimationCommand( pPopup, "xpos", GetWide() - pPopup->GetWide(), 0.f, 0.0f, AnimationController::INTERPOLATOR_SIMPLESPLINE, 0, true, true );
				break;
			}
		}
	};

	if ( tf_notifications_push_bottom.GetBool() )
	{
		FOR_EACH_VEC( m_vecNotifications, i )
		{
			lambdaPosition( i );
		}
	}
	else
	{
		FOR_EACH_VEC_BACK( m_vecNotifications, i )
		{
			lambdaPosition( i );
		}
	}
}

void CTFMatchmakingDashboard::PushSlidePanel( CMatchMakingDashboardSidePanel* pPanel )
{
	auto& vecStack = GetStackForSide( pPanel->GetSide() );

	FOR_EACH_VEC( vecStack, i )
	{
		if ( vecStack[ i ].Get() == pPanel )
			return;
	}

	vecStack[ vecStack.AddToTail() ].Set( pPanel );

	RepositionSidePanels( pPanel->GetSide() );

	PlaySoundEntry( "Panel.SlideDown" );
}

void CTFMatchmakingDashboard::PopStack( int nLevels, EStackSide_t eSide )
{
	auto& vecStack = GetStackForSide( eSide );

	if ( vecStack.IsEmpty() )
		return;

	int nSurfaceWide, nSurfaceTall;
	vgui::surface()->GetScreenSize( nSurfaceWide, nSurfaceTall );

	int nOriginalCount = vecStack.Count();
	nLevels = Clamp( nLevels, 0, nOriginalCount );

	while( nLevels-- )
	{
		float flTransitionTime = tf_dashboard_slide_time.GetFloat() + ( nOriginalCount - nLevels ) * 0.1f;
		CMatchMakingDashboardSidePanel* pPanel = vecStack.Tail();

		// Fire this off to hide ourselves after we've fully transitioned out.  Then we're not thinking.
		pPanel->PostMessage( pPanel, new KeyValues( "UpdateVisiblity" ), flTransitionTime );

		switch ( eSide )
		{
			case k_eSideLeft:
			{
				g_pClientMode->GetViewportAnimationController()->RunAnimationCommand( pPanel, "xpos", -pPanel->GetWide(), 0.f,flTransitionTime , vgui::AnimationController::INTERPOLATOR_SIMPLESPLINE, 0.8f, true, false );
				break;
			}

			case k_eSideRight:
			{
				g_pClientMode->GetViewportAnimationController()->RunAnimationCommand( pPanel, "xpos", nSurfaceWide, 0.f, flTransitionTime, vgui::AnimationController::INTERPOLATOR_SIMPLESPLINE, 0.8f, true, false );
				break;
			}
		}
		vecStack.RemoveMultipleFromTail( 1 );
	}

	RepositionSidePanels( eSide );

	PlaySoundEntry( "Panel.SlideUp" );
}

void CTFMatchmakingDashboard::ClearAllStacks()
{
	PopStack( 100, k_eSideLeft );
	PopStack( 100, k_eSideRight );
}

void CTFMatchmakingDashboard::RepositionSidePanels( EStackSide_t eSide )
{
	auto& vecStack = GetStackForSide( eSide );

	int nInnerEdge = 0;
	int nSurfaceWide, nSurfaceTall;
	vgui::surface()->GetScreenSize( nSurfaceWide, nSurfaceTall );

	FOR_EACH_VEC_BACK( vecStack, i )
	{
		CMatchMakingDashboardSidePanel* pPanel = vecStack[ i ];
		if ( !pPanel )
		{
			// This can happen.  We have handles and it's OK
			vecStack.Remove( i );
			continue;
		}

		bool bTop = i == vecStack.Count() - 1;
		pPanel->SetAsActive( bTop );
		pPanel->SetVisible( true );

		int nWide = pPanel->GetWide();
		int nX = 0;
		switch( eSide )
		{
			case k_eSideRight:
			{
				nX = Max( nInnerEdge, nWide );
				nInnerEdge = Max( nInnerEdge, int( nX + YRES( tf_mm_dashboard_slide_panel_step.GetInt() ) ) );
				g_pClientMode->GetViewportAnimationController()->RunAnimationCommand( pPanel, "xpos", nSurfaceWide - nX, 0.0f, tf_dashboard_slide_time.GetFloat(), vgui::AnimationController::INTERPOLATOR_SIMPLESPLINE, 0.8f, true, false );
				break;
			}

			case k_eSideLeft:
			{
				nX = Min( nInnerEdge, nWide );
				nInnerEdge = Min( nInnerEdge, int( nX - YRES( tf_mm_dashboard_slide_panel_step.GetInt() ) ) );
				g_pClientMode->GetViewportAnimationController()->RunAnimationCommand( pPanel, "xpos", -nX, 0.0f, tf_dashboard_slide_time.GetFloat(), vgui::AnimationController::INTERPOLATOR_SIMPLESPLINE, 0.8f, true, false );
				break;
			}
		}
	}

	// Show/Hide play button
	if ( eSide == k_eSideRight )
	{
		UpdateFindAGameButton();
	}

	// Show/Hide dimmer
	UpdateDimmer();
}

void CTFMatchmakingDashboard::UpdateFindAGameButton()
{
	int nPlayButtonYPos = GetStackForSide( k_eSideRight ).IsEmpty() ? YRES( 0 ) : YRES( -50 );
	g_pClientMode->GetViewportAnimationController()->RunAnimationCommand( m_pPlayButton, "ypos", nPlayButtonYPos, 0.0f, 0.4f, vgui::AnimationController::INTERPOLATOR_SIMPLESPLINE, 0.8f, true, false );
}

void CTFMatchmakingDashboard::UpdateDisconnectAndResume()
{
	bool bInGame = engine->IsInGame();

	m_pResumeButton->SetVisible( bInGame && !BInEndOfMatch() );

	m_pTopBar->SetControlVisible( "DisconnectButton", bInGame );
	m_pTopBar->SetControlVisible( "QuitButton", !bInGame );

	Panel* pOffsetPanel = bInGame ? m_pDisconnectButton : m_pQuitButton;

	m_pPlayButton->SetPos( pOffsetPanel->GetXPos() - m_pPlayButton->GetWide() - 1, m_pPlayButton->GetYPos() );
	m_pResumeButton->SetPos( m_pPlayButton->GetXPos() - m_pResumeButton->GetWide() - 1, m_pResumeButton->GetYPos() );
}

bool CTFMatchmakingDashboard::BAnySidePanelsShowing() const
{
	for( int i=0; i < ARRAYSIZE( m_vecSideSlideStack ); ++i )
	{
		if ( !m_vecSideSlideStack[ i ].IsEmpty() )
			return true;
	}

	return false;
}

bool CTFMatchmakingDashboard::BIsSidePanelShowing( const CMatchMakingDashboardSidePanel* pSidePanel ) const
{
	for( int i=0; i < ARRAYSIZE( m_vecSideSlideStack ); ++i )
	{
		FOR_EACH_VEC( m_vecSideSlideStack[ i ], j )
		{
			if ( m_vecSideSlideStack[ i ][ j ].Get() == pSidePanel )
				return true;
		}
	}

	return false;
}

void CTFMatchmakingDashboard::UpdateDimmer()
{
	bool bAnySlidePanels = BAnySidePanelsShowing();
	bool bShowDimmer = bAnySlidePanels;

	Panel* pDimmer = GetDashboardPanel().GetPanel( k_eBGDimmer );
	int nDimmerAlpha = bShowDimmer ? 230 : 0;
	g_pClientMode->GetViewportAnimationController()->RunAnimationCommand( pDimmer, "alpha", nDimmerAlpha, 0.0f, 0.4f, vgui::AnimationController::INTERPOLATOR_GAIN, 0.8f, true, false );
	pDimmer->SetMouseInputEnabled( bShowDimmer );
}

void GetQueuedString( wchar_t* pwszBuff, int nSize )
{
	if ( GTFPartyClient()->BInStandbyQueue() )
	{
		g_pVGuiLocalize->ConstructString( pwszBuff, 
										  nSize,
										  g_pVGuiLocalize->Find( "#TF_MM_QueueState_Standby" ),
										  0 );
	}
	else
	{
		if ( GTFPartyClient()->GetNumQueuedMatchGroups() > 1 )
		{
			g_pVGuiLocalize->ConstructString( pwszBuff, 
											  nSize,
											  g_pVGuiLocalize->Find( "#TF_MM_QueueState_Multiple" ),
											  0 );
		}
		else if ( GTFPartyClient()->GetNumQueuedMatchGroups() == 1 )
		{
			auto pMatchGroup = GetMatchGroupDescription( GTFPartyClient()->GetQueuedMatchGroupByIdx( 0 ) );
			g_pVGuiLocalize->ConstructString( pwszBuff, 
											  nSize,
											  g_pVGuiLocalize->Find( "#TF_MM_QueueState_Format" ),
											  1, 
											  g_pVGuiLocalize->Find( pMatchGroup->GetNameLocToken() ) );
		}
	}
}

//
// The HUD element you see in game that says what you're in queue for
//
class CQueueHUDStatus : public EditablePanel,
						public CHudElement
{
	DECLARE_CLASS_SIMPLE( CQueueHUDStatus, EditablePanel );
public:
	CQueueHUDStatus( const char *pElementName )
		: CHudElement( pElementName )
		, EditablePanel( NULL, "QueueHUDStatus" )
	{
		Panel *pParent = g_pClientMode->GetViewport();
		SetParent(pParent);

		ListenForGameEvent( "party_queue_state_changed" );
	}

	virtual void FireGameEvent( IGameEvent * event ) OVERRIDE
	{
		if ( FStrEq( event->GetName(), "party_queue_state_changed" ) )
		{
			wchar_t wszBuff[ 256 ];
			GetQueuedString( wszBuff, sizeof( wszBuff ) );
			SetDialogVariable( "queue_state", wszBuff );
			return;
		}

		CHudElement::FireGameEvent( event );
	}

	virtual void ApplySchemeSettings( IScheme *pScheme ) OVERRIDE
	{
		BaseClass::ApplySchemeSettings( pScheme );

		LoadControlSettings( "resource/UI/InGameQueueStatus.res" );
	}

	virtual bool ShouldDraw()
	{
		return GTFPartyClient()->BInAnyMatchQueue() || GTFPartyClient()->BInStandbyQueue();
	}
};

DECLARE_HUDELEMENT( CQueueHUDStatus );

void CTFMatchmakingDashboard::UpdateQueuePanel()
{
	if ( GTFPartyClient()->BInAnyMatchQueue() || GTFPartyClient()->BInStandbyQueue() )
	{
		// Make sure the panel is in place
		g_pClientMode->GetViewportAnimationController()->RunAnimationCommand( m_pQueuePanel, "ypos", 0, 0.0f, tf_dashboard_slide_time.GetFloat(), vgui::AnimationController::INTERPOLATOR_GAIN, 0.8f, true, false );

		wchar_t wszBuff[ 256 ];
		GetQueuedString( wszBuff, sizeof( wszBuff ) );

		// Update the text to reflect what matchgroup we're in
		m_pQueuePanel->SetDialogVariable( "queue_state", wszBuff );

		//
		// Queue management controls
		//
		{
			bool bCanManageQueue = false;
			if ( GTFPartyClient()->BInAnyMatchQueue() )
			{
				bCanManageQueue = GTFPartyClient()->BControllingPartyActions();
			}
			else 
			{
				bCanManageQueue = true;
			}

			// TODO MultiQueue: Check if queued for multiple queues
			bool bMultiQueue = GTFPartyClient()->GetNumQueuedMatchGroups() > 1;

			m_pQueuePanel->SetControlVisible( "CloseButton", bCanManageQueue && !bMultiQueue );
			m_pQueuePanel->SetControlVisible( "MultiQueuesManageButton", bCanManageQueue && bMultiQueue );
		}

		Panel* pSpinner = m_pQueuePanel->FindChildByName( "CTFLogoPanel" );
		if ( pSpinner )
		{
			Color colorSpinner = GetColor( tf_queue_spinner_color.GetInt() == 0 ? "HUDRedTeamSolid" : "HUDBlueTeamSolid" );
			g_pClientMode->GetViewportAnimationController()->RunAnimationCommand( pSpinner, "fgcolor", colorSpinner, 0.0f, 0.02f, vgui::AnimationController::INTERPOLATOR_GAIN, 0.8f, true, false );
		}
	}
	else
	{
		// Get outta here
		g_pClientMode->GetViewportAnimationController()->RunAnimationCommand( m_pQueuePanel, "ypos", -YRES(50), 0.0f, tf_dashboard_slide_time.GetFloat(), vgui::AnimationController::INTERPOLATOR_GAIN, 0.8f, true, false );
	}
}

void CTFMatchmakingDashboard::UpdateJoinPartyLobbyPanel()
{
	if ( GTFPartyClient()->BCanQueueForStandby() )
	{
		// Make sure the panel is in place
		g_pClientMode->GetViewportAnimationController()->RunAnimationCommand( m_pJoinPartyLobbyPanel, "ypos", 0, 0.0f, tf_dashboard_slide_time.GetFloat(), vgui::AnimationController::INTERPOLATOR_GAIN, 0.8f, true, false );
	}
	else
	{
		// Get outta here
		g_pClientMode->GetViewportAnimationController()->RunAnimationCommand( m_pJoinPartyLobbyPanel, "ypos", -YRES(50), 0.0f, tf_dashboard_slide_time.GetFloat(), vgui::AnimationController::INTERPOLATOR_GAIN, 0.8f, true, false );
	}
}
