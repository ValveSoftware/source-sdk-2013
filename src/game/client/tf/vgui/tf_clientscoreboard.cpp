//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"

#include <tier1/fmtstr.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui_controls/RichText.h>
#include <vgui_controls/Frame.h>
#include <vgui/IScheme.h>
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <vgui/IVGui.h>
#include <vgui_controls/SectionedListPanel.h>
#include <vgui_controls/ImageList.h>
#include <game/client/iviewport.h>
#include <KeyValues.h>
#include <filesystem.h>
#include "IGameUIFuncs.h" // for key bindings
#include "VGuiMatSurface/IMatSystemSurface.h"

#include "tf_controls.h"
#include "tf_shareddefs.h"
#include "tf_playermodelpanel.h"
#include "tf_clientscoreboard.h"
#include "c_playerresource.h"
#include "c_tf_playerresource.h"
#include "c_tf_team.h"
#include "c_tf_player.h"
#include "vgui_avatarimage.h"
#include "tf_gamerules.h"
#include "econ_item_description.h"
#include "vgui_controls/MenuItem.h"
#include "vgui/IInput.h"
#include "voice_status.h"
#include "vgui_controls/ScrollBarSlider.h"
#include "econ/econ_trading.h"
#include "in_buttons.h"
#include "tf_mapinfo.h"

#if defined ( _X360 )
#include "engine/imatchmaking.h"
#endif

#if defined( REPLAY_ENABLED )
#include "replay/ireplaysystem.h"
#include "replay/ienginereplay.h"
#endif

#include "econ_item_system.h"
#include "tf_mann_vs_machine_stats.h"
#include "player_vs_environment/c_tf_upgrades.h"
#include "tf_badge_panel.h"


using namespace vgui;

#define SCOREBOARD_MAX_LIST_ENTRIES 12

ConVar tf_scoreboard_mouse_mode( "tf_scoreboard_mouse_mode", "2", FCVAR_ARCHIVE );
ConVar sv_vote_issue_kick_allowed( "sv_vote_issue_kick_allowed", "0", FCVAR_REPLICATED, "Can players call votes to kick players from the server?" );

ConVar tf_show_all_scoreboard_elements( "tf_show_all_scoreboard_elements", "0", FCVAR_DEVELOPMENTONLY );

void cc_scoreboard_convar_changed( IConVar *pConVar, const char *pOldString, float flOldValue )
{
	CTFClientScoreBoardDialog *pScoreboard = dynamic_cast< CTFClientScoreBoardDialog *>( gViewPortInterface->FindPanelByName( PANEL_SCOREBOARD ) );
	if ( pScoreboard )
	{
		pScoreboard->Reset();
	}
}
ConVar tf_scoreboard_ping_as_text( "tf_scoreboard_ping_as_text", "0", FCVAR_ARCHIVE, "Show ping values as text in the scoreboard.", cc_scoreboard_convar_changed );
ConVar tf_scoreboard_alt_class_icons( "tf_scoreboard_alt_class_icons", "0", FCVAR_ARCHIVE, "Show alternate class icons in the scoreboard." );

extern bool IsInCommentaryMode( void );
extern bool DuelMiniGame_GetStats( C_TFPlayer **ppPlayer, uint32 &unMyScore, uint32 &unOpponentScore );
extern void AddSubKeyNamed( KeyValues *pKeys, const char *pszName );

extern ConVar cl_hud_playerclass_use_playermodel;
extern ConVar mp_tournament;

//......................................................
enum
{
	PING_LOW,
	PING_MED,
	PING_HIGH,
	PING_VERY_HIGH,
	PING_BOT_RED,
	PING_BOT_BLUE,

	PING_MAX_TYPES
};
static const char *pszPingIcons[SCOREBOARD_PING_ICONS] = {
	"../hud/scoreboard_ping_low",
	"../hud/scoreboard_ping_med",
	"../hud/scoreboard_ping_high",
	"../hud/scoreboard_ping_very_high",
	"../hud/scoreboard_ping_bot_red",
	"../hud/scoreboard_ping_bot_blue",
};
static const char *pszPingIconsDead[SCOREBOARD_PING_ICONS] = {
	"../hud/scoreboard_ping_low_d",
	"../hud/scoreboard_ping_med_d",
	"../hud/scoreboard_ping_high_d",
	"../hud/scoreboard_ping_very_high_d",
	"../hud/scoreboard_ping_bot_red_d",
	"../hud/scoreboard_ping_bot_blue_d",
};
COMPILE_TIME_ASSERT( SCOREBOARD_PING_ICONS == PING_MAX_TYPES );
//......................................................

static const char *pszDominationIcons[SCOREBOARD_DOMINATION_ICONS] = {
	"",
	"../hud/leaderboard_dom1",
	"../hud/leaderboard_dom2",
	"../hud/leaderboard_dom3",
	"../hud/leaderboard_dom4",
	"../hud/leaderboard_dom5",
	"../hud/leaderboard_dom6",
	"../hud/leaderboard_dom7",
	"../hud/leaderboard_dom8",
	"../hud/leaderboard_dom9",
	"../hud/leaderboard_dom10",
	"../hud/leaderboard_dom11",
	"../hud/leaderboard_dom12",
	"../hud/leaderboard_dom13",
	"../hud/leaderboard_dom14",
	"../hud/leaderboard_dom15",
	"../hud/leaderboard_dom16",
};

static const char *pszDominationIconsDead[SCOREBOARD_DOMINATION_ICONS] = {
	"",
	"../hud/leaderboard_dom1_d",
	"../hud/leaderboard_dom2_d",
	"../hud/leaderboard_dom3_d",
	"../hud/leaderboard_dom4_d",
	"../hud/leaderboard_dom5_d",
	"../hud/leaderboard_dom6_d",
	"../hud/leaderboard_dom7_d",
	"../hud/leaderboard_dom8_d",
	"../hud/leaderboard_dom9_d",
	"../hud/leaderboard_dom10_d",
	"../hud/leaderboard_dom11_d",
	"../hud/leaderboard_dom12_d",
	"../hud/leaderboard_dom13_d",
	"../hud/leaderboard_dom14_d",
	"../hud/leaderboard_dom15_d",
	"../hud/leaderboard_dom16_d",
};

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFClientScoreBoardDialog::CTFClientScoreBoardDialog( IViewPort *pViewPort ) : CClientScoreBoardDialog( pViewPort )
{
	SetProportional( true );
	SetKeyBoardInputEnabled( false );
	SetScheme( "ClientScheme" );

	m_pPlayerListBlue = new SectionedListPanel( this, "BluePlayerList" );
	m_pPlayerListRed = new SectionedListPanel( this, "RedPlayerList" );
	m_pLabelPlayerName = new CExLabel( this, "PlayerNameLabel", "" );
	m_pImagePanelHorizLine = new ImagePanel( this, "HorizontalLine" );
	m_pClassImage = new CTFClassImage( this, "ClassImage" );
	m_pPlayerModelPanel = new CTFPlayerModelPanel( this, "classmodelpanel" );
	m_pLocalPlayerStatsPanel = new vgui::EditablePanel( this, "LocalPlayerStatsPanel" );
	m_pLocalPlayerDuelStatsPanel = new vgui::EditablePanel( this, "LocalPlayerDuelStatsPanel" );
	m_duelPanelLocalPlayer.m_pPanel = new vgui::EditablePanel( m_pLocalPlayerDuelStatsPanel, "LocalPlayerData" );
	m_duelPanelOpponent.m_pPanel = new vgui::EditablePanel( m_pLocalPlayerDuelStatsPanel, "OpponentData" );
	m_pRedTeamName = new CExLabel( this, "RedTeamLabel", "" );
	m_pBlueTeamName = new CExLabel( this, "BlueTeamLabel", "" );
	m_pRedLeaderAvatarImage = new CAvatarImagePanel( this, "RedLeaderAvatar" );
	m_pRedLeaderAvatarBG = new EditablePanel( this, "RedLeaderAvatarBG" );
	m_pRedTeamImage = new ImagePanel( this, "RedTeamImage" );
	m_pBlueLeaderAvatarImage = new CAvatarImagePanel( this, "BlueLeaderAvatar" );
	m_pBlueLeaderAvatarBG = new EditablePanel( this, "BlueLeaderAvatarBG" );
	m_pBlueTeamImage = new ImagePanel( this, "BlueTeamImage" );

	m_pServerTimeLeftValue = NULL;
	m_pFontTimeLeftNumbers = vgui::INVALID_FONT;
	m_pFontTimeLeftString = vgui::INVALID_FONT;

	m_pMvMScoreboard = new CTFHudMannVsMachineScoreboard( this, "MvMScoreboard" );
	m_pRightClickMenu = NULL;

	m_iImageDominated = 0;
	m_iImageDominatedDead = 0;
	m_iImageNemesis = 0;
	m_iImageNemesisDead = 0;
	m_iImageStreak = 0;
	m_iImageStreakDead = 0;
	m_iTextureCamera = -1;

	m_bIsPVEMode = false;
//	m_bDisplayLevel = false;
	m_bMouseActivated = true;

	Q_memset( m_iImageClass, NULL, sizeof( m_iImageClass ) );
	Q_memset( m_iImageClassAlt, NULL, sizeof( m_iImageClassAlt ) );
	Q_memset( m_iImageDom, NULL, sizeof( m_iImageDom ) );
	Q_memset( m_iImageDomDead, NULL, sizeof( m_iImageDomDead ) );
	
	ListenForGameEvent( "server_spawn" );

	SetDialogVariable( "server", "" );
	SetVisible( false );

	m_nPlayerModelPanelIndex = -1;
	m_bRedScrollBarVisible = false;
	m_bBlueScrollBarVisible = false;
	m_nExtraSpace = 0;
	m_hSelectedPlayer = NULL;
	m_bUsePlayerModel = false;
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CTFClientScoreBoardDialog::~CTFClientScoreBoardDialog()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFClientScoreBoardDialog::PerformLayout()
{
	BaseClass::PerformLayout();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFClientScoreBoardDialog::UpdatePlayerModel()
{
	if ( !m_pPlayerModelPanel->IsVisible() )
		return;

	if ( m_nPlayerModelPanelIndex == -1 )
	{
		m_nPlayerModelPanelIndex = GetLocalPlayerIndex();
	}

	C_TFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( m_nPlayerModelPanelIndex ) );
	if ( !pPlayer )
		return;

	int nClass = pPlayer->GetPlayerClass()->GetClassIndex();
	int nTeam = pPlayer->GetTeamNumber();
	int nItemSlot = ( pPlayer->IsAlive() && pPlayer->GetActiveTFWeapon() ) ? pPlayer->GetActiveTFWeapon()->GetAttributeContainer()->GetItem()->GetStaticData()->GetLoadoutSlot( nClass ) : LOADOUT_POSITION_PRIMARY;
	CEconItemView *pWeapon = NULL;

	CTFWeaponBase *pEnt = dynamic_cast<CTFWeaponBase*>( pPlayer->GetEntityForLoadoutSlot( nItemSlot ) );
	if ( pEnt )
	{
		pWeapon = pEnt->GetAttributeContainer()->GetItem();
	}

	m_pPlayerModelPanel->ClearCarriedItems();
	m_pPlayerModelPanel->SetToPlayerClass( nClass );
	m_pPlayerModelPanel->SetTeam( nTeam );

	if ( pWeapon )
	{
		m_pPlayerModelPanel->AddCarriedItem( pWeapon );
	}

	for ( int wbl = pPlayer->GetNumWearables() - 1; wbl >= 0; wbl-- )
	{
		C_TFWearable *pItem = dynamic_cast<C_TFWearable*>( pPlayer->GetWearable( wbl ) );
		if ( !pItem )
			continue;

		if ( pItem->IsViewModelWearable() )
			continue;

		if ( pItem->IsDisguiseWearable() )
			continue;

		CAttributeContainer *pCont = pItem->GetAttributeContainer();
		CEconItemView		*pEconItemView = pCont ? pCont->GetItem() : NULL;

		if ( pEconItemView && pEconItemView->IsValid() )
		{
			m_pPlayerModelPanel->AddCarriedItem( pEconItemView );
		}
	}

	m_pPlayerModelPanel->HoldItemInSlot( nItemSlot );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFClientScoreBoardDialog::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	KeyValues *pConditions = NULL;

	if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() )
	{
		pConditions = new KeyValues( "conditions" );
		AddSubKeyNamed( pConditions, "if_mvm" );
	}

	LoadControlSettings( "Resource/UI/Scoreboard.res", NULL, NULL, pConditions );
	m_hScoreFontDefault = pScheme->GetFont( "Default", true );
	m_hScoreFontSmallest = pScheme->GetFont( "ScoreboardSmallest", true );

	if ( pConditions )
	{
		pConditions->deleteThis();
	}
	 
	if ( m_pImageList )
	{
		m_iImageDominated = m_pImageList->AddImage( scheme()->GetImage( "../hud/leaderboard_dominated", true ) );
		m_iImageDominatedDead = m_pImageList->AddImage( scheme()->GetImage( "../hud/leaderboard_dominated_d", true ) );
		m_iImageNemesis = m_pImageList->AddImage( scheme()->GetImage( "../hud/leaderboard_nemesis", true ) );
		m_iImageNemesisDead = m_pImageList->AddImage( scheme()->GetImage( "../hud/leaderboard_nemesis_d", true ) );
		m_iImageStreak = m_pImageList->AddImage( scheme()->GetImage( "../hud/scoreboard_streak", true ) );
		m_iImageStreakDead = m_pImageList->AddImage( scheme()->GetImage( "../hud/scoreboard_streak_d", true ) );

		for(int i = 1 ; i < SCOREBOARD_DOMINATION_ICONS ; i++)
		{
			m_iImageDom[i] = m_pImageList->AddImage( scheme()->GetImage( pszDominationIcons[i], true ) );
			m_iImageDomDead[i] = m_pImageList->AddImage( scheme()->GetImage( pszDominationIconsDead[i], true ) );
		}
		for( int i = 1 ; i < SCOREBOARD_CLASS_ICONS ; i++ )
		{
			m_iImageClass[i] = m_pImageList->AddImage( scheme()->GetImage( g_pszClassIcons[i], true ) );
			m_iImageClassAlt[i] = m_pImageList->AddImage( scheme()->GetImage( g_pszClassIconsAlt[i], true ) );
		}
		for ( int i = 0; i < SCOREBOARD_PING_ICONS; i++ )
		{
			m_iImagePing[i] = m_pImageList->AddImage( scheme()->GetImage( pszPingIcons[i], true ) );
			m_iImagePingDead[i] = m_pImageList->AddImage( scheme()->GetImage( pszPingIconsDead[i], true ) );
		}

		// resize the images to our resolution
		for (int i = 1 ; i < m_pImageList->GetImageCount(); i++ )
		{
			int wide = 13, tall = 13;
			m_pImageList->GetImage(i)->SetSize(scheme()->GetProportionalScaledValueEx( GetScheme(), wide ), scheme()->GetProportionalScaledValueEx( GetScheme(),tall ) );
		}
	}

	SetPlayerListImages( m_pPlayerListBlue );
	SetPlayerListImages( m_pPlayerListRed );

	SetBgColor( Color( 0, 0, 0, 0) );
	SetBorder( NULL );
	SetVisible( false );

	m_duelPanelLocalPlayer.m_pAvatar = dynamic_cast< CAvatarImagePanel *>( m_duelPanelLocalPlayer.m_pPanel->FindChildByName("AvatarImage") );
	m_duelPanelLocalPlayer.m_pPlayerNameLabel = dynamic_cast< CExLabel *>( m_duelPanelLocalPlayer.m_pPanel->FindChildByName("AvatarTextLabel") );
	m_duelPanelOpponent.m_pAvatar = dynamic_cast< CAvatarImagePanel *>( m_duelPanelOpponent.m_pPanel->FindChildByName("AvatarImage") );
	m_duelPanelOpponent.m_pPlayerNameLabel = dynamic_cast< CExLabel *>( m_duelPanelOpponent.m_pPanel->FindChildByName("AvatarTextLabel") );

	if ( m_pLocalPlayerStatsPanel )
	{
		m_pKillsLabel = dynamic_cast< CExLabel* >( m_pLocalPlayerStatsPanel->FindChildByName( "Kills" ) );
		m_pDeathsLabel = dynamic_cast< CExLabel* >( m_pLocalPlayerStatsPanel->FindChildByName( "Deaths" ) );
		m_pAssistLabel = dynamic_cast< CExLabel* >( m_pLocalPlayerStatsPanel->FindChildByName( "Assists" ) );
		m_pDestructionLabel = dynamic_cast< CExLabel* >( m_pLocalPlayerStatsPanel->FindChildByName( "Destruction" ) );
		m_pCapturesLabel = dynamic_cast< CExLabel* >( m_pLocalPlayerStatsPanel->FindChildByName( "Captures" ) );
		m_pDefensesLabel = dynamic_cast< CExLabel* >( m_pLocalPlayerStatsPanel->FindChildByName( "Defenses" ) );
		m_pDominationsLabel = dynamic_cast< CExLabel* >( m_pLocalPlayerStatsPanel->FindChildByName( "Domination" ) );
		m_pRevengeLabel = dynamic_cast< CExLabel* >( m_pLocalPlayerStatsPanel->FindChildByName( "Revenge" ) );
		m_pHealingLabel = dynamic_cast< CExLabel* >( m_pLocalPlayerStatsPanel->FindChildByName( "Healing" ) );
		m_pInvulnsLabel = dynamic_cast< CExLabel* >( m_pLocalPlayerStatsPanel->FindChildByName( "Invuln" ) );
		m_pTeleportsLabel = dynamic_cast< CExLabel* >( m_pLocalPlayerStatsPanel->FindChildByName( "Teleports" ) );
		m_pHeadshotsLabel = dynamic_cast< CExLabel* >( m_pLocalPlayerStatsPanel->FindChildByName( "Headshots" ) );
		m_pBackstabsLabel = dynamic_cast< CExLabel* >( m_pLocalPlayerStatsPanel->FindChildByName( "Backstabs" ) );
		m_pBonusLabel = dynamic_cast< CExLabel* >( m_pLocalPlayerStatsPanel->FindChildByName( "Bonus" ) );
		m_pSupportLabel = dynamic_cast< CExLabel* >( m_pLocalPlayerStatsPanel->FindChildByName( "Support" ) );
		m_pDamageLabel = dynamic_cast< CExLabel* >( m_pLocalPlayerStatsPanel->FindChildByName( "Damage" ) );
	}

	m_pServerTimeLeftValue = dynamic_cast< CExLabel* >( FindChildByName( "ServerTimeLeftValue" ) );
	m_pFontTimeLeftNumbers = pScheme->GetFont( "ScoreboardMediumSmall", true );
	m_pFontTimeLeftString = pScheme->GetFont( "ScoreboardVerySmall", true );

	Reset();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFClientScoreBoardDialog::ShowPanel( bool bShow )
{
	if ( bShow )
	{
		if ( TFGameRules() && TFGameRules()->ShowMatchSummary() )
			return;
	}

	// Catch the case where we call ShowPanel before ApplySchemeSettings, eg when
	// going from windowed <-> fullscreen
	if ( m_pImageList == NULL )
	{
		InvalidateLayout( true, true );
	}

	bool bIsPVEMode = TFGameRules() && TFGameRules()->IsPVEModeActive();
	if ( m_bIsPVEMode != bIsPVEMode )
	{
		m_bIsPVEMode = bIsPVEMode;
		InvalidateLayout( true, true );		
	}

	// Don't show in commentary mode
	if ( IsInCommentaryMode() )
	{
		bShow = false;
	}
	
	if ( IsVisible() == bShow )
	{
		return;
	}

	int iRenderGroup = gHUD.LookupRenderGroupIndexByName( "global" );

	if ( bShow )
	{		
		SetVisible( true );
		MoveToFront();
		InitializeInputScheme();

		gHUD.LockRenderGroup( iRenderGroup );

		// Clear the selected item, this forces the default to the local player
		SectionedListPanel *pList = GetSelectedPlayerList();
		if ( pList )
		{
			pList->ClearSelection();
		}

		if ( tf_scoreboard_mouse_mode.GetInt() == 2 )
			m_bMouseActivated = false;

		UpdateServerTimeLeft();

		m_nPlayerModelPanelIndex = -1;
		m_hSelectedPlayer = NULL;
		UpdatePlayerModel();
	}
	else
	{
		SetVisible( false );

		if ( m_pRightClickMenu )
		{
			delete m_pRightClickMenu;
			m_pRightClickMenu = NULL;
		}

		gHUD.UnlockRenderGroup( iRenderGroup );
		m_bMouseActivated = false;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFClientScoreBoardDialog::OnCommand( const char *command )
{
	if ( V_stristr( command, "report_player" ) )
	{
		engine->ClientCmd( command );
	}
	else if ( !V_strcmp( command, "muteplayer" ) )
	{
		SectionedListPanel *pList = GetSelectedPlayerList();
		if ( pList )
		{
			int iSelectedItem = pList->GetSelectedItem();
			if ( iSelectedItem >= 0 )
			{
				KeyValues *pIssueKeyValues = pList->GetItemData( iSelectedItem );
				if ( !pIssueKeyValues )
					return;

				int playerIndex = pIssueKeyValues->GetInt( "playerIndex", 0 );
				if ( playerIndex > 0 && playerIndex <= MAX_PLAYERS && GetClientVoiceMgr() )
				{
					// Toggle
					GetClientVoiceMgr()->SetPlayerBlockedState( playerIndex, ( GetClientVoiceMgr()->IsPlayerBlocked( playerIndex ) ? false : true ) );
				}
			}
		}
	}
	else if ( !V_strcmp( command, "specplayer" ) )
	{
		SectionedListPanel *pList = GetSelectedPlayerList();
		if ( pList )
		{
			int iSelectedItem = pList->GetSelectedItem();
			if ( iSelectedItem >= 0 )
			{
				KeyValues *pIssueKeyValues = pList->GetItemData( iSelectedItem );
				if ( !pIssueKeyValues )
					return;

				int playerIndex = pIssueKeyValues->GetInt( "playerIndex", 0 );
				if ( playerIndex > 0 && playerIndex <= MAX_PLAYERS )
				{
					engine->ClientCmd_Unrestricted( VarArgs( "spec_player %d\n", playerIndex ) );
				}
			}
		}
	}
	else if ( !V_strcmp( command, "friendadd" ) )
	{
		SectionedListPanel *pList = GetSelectedPlayerList();
		if ( pList )
		{
			int iSelectedItem = pList->GetSelectedItem();
			if ( iSelectedItem >= 0 )
			{
				KeyValues *pIssueKeyValues = pList->GetItemData( iSelectedItem );
				if ( !pIssueKeyValues )
					return;

				int playerIndex = pIssueKeyValues->GetInt( "playerIndex", 0 );
				CBasePlayer *pTarget = UTIL_PlayerByIndex( playerIndex );
				if ( pTarget && !( pTarget->IsBot() || pTarget->IsHLTV() ) )
				{
					CSteamID steamID;
					if ( pTarget->GetSteamID( &steamID ) && steamapicontext && steamapicontext->SteamFriends() )
					{
						steamapicontext->SteamFriends()->ActivateGameOverlayToUser( "friendadd", steamID );
					}
				}
			}
		}
	}
	else if ( !V_strcmp( command, "jointrade" ) )
	{
		SectionedListPanel *pList = GetSelectedPlayerList();
		if ( pList )
		{
			int iSelectedItem = pList->GetSelectedItem();
			if ( iSelectedItem >= 0 )
			{
				KeyValues *pIssueKeyValues = pList->GetItemData( iSelectedItem );
				if ( !pIssueKeyValues )
					return;

				int playerIndex = pIssueKeyValues->GetInt( "playerIndex", 0 );
				CBasePlayer *pTarget = UTIL_PlayerByIndex( playerIndex );
				if ( pTarget && !( pTarget->IsBot() || pTarget->IsHLTV() ) )
				{
					// Prevent large UI popup during a match
					if ( pTarget->GetTeamNumber() >= FIRST_GAME_TEAM )
					{
						if ( TFGameRules() && TFGameRules()->UsePlayerReadyStatusMode() && TFGameRules()->State_Get() == GR_STATE_RND_RUNNING )
							return;
					}

					Trading_RequestTrade( playerIndex );
				}
			}
		}
	}
	else if ( !V_strcmp( command, "profile" ) )
	{
		SectionedListPanel *pList = GetSelectedPlayerList();
		if ( pList )
		{
			int iSelectedItem = pList->GetSelectedItem();
			if ( iSelectedItem >= 0 )
			{
				KeyValues *pIssueKeyValues = pList->GetItemData( iSelectedItem );
				if ( !pIssueKeyValues )
					return;

				int playerIndex = pIssueKeyValues->GetInt( "playerIndex", 0 );
				CBasePlayer *pTarget = UTIL_PlayerByIndex( playerIndex );
				if ( pTarget && !( pTarget->IsBot() || pTarget->IsHLTV() ) )
				{
					CSteamID steamID;
					if ( pTarget->GetSteamID( &steamID ) && steamapicontext && steamapicontext->SteamFriends() )
					{
						steamapicontext->SteamFriends()->ActivateGameOverlayToUser( "steamid", steamID );
					}
				}
			}
		}
	}
	else if ( !V_strcmp( command, "votekickplayer" ) )
	{
		SectionedListPanel* pList = GetSelectedPlayerList();
		if ( pList )
		{
			int iSelectedItem = pList->GetSelectedItem();
			if ( iSelectedItem >= 0 )
			{
				KeyValues* pIssueKeyValues = pList->GetItemData( iSelectedItem );
				if ( !pIssueKeyValues )
					return;

				int playerIndex = pIssueKeyValues->GetInt( "playerIndex", 0 );
				if ( playerIndex > 0 && playerIndex <= MAX_PLAYERS )
				{
					engine->ClientCmd_Unrestricted( VarArgs( "spec_player %d\n", playerIndex ) );
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFClientScoreBoardDialog::OnItemSelected( vgui::Panel *panel )
{
	if ( panel == m_pPlayerListBlue || panel == m_pPlayerListRed )
	{
		// There can be only one... selection
		if ( panel == m_pPlayerListBlue && m_pPlayerListBlue->GetSelectedItem() >= 0 && m_pPlayerListRed->GetSelectedItem() >= 0 )
		{
			m_pPlayerListRed->ClearSelection();
		}
		else if ( panel == m_pPlayerListRed && m_pPlayerListRed->GetSelectedItem() >= 0 && m_pPlayerListBlue->GetSelectedItem() >= 0 )
		{
			m_pPlayerListBlue->ClearSelection();
		}

		if ( vgui::input()->IsMouseDown( MOUSE_RIGHT ) )
		{
			OnScoreBoardMouseRightRelease();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFClientScoreBoardDialog::OnItemContextMenu( vgui::Panel *panel )
{
	if ( panel == m_pPlayerListBlue || panel == m_pPlayerListRed )
	{
		if ( vgui::input()->IsMouseDown( MOUSE_RIGHT ) )
		{
			OnScoreBoardMouseRightRelease();
		}
	}
}


void CTFClientScoreBoardDialog::OnVoteKickPlayer( KeyValues *pData )
{
	int playerIndex = pData->GetInt( "playerIndex" );
	const char *pszReason = pData->GetString( "reason" );

	char szVoteCommand[256];
	Q_snprintf( szVoteCommand, sizeof( szVoteCommand ), "callvote %s \"%d %s\"\n;", "Kick", g_PR->GetUserID( playerIndex ), pszReason );
	engine->ClientCmd( szVoteCommand );
}

//-----------------------------------------------------------------------------
// Purpose: Figure out where to put the camera view
//-----------------------------------------------------------------------------
void CTFClientScoreBoardDialog::GetCameraUnderlayBounds( int *pX, int *pY, int *pWide, int *pTall )
{
	GetBounds( *pX, *pY, *pWide, *pTall );

	// inset these b*pY a bit
	*pX += 15;
	*pY += 25;
	*pWide -= 30;
	*pTall -= 40;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFClientScoreBoardDialog::OnScoreBoardMouseRightRelease( void )
{
	if ( !IsVisible() )
		return;

	C_TFPlayer *pLocalTFPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pLocalTFPlayer )
		return;

	SectionedListPanel *pList = GetSelectedPlayerList();
	if ( !pList )
		return;

	int iSelectedItem = pList->GetSelectedItem();
	if ( iSelectedItem < 0 )
		return;

	KeyValues *pIssueKeyValues = pList->GetItemData( iSelectedItem );
	if ( !pIssueKeyValues )
		return;
		
	int playerIndex = pIssueKeyValues->GetInt( "playerIndex", 0 );
	if ( GetLocalPlayerIndex() == playerIndex )
		return;

	if ( m_pRightClickMenu )
		delete m_pRightClickMenu;

	const char *pszContextMenuBorder = "DarkComboBoxBorder";
	const char *pszContextMenuFont = "HudFontMediumSecondary";
	m_pRightClickMenu = new Menu( pList, "RightClickMenu" );
	m_pRightClickMenu->SetProportional( true );
	m_pRightClickMenu->SetKeyBoardInputEnabled( false );
	m_pRightClickMenu->SetBorder( scheme()->GetIScheme( GetScheme() )->GetBorder( pszContextMenuBorder ) );
	m_pRightClickMenu->SetFgColor( Color( 0, 0, 255, 255 ) );
	m_pRightClickMenu->SetPaintBackgroundEnabled( true );
	m_pRightClickMenu->SetPaintBackgroundType( 0 );
	m_pRightClickMenu->SetFont( scheme()->GetIScheme( GetScheme() )->GetFont( pszContextMenuFont, true ) );

	bool bFakeClient = ( g_TF_PR->IsFakePlayer( playerIndex ) );
	bool bTournamentGame = ( g_TF_PR->GetTeam( playerIndex ) >= FIRST_GAME_TEAM && TFGameRules() && TFGameRules()->UsePlayerReadyStatusMode() && TFGameRules()->State_Get() == GR_STATE_RND_RUNNING );

	MenuBuilder contextMenuBuilder( m_pRightClickMenu, this );


	// Vote Kick
	if ( sv_vote_issue_kick_allowed.GetBool() && engine->GetLocalPlayer() != playerIndex )
	{
		C_TFPlayer* pTFTarget = ToTFPlayer( UTIL_PlayerByIndex( playerIndex ) );
		if ( pTFTarget && pTFTarget->GetTeamNumber() == pLocalTFPlayer->GetTeamNumber() )
		{
			Menu *pVoteKickSubMenu = new Menu( this, "VoteKickSubMenu" );
			pVoteKickSubMenu->SetBorder( scheme()->GetIScheme( GetScheme() )->GetBorder( pszContextMenuBorder ) );
			pVoteKickSubMenu->SetFont( scheme()->GetIScheme( GetScheme() )->GetFont( pszContextMenuFont, true ) );
			contextMenuBuilder.AddCascadingMenuItem( "#TF_ScoreBoard_VoteKick", pVoteKickSubMenu, "votekick" );

			struct VoteKickReason_t
			{
				const char *pszReason;
				const char *pszText;
			} g_pszVoteKickReasons[] =
			{
				{ "other", "#TF_VoteKickReason_Other" },
				{ "cheating", "#TF_VoteKickReason_Cheating" },
				{ "idle", "#TF_VoteKickReason_Idle" },
				{ "scamming", "#TF_VoteKickReason_Scamming" },
			};

			for ( auto &reason : g_pszVoteKickReasons )
			{
				KeyValues *pReport = new KeyValues( "VoteKickPlayer" );
				pReport->SetInt( "playerIndex", playerIndex );
				pReport->SetString( "reason", reason.pszReason );
				pVoteKickSubMenu->AddMenuItem( reason.pszText, pReport, this );
			}
		}
	}

	// Mute
	if ( !bFakeClient && GetClientVoiceMgr() )
	{
		const char *pszString = GetClientVoiceMgr()->IsPlayerBlocked( playerIndex ) ? "#TF_ScoreBoard_Context_UnMute" : "#TF_ScoreBoard_Context_Mute";
		contextMenuBuilder.AddMenuItem( pszString, "muteplayer", "mute" );
	}

	// Spectate
	if ( g_TF_PR->IsAlive( playerIndex ) && ( !pLocalTFPlayer->IsAlive() || pLocalTFPlayer->GetTeamNumber() == TEAM_SPECTATOR ) )
	{
		contextMenuBuilder.AddMenuItem( "#TF_ScoreBoard_Context_Spec", "specplayer", "spectate" );
	}

	// Add Friend
	if ( !bFakeClient )
	{
		CSteamID steamID;
		C_TFPlayer *pTFTarget = ToTFPlayer( UTIL_PlayerByIndex( playerIndex ) );
		if ( pTFTarget && pTFTarget->GetSteamID( &steamID ) )
		{
			if ( steamapicontext && steamapicontext->SteamFriends() && steamapicontext->SteamFriends()->GetFriendRelationship( steamID ) == k_EFriendRelationshipNone )
			{
				contextMenuBuilder.AddMenuItem( "#TF_ScoreBoard_Context_Friend", "friendadd", "friend" );
			}
		}
	}
	
	// Trade
	if ( !bFakeClient && !bTournamentGame )
	{
		contextMenuBuilder.AddMenuItem( "#TF_ScoreBoard_Context_Trade", "jointrade", "trade" );
	}

	// Profile
	if ( !bFakeClient )
	{
		contextMenuBuilder.AddMenuItem( "#TF_ScoreBoard_Context_Profile", "profile", "profile" );
	}

	int x, y;
	vgui::input()->GetCursorPosition( x, y );
	m_pRightClickMenu->SetPos( x, y );
	m_pRightClickMenu->SetVisible( true );
	m_pRightClickMenu->AddActionSignalTarget( this );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFClientScoreBoardDialog::UseMouseMode( void )
{
	return tf_scoreboard_mouse_mode.GetInt() == 1 || ( tf_scoreboard_mouse_mode.GetInt() == 2 && m_bMouseActivated );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFClientScoreBoardDialog::InitializeInputScheme( void )
{
	if ( !IsVisible() )
		return;

	if ( !m_pPlayerListBlue || !m_pPlayerListRed )
		return;

	MakePopup( false, UseMouseMode() );
	SetKeyBoardInputEnabled( false );
	SetMouseInputEnabled( UseMouseMode() );
	m_pPlayerListBlue->SetMouseInputEnabled( UseMouseMode() );
	m_pPlayerListBlue->AddActionSignalTarget( this );
	m_pPlayerListBlue->SetClickable( UseMouseMode() );
	m_pPlayerListRed->SetMouseInputEnabled( UseMouseMode() );
	m_pPlayerListRed->AddActionSignalTarget( this );
	m_pPlayerListRed->SetClickable( UseMouseMode() );

	if ( m_pPlayerListBlue->GetScrollBar() &&
		m_pPlayerListBlue->GetScrollBar()->GetButton( 0 ) &&
		m_pPlayerListBlue->GetScrollBar()->GetButton( 1 ) &&
		m_pPlayerListBlue->GetScrollBar()->GetSlider() )
	{
		m_pPlayerListBlue->SetVerticalScrollbar( UseMouseMode() );
		m_pPlayerListBlue->GetScrollBar()->GetButton( 0 )->SetMouseInputEnabled( UseMouseMode() );
		m_pPlayerListBlue->GetScrollBar()->GetButton( 1 )->SetMouseInputEnabled( UseMouseMode() );
		m_pPlayerListBlue->GetScrollBar()->GetSlider()->SetMouseInputEnabled( UseMouseMode() );
		m_pPlayerListBlue->GetScrollBar()->SetMouseInputEnabled( UseMouseMode() );
	}

	if ( m_pPlayerListRed->GetScrollBar() &&
		m_pPlayerListRed->GetScrollBar()->GetButton( 0 ) &&
		m_pPlayerListRed->GetScrollBar()->GetButton( 1 ) &&
		m_pPlayerListRed->GetScrollBar()->GetSlider() )
	{
		m_pPlayerListRed->SetVerticalScrollbar( UseMouseMode() );
		m_pPlayerListRed->GetScrollBar()->SetMouseInputEnabled( UseMouseMode() );
		m_pPlayerListRed->GetScrollBar()->GetButton( 0 )->SetMouseInputEnabled( UseMouseMode() );
		m_pPlayerListRed->GetScrollBar()->GetButton( 1 )->SetMouseInputEnabled( UseMouseMode() );
		m_pPlayerListRed->GetScrollBar()->GetSlider()->SetMouseInputEnabled( UseMouseMode() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Resets the scoreboard panel
//-----------------------------------------------------------------------------
void CTFClientScoreBoardDialog::Reset()
{
	if ( !m_bIsPVEMode )
	{
		InitPlayerList( m_pPlayerListBlue );
		InitPlayerList( m_pPlayerListRed );
	}

 	m_pPlayerListBlue->SetVisible( !m_bIsPVEMode );
 	m_pPlayerListRed->SetVisible( !m_bIsPVEMode );
}

//-----------------------------------------------------------------------------
// Purpose: Inits the player list in a list panel
//-----------------------------------------------------------------------------
void CTFClientScoreBoardDialog::InitPlayerList( SectionedListPanel *pPlayerList )
{
	pPlayerList->SetVerticalScrollbar( UseMouseMode() );
	pPlayerList->RemoveAll();
	pPlayerList->RemoveAllSections();
	pPlayerList->AddSection( 0, "Players", TFPlayerSortFunc );
	pPlayerList->SetSectionAlwaysVisible( 0, true );
	pPlayerList->SetSectionFgColor( 0, Color( 255, 255, 255, 255 ) );
	pPlayerList->SetBgColor( Color( 0, 0, 0, 0 ) );
	pPlayerList->SetBorder( NULL );

	pPlayerList->AddColumnToSection( 0, "medal", "", SectionedListPanel::COLUMN_IMAGE | SectionedListPanel::COLUMN_CENTER, m_iMedalColumnWidth );

	// Avatars are always displayed at 32x32 regardless of resolution
	if ( ShowAvatars() )
	{
		pPlayerList->AddColumnToSection( 0, "avatar", "", SectionedListPanel::COLUMN_IMAGE, m_iAvatarWidth );
		pPlayerList->AddColumnToSection( 0, "spacer", "", 0, m_iSpacerWidth );
	}
	
	// the player avatar is always a fixed size, so as we change resolutions we need to vary the size of the name column to adjust the total width of all the columns
	m_nExtraSpace = pPlayerList->GetWide() - m_iMedalColumnWidth - m_iAvatarWidth - m_iSpacerWidth - m_iNameWidth - m_iKillstreakWidth - m_iKillstreakImageWidth - m_iNemesisWidth - m_iNemesisWidth - m_iScoreWidth - m_iClassWidth - m_iPingWidth - m_iSpacerWidth - ( 2 * SectionedListPanel::COLUMN_DATA_INDENT ); // the SectionedListPanel will indent the columns on either end by SectionedListPanel::COLUMN_DATA_INDENT 

	pPlayerList->AddColumnToSection( 0, "name", "#TF_Scoreboard_Name", 0, m_iNameWidth + m_nExtraSpace );
	pPlayerList->AddColumnToSection( 0, "killstreak", "", SectionedListPanel::COLUMN_RIGHT, m_iKillstreakWidth );
	pPlayerList->AddColumnToSection( 0, "killstreak_image", "", SectionedListPanel::COLUMN_IMAGE, m_iKillstreakImageWidth );
	pPlayerList->AddColumnToSection( 0, "dominating", "", SectionedListPanel::COLUMN_IMAGE | SectionedListPanel::COLUMN_CENTER, m_iNemesisWidth );
	pPlayerList->AddColumnToSection( 0, "nemesis", "", SectionedListPanel::COLUMN_IMAGE | SectionedListPanel::COLUMN_CENTER, m_iNemesisWidth );
	pPlayerList->AddColumnToSection( 0, "score", "#TF_Scoreboard_Score", SectionedListPanel::COLUMN_RIGHT, m_iScoreWidth );
	pPlayerList->AddColumnToSection( 0, "class", "", SectionedListPanel::COLUMN_IMAGE | SectionedListPanel::COLUMN_RIGHT, m_iClassWidth );

	if ( tf_scoreboard_ping_as_text.GetBool() )
	{
		pPlayerList->AddColumnToSection( 0, "ping", "#TF_Scoreboard_Ping", SectionedListPanel::COLUMN_RIGHT, m_iPingWidth );
	}
	else
	{
		pPlayerList->AddColumnToSection( 0, "ping", "", SectionedListPanel::COLUMN_IMAGE | SectionedListPanel::COLUMN_RIGHT, m_iPingWidth );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Builds the image list to use in the player list
//-----------------------------------------------------------------------------
void CTFClientScoreBoardDialog::SetPlayerListImages( vgui::SectionedListPanel *pPlayerList )
{
	pPlayerList->SetImageList( m_pImageList, false );
	pPlayerList->SetVisible( true );
}

//-----------------------------------------------------------------------------
// Purpose: Updates the dialog
//-----------------------------------------------------------------------------
void CTFClientScoreBoardDialog::Update()
{
	// MvM
	if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() )
	{
		m_pMvMScoreboard->OnTick();
	}

	UpdateTeamInfo();
	UpdatePlayerList();
	UpdateSpectatorList();
	UpdateArenaWaitingToPlayList();
	UpdatePlayerDetails();
	MoveToCenterOfScreen();
	UpdateServerTimeLeft();
	AdjustForVisibleScrollbar();
	UpdateBadgePanels( m_pRedBadgePanels, m_pPlayerListRed );
	UpdateBadgePanels( m_pBlueBadgePanels, m_pPlayerListBlue );

	float flNextUpdate = 1.0f;
	if ( UseMouseMode() )
	{
		if ( ( m_pPlayerListRed && m_pPlayerListRed->GetScrollBar() && m_pPlayerListRed->GetScrollBar()->IsVisible() ) ||
			 ( m_pPlayerListBlue && m_pPlayerListBlue->GetScrollBar() && m_pPlayerListBlue->GetScrollBar()->IsVisible() ) )
		{
			// we need a much faster update rate for MouseMode() so the MedalList can update when the scrollbar is quickly moved
			// we're listening for the ScrollBarSliderMoved message but it still doesn't keep up with dragging the mouse around
			flNextUpdate = 0.02f;
		}
		else
		{
			flNextUpdate = 0.15f;
		}
	}

	m_fNextUpdateTime = gpGlobals->curtime + flNextUpdate; 
}

//-----------------------------------------------------------------------------
// Purpose: Updates information about teams
//-----------------------------------------------------------------------------
void CTFClientScoreBoardDialog::UpdateTeamInfo()
{
	// update the team sections in the scoreboard
	for ( int teamIndex = TF_TEAM_RED; teamIndex <= TF_TEAM_BLUE; teamIndex++ )
	{
		C_TFTeam *team = GetGlobalTFTeam( teamIndex );
		if ( team )
		{
			// choose dialog variables to set depending on team
			const char *pDialogVarTeamScore = "";
			const char *pDialogVarTeamPlayerCount = "";
			const char *pDialogVarTeamName = "";

			switch ( teamIndex )
			{
				case TF_TEAM_RED:
					pDialogVarTeamScore = "redteamscore";
					pDialogVarTeamPlayerCount = "redteamplayercount";
					pDialogVarTeamName = "redteamname";
					break;
				case TF_TEAM_BLUE:
					pDialogVarTeamScore = "blueteamscore";
					pDialogVarTeamPlayerCount = "blueteamplayercount";
					pDialogVarTeamName = "blueteamname";
					break;
				default:
					Assert( false );
					break;
			}

			// update # of players on each team
			wchar_t string1[1024];
			wchar_t wNumPlayers[6];
			_snwprintf( wNumPlayers, ARRAYSIZE( wNumPlayers ), L"%i", team->Get_Number_Players() );
			if ( team->Get_Number_Players() == 1 )
			{
				g_pVGuiLocalize->ConstructString_safe( string1, g_pVGuiLocalize->Find( "#TF_ScoreBoard_Player" ), 1, wNumPlayers );
			}
			else
			{
				g_pVGuiLocalize->ConstructString_safe( string1, g_pVGuiLocalize->Find( "#TF_ScoreBoard_Players" ), 1, wNumPlayers );
			}

			// set # of players for team in dialog
			SetDialogVariable( pDialogVarTeamPlayerCount, string1 );

			// set team score in dialog
			SetDialogVariable( pDialogVarTeamScore, team->Get_Score() );

			// set the team name
			SetDialogVariable( pDialogVarTeamName, team->Get_Localized_Name() );
		}
	}

	bool bMvM = TFGameRules() && TFGameRules()->IsMannVsMachineMode();
	bool bTournament = mp_tournament.GetBool() && !bMvM;
	if ( m_pRedTeamName->IsVisible() != bTournament )
	{
		m_pRedTeamName->SetVisible( bTournament );
	}

	if ( m_pBlueTeamName->IsVisible() != bTournament )
	{
		m_pBlueTeamName->SetVisible( bTournament );
	}

	bool bShowAvatars = g_TF_PR && g_TF_PR->HasPremadeParties();
	if ( bShowAvatars )
	{
		m_pRedLeaderAvatarImage->SetPlayer( GetSteamIDForPlayerIndex( g_TF_PR->GetPartyLeaderRedTeamIndex() ), k_EAvatarSize64x64 );
		m_pRedLeaderAvatarImage->SetShouldDrawFriendIcon( false );
		m_pBlueLeaderAvatarImage->SetPlayer( GetSteamIDForPlayerIndex( g_TF_PR->GetPartyLeaderBlueTeamIndex() ), k_EAvatarSize64x64 );
		m_pBlueLeaderAvatarImage->SetShouldDrawFriendIcon( false );
	}

	m_pRedLeaderAvatarImage->SetVisible( bShowAvatars );
	m_pRedLeaderAvatarBG->SetVisible( bShowAvatars );
	m_pRedTeamImage->SetVisible( !bShowAvatars && !bMvM );

	m_pBlueLeaderAvatarImage->SetVisible( bShowAvatars );
	m_pBlueLeaderAvatarBG->SetVisible( bShowAvatars );
	m_pBlueTeamImage->SetVisible( !bShowAvatars && !bMvM );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool AreEnemyTeams( int iTeam1, int iTeam2 )
{
	if ( iTeam1 == TF_TEAM_RED && iTeam2 == TF_TEAM_BLUE )
		return true;

	if ( iTeam1 == TF_TEAM_BLUE && iTeam2 == TF_TEAM_RED )
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFClientScoreBoardDialog::AdjustForVisibleScrollbar( void )
{
	if ( m_pPlayerListRed && m_pPlayerListRed->GetScrollBar() && ( m_bRedScrollBarVisible != m_pPlayerListRed->GetScrollBar()->IsVisible() ) )
	{
		m_bRedScrollBarVisible = m_pPlayerListRed->GetScrollBar()->IsVisible();
		int iScrollBarWidth = m_bRedScrollBarVisible ? m_pPlayerListRed->GetScrollBar()->GetWide() : 0;
		m_pPlayerListRed->SetColumnWidthBySection( 0, "name", m_iNameWidth + m_nExtraSpace - iScrollBarWidth );
	}

	if ( m_pPlayerListBlue && m_pPlayerListBlue->GetScrollBar() && ( m_bBlueScrollBarVisible != m_pPlayerListBlue->GetScrollBar()->IsVisible() ) )
	{
		m_bBlueScrollBarVisible = m_pPlayerListBlue->GetScrollBar()->IsVisible();
		int iScrollBarWidth = m_bBlueScrollBarVisible ? m_pPlayerListBlue->GetScrollBar()->GetWide() : 0;
		m_pPlayerListBlue->SetColumnWidthBySection( 0, "name", m_iNameWidth + m_nExtraSpace - iScrollBarWidth );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFClientScoreBoardDialog::UpdateBadgePanels( CUtlVector<CTFBadgePanel*> &pBadgePanels, vgui::SectionedListPanel *pPlayerList )
{
	int iNumPanels = 0;

	if ( tf_show_all_scoreboard_elements.GetBool() )
	{
		int parentTall = pPlayerList->GetTall();
		CTFBadgePanel *pPanel = NULL;

		for ( int i = 0; i < pPlayerList->GetItemCount(); i++ )
		{
			KeyValues* pKeyValues = pPlayerList->GetItemData( i );
			if ( !pKeyValues )
				continue;

			//int iPlayerIndex = pKeyValues->GetInt( "playerIndex" );
			{
				if ( iNumPanels >= pBadgePanels.Count() )
				{
					pPanel = new CTFBadgePanel( this, "BadgePanel" );
					pPanel->MakeReadyForUse();
					pPanel->SetVisible( true );
					pPanel->SetZPos( 9999 );
					pBadgePanels.AddToTail( pPanel );
				}
				else
				{
					pPanel = pBadgePanels[iNumPanels];
				}

				int x, y, wide, tall;
				pPlayerList->GetMaxCellBounds( i, 0, x, y, wide, tall );

				wide = m_iMedalWidth;

				if ( y + tall > parentTall )
					continue;

				if ( !pPanel->IsVisible() )
				{
					pPanel->SetVisible( true );
				}

				int xParent, yParent;
				pPlayerList->GetPos( xParent, yParent );

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

				pPanel->SetupDummyBadge( 100, false );
				iNumPanels++;
			}
		}
	}
	else
	{
		const IMatchGroupDescription *pMatchDesc = TFGameRules() ? GetMatchGroupDescription( TFGameRules()->GetCurrentMatchGroup() ) : NULL;
		if ( pMatchDesc && pPlayerList )
		{
			if ( TFGameRules()->IsMatchTypeCasual() )
			{
				int parentTall = pPlayerList->GetTall();
				CTFBadgePanel *pPanel = NULL;

				for ( int i = 0; i < pPlayerList->GetItemCount(); i++ )
				{
					KeyValues *pKeyValues = pPlayerList->GetItemData( i );
					if ( !pKeyValues )
						continue;

					int iPlayerIndex = pKeyValues->GetInt( "playerIndex" );
					const CSteamID steamID = GetSteamIDForPlayerIndex( iPlayerIndex );
					if ( steamID.IsValid() )
					{
						if ( iNumPanels >= pBadgePanels.Count() )
						{
							pPanel = new CTFBadgePanel( this, "BadgePanel" );
							pPanel->MakeReadyForUse();
							pPanel->SetVisible( true );
							pPanel->SetZPos( 9999 );
							pBadgePanels.AddToTail( pPanel );
						}
						else
						{
							pPanel = pBadgePanels[iNumPanels];
						}

						int x, y, wide, tall;
						pPlayerList->GetMaxCellBounds( i, 0, x, y, wide, tall );

						if ( y + tall > parentTall )
							continue;

						if ( !pPanel->IsVisible() )
						{
							pPanel->SetVisible( true );
						}

						int xParent, yParent;
						pPlayerList->GetPos( xParent, yParent );

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
	}

	// hide any unused images
	for ( int i = iNumPanels; i < pBadgePanels.Count(); i++ )
	{
		if ( pBadgePanels[i]->IsVisible() )
		{
			pBadgePanels[i]->SetVisible( false );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Updates the player list
//-----------------------------------------------------------------------------
void CTFClientScoreBoardDialog::UpdatePlayerList()
{
	int iSelectedPlayerIndex = GetLocalPlayerIndex();

	// Save off which player we had selected
	SectionedListPanel *pList = GetSelectedPlayerList();
	if ( pList )
	{
		int itemID = pList->GetSelectedItem();

		if ( itemID >= 0 )
		{
			KeyValues *pInfo = pList->GetItemData( itemID );
			if ( pInfo )
			{
				iSelectedPlayerIndex = pInfo->GetInt( "playerIndex" );
			}
		}
	}

	m_pPlayerListRed->ClearSelection();
	m_pPlayerListBlue->ClearSelection();
	m_pPlayerListRed->RemoveAll();
	m_pPlayerListBlue->RemoveAll();

	if ( !g_TF_PR )
		return;

	C_TFPlayer *pLocalTFPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pLocalTFPlayer )
		return;

	int localteam = pLocalTFPlayer->GetTeamNumber();
	if ( pLocalTFPlayer->m_bIsCoaching && pLocalTFPlayer->m_hStudent )
	{
		localteam = pLocalTFPlayer->m_hStudent->GetTeamNumber();
	}

	bool bMadeSelection = false;

	for ( int playerIndex = 1; playerIndex <= MAX_PLAYERS; playerIndex++ )
	{
		if ( g_PR->IsConnected( playerIndex ) || g_PR->IsValid( playerIndex ) )
		{
			SectionedListPanel *pPlayerList = NULL;
			int nTeam = g_PR->GetTeam( playerIndex );
			switch ( nTeam )
			{
			case TF_TEAM_BLUE:
				pPlayerList = m_pPlayerListBlue;
				break;
			case TF_TEAM_RED:
				pPlayerList = m_pPlayerListRed;
				break;
			}
			if ( !pPlayerList )
				continue;

			MM_PlayerConnectionState_t eConnectionState = g_TF_PR->GetPlayerConnectionState( playerIndex );
			const wchar_t *pwszFormat = NULL;
			if ( eConnectionState == MM_DISCONNECTED )
			{
				pwszFormat = g_pVGuiLocalize->Find( "#TF_MM_PlayerLostConnection" );
			}
			else if ( ( eConnectionState == MM_CONNECTING ) || ( eConnectionState == MM_LOADING ) )
			{
				pwszFormat = g_pVGuiLocalize->Find( "#TF_MM_PlayerConnecting" );
			}

			if ( pwszFormat )
			{
				KeyValues *pKV = new KeyValues( "data" );
				pKV->SetInt( "playerIndex", playerIndex );
				pKV->SetInt( "connected", 1 );

				// HOLY CHEESEBALL BUSY INDICATOR
				const wchar_t *pwszEllipses = &L"....."[4 - ( (unsigned)Plat_FloatTime() % 5U )];
				wchar_t wszLocalized[512];
				g_pVGuiLocalize->ConstructString_safe( wszLocalized, pwszFormat, 1, pwszEllipses );
				pKV->SetWString( "name", wszLocalized );

				int itemID = pPlayerList->AddItem( 0, pKV );
				pPlayerList->SetItemFgColor( itemID, ( eConnectionState == MM_DISCONNECTED ) ? Color( 208, 147, 7, 255 ) : Color( 76, 107, 34, 255 ) );
				pPlayerList->SetItemBgColor( itemID, Color( 0, 0, 0, 80 ) );
				pPlayerList->SetItemFont( itemID, m_hScoreFontSmallest );

				pKV->deleteThis();

				continue;
			}

			int iActiveDominations = g_TF_PR->GetActiveDominations( playerIndex );

			// Debug getting some bogus counts here.
			if ( iActiveDominations < 0 )
			{
				Assert( iActiveDominations >= 0 );
				iActiveDominations = 0;
			}
			else if ( iActiveDominations >= MAX_PLAYERS )
			{
				Assert( iActiveDominations < MAX_PLAYERS );
				iActiveDominations = MAX_PLAYERS - 1;
			}

			// Now limit based on the number of images we have
			if ( iActiveDominations >= SCOREBOARD_DOMINATION_ICONS )
			{
				iActiveDominations = SCOREBOARD_DOMINATION_ICONS - 1;
			}

			bool bDominating;
			if ( iActiveDominations > 0 )
			{
				bDominating = true;
			}
			else
			{
				bDominating = false;
			}

			bool bAlive = g_TF_PR->IsAlive( playerIndex );

			int iDominationIndex = ( bDominating ? ( m_iImageDom[iActiveDominations] ) : 0 );
			if ( !bAlive )
			{
				iDominationIndex = ( bDominating ? ( m_iImageDomDead[iActiveDominations] ) : 0 );
			}

			KeyValues *pKeyValues = new KeyValues( "data" );
			pKeyValues->SetInt( "playerIndex", playerIndex );
			pKeyValues->SetString( "name", g_TF_PR->GetPlayerName( playerIndex ) );
			pKeyValues->SetInt( "dominating", iDominationIndex );
			pKeyValues->SetInt( "score", g_TF_PR->GetTotalScore( playerIndex ) );
			pKeyValues->SetInt( "connected", 2 );

			C_TFPlayer *pTFPlayer = ToTFPlayer( UTIL_PlayerByIndex( playerIndex ) );

			if ( pTFPlayer && pTFPlayer->GetActiveTFWeapon() )
			{
				int nCount = g_TF_PR->GetStreak( playerIndex, CTFPlayerShared::kTFStreak_Kills );
				if ( nCount )
				{
					pKeyValues->SetInt( "killstreak_image", bAlive ? m_iImageStreak : m_iImageStreakDead );
					pKeyValues->SetInt( "killstreak", nCount );
				}
				else
				{
					pKeyValues->SetInt( "killstreak_image", 0 );
					pKeyValues->SetString( "killstreak", "" );
				}
			}

			// check for bots first, so malicious server operators can't fake a ping and stuff their server with bots that look like players
			if ( g_PR->IsFakePlayer( playerIndex ) )
			{
				if ( tf_scoreboard_ping_as_text.GetBool() )
				{
					pKeyValues->SetString( "ping", "#TF_Scoreboard_Bot" );
				}
				else
				{
					int iIndex = ( nTeam == TF_TEAM_RED ) ? PING_BOT_RED : PING_BOT_BLUE;
					pKeyValues->SetInt( "ping", bAlive ? m_iImagePing[iIndex] : m_iImagePingDead[iIndex] );
				}
			}
			else
			{
				int nPing = g_PR->GetPing( playerIndex );

 				if ( nPing < 1 )
 				{
					if ( tf_scoreboard_ping_as_text.GetBool() )
					{
						pKeyValues->SetString( "ping", "" );
					}
					else
					{
						pKeyValues->SetInt( "ping", 0 );
					}
 				}
 				else
				{
					if ( tf_scoreboard_ping_as_text.GetBool() )
					{
						pKeyValues->SetInt( "ping", nPing );
					}
					else
					{
						int iIndex = PING_VERY_HIGH;

						if ( nPing < 125 )
						{
							iIndex = PING_LOW;
						}
						else if ( nPing < 200 )
						{
							iIndex = PING_MED;
						}
						else if ( nPing < 275 )
						{
							iIndex = PING_HIGH;
						}
							
						pKeyValues->SetInt( "ping", bAlive ? m_iImagePing[iIndex] : m_iImagePingDead[iIndex] );
					}
				}
			}

			if ( TFGameRules() && TFGameRules()->IsHolidayActive( kHoliday_Halloween ) && TFGameRules()->ArePlayersInHell() )
			{
				bAlive &= pTFPlayer && !pTFPlayer->m_Shared.InCond( TF_COND_HALLOWEEN_GHOST_MODE );
			}

			// are they a Spy that's feigning death? mark them as dead in the scoreboard...
			if ( g_TF_PR->GetPlayerClass( playerIndex ) == TF_CLASS_SPY )
			{
				if ( pTFPlayer && pTFPlayer->m_Shared.InCond( TF_COND_FEIGN_DEATH ) )
				{
					bAlive = false;
				}
			}

			// can only see class information if we're on the same team
			if ( !AreEnemyTeams( g_PR->GetTeam( playerIndex ), localteam ) && !( localteam == TEAM_UNASSIGNED ) )
			{
				// class name
				if ( g_PR->IsConnected( playerIndex ) )
				{
					int iClass = g_TF_PR->GetPlayerClass( playerIndex );
					if ( GetLocalPlayerIndex() == playerIndex && !bAlive )
					{
						// If this is local player and he is dead, show desired class (which he will spawn as) rather than current class.
						int iDesiredClass = pLocalTFPlayer->m_Shared.GetDesiredPlayerClassIndex();
						// use desired class unless it's random -- if random, his future class is not decided until moment of spawn
						if ( TF_CLASS_RANDOM != iDesiredClass )
						{
							iClass = iDesiredClass;
						}
					}
					else
					{
						// for non-local players, show the current class
						iClass = g_TF_PR->GetPlayerClass( playerIndex );
					}

					if ( iClass >= TF_FIRST_NORMAL_CLASS && iClass <= TF_LAST_NORMAL_CLASS )
					{
						if ( bAlive )
						{
							pKeyValues->SetInt( "class", tf_scoreboard_alt_class_icons.GetBool() ? m_iImageClassAlt[iClass] : m_iImageClass[iClass] );
						}
						else
						{
							pKeyValues->SetInt( "class", tf_scoreboard_alt_class_icons.GetBool() ? m_iImageClassAlt[iClass + 9] : m_iImageClass[iClass + 9] ); // +9 is to jump ahead to the darker dead icons
						}
					}
					else
					{
						pKeyValues->SetInt( "class", 0 );
					}
				}
			}
			else
			{
				if ( pTFPlayer && pTFPlayer->m_Shared.IsPlayerDominated( pLocalTFPlayer->entindex() ) )
				{
					// if local player is dominated by this player, show a nemesis icon
					pKeyValues->SetInt( "nemesis", bAlive ? m_iImageNemesis : m_iImageNemesisDead );
				}
				else if ( pLocalTFPlayer->m_Shared.IsPlayerDominated( playerIndex ) )
				{
					// if this player is dominated by the local player, show the domination icon
					pKeyValues->SetInt( "nemesis", bAlive ? m_iImageDominated : m_iImageDominatedDead );
				}
			}

			if ( tf_show_all_scoreboard_elements.GetBool() )
			{
				pKeyValues->SetString( "name", g_TF_PR->GetPlayerName( playerIndex ) );
				pKeyValues->SetInt( "dominating", m_iImageDom[1] );
				pKeyValues->SetInt( "score", 9999 );
				pKeyValues->SetInt( "connected", 2 );
				pKeyValues->SetInt( "killstreak_image", bAlive ? m_iImageStreak : m_iImageStreakDead );
				pKeyValues->SetInt( "killstreak", 100 );
				pKeyValues->SetInt( "nemesis", bAlive ? m_iImageNemesis : m_iImageNemesisDead );
			}


			UpdatePlayerAvatar( playerIndex, pKeyValues );

			// the medal column is just a place holder for the images that are displayed later
			pKeyValues->SetInt( "medal", 0 );

			int itemID = pPlayerList->AddItem( 0, pKeyValues );
			Color clr = g_PR->GetTeamColor( nTeam );

			// change color based off alive or dead status. Also slightly different if its local player since the name is highlighted.
			if ( !bAlive )
			{
				if ( nTeam == TF_TEAM_RED )
				{
					if ( GetLocalPlayerIndex() != playerIndex )
					{
						clr = Color( 135, 83, 83, 255 );
					}
					else
					{
						clr = Color( 182, 75, 75, 255 );
					}
				}
				else
				{
					if ( GetLocalPlayerIndex() != playerIndex )
					{
						clr = Color( 81, 97, 129, 255 );
					}
					else
					{
						clr = Color( 123, 153, 187, 255 );
					}
				}
			}

			pPlayerList->SetItemFgColor( itemID, clr );
			pPlayerList->SetItemBgColor( itemID, Color( 0, 0, 0, 80 ) );
			pPlayerList->SetItemFont( itemID, m_hScoreFontDefault );

			if ( iSelectedPlayerIndex == playerIndex )
			{
				bMadeSelection = true;
				pPlayerList->SetSelectedItem( itemID );
			}

			pKeyValues->deleteThis();
		}
		else
		{
			MM_PlayerConnectionState_t eConnectionState = g_TF_PR->GetPlayerConnectionState( playerIndex );
			if ( eConnectionState == MM_WAITING_FOR_PLAYER )
			{
				SectionedListPanel *pPlayerList = NULL;
				int nTeam = g_PR->GetTeam( playerIndex );
				switch ( nTeam )
				{
				case TF_TEAM_BLUE:
					pPlayerList = m_pPlayerListBlue;
					break;
				case TF_TEAM_RED:
					pPlayerList = m_pPlayerListRed;
					break;
				}
				if ( pPlayerList )
				{
					const wchar_t *pwszFormat = g_pVGuiLocalize->Find( "#TF_MM_LookingForPlayer" );
					if ( pwszFormat )
					{
						KeyValues *pKV = new KeyValues( "data" );
						pKV->SetInt( "playerIndex", playerIndex );
						pKV->SetInt( "connected", 0 );

						// HOLY CHEESEBALL BUSY INDICATOR
						const wchar_t *pwszEllipses = &L"....."[4 - ( (unsigned)Plat_FloatTime() % 5U )];
						wchar_t wszLocalized[512];
						g_pVGuiLocalize->ConstructString_safe( wszLocalized, pwszFormat, 1, pwszEllipses );
						pKV->SetWString( "name", wszLocalized );

						int itemID = pPlayerList->AddItem( 0, pKV );
						pPlayerList->SetItemFgColor( itemID, Color( 120, 120, 120, 255 ) );
						pPlayerList->SetItemBgColor( itemID, Color( 0, 0, 0, 80 ) );
						pPlayerList->SetItemFont( itemID, m_hScoreFontSmallest );

						pKV->deleteThis();
					}
				}
			}
		}
	}

	// If we're on spectator, find a default selection
#ifdef _X360
	if ( !bMadeSelection )
	{
		if ( m_pPlayerListBlue->GetItemCount() > 0 )
		{
			m_pPlayerListBlue->SetSelectedItem( 0 );
		}
		else if ( m_pPlayerListRed->GetItemCount() > 0 )
		{
			m_pPlayerListRed->SetSelectedItem( 0 );
		}
	}
#endif

	// force the lists to PerformLayout() now so we can update our medal images after we return
 	m_pPlayerListRed->InvalidateLayout( true );
 	m_pPlayerListBlue->InvalidateLayout( true );
}

//-----------------------------------------------------------------------------
// Purpose: Updates the spectator list
//-----------------------------------------------------------------------------
void CTFClientScoreBoardDialog::UpdateSpectatorList()
{
	char szCoachList[512] = "" ;
	int nCoaches = 0;
	char szSpectatorList[512] = "" ;
	int nSpectators = 0;
	for( int playerIndex = 1 ; playerIndex <= MAX_PLAYERS; playerIndex++ )
	{
		if ( ShouldShowAsSpectator( playerIndex ) )
		{
			C_TFPlayer *pPlayer = (C_TFPlayer*)UTIL_PlayerByIndex( playerIndex );
			if ( pPlayer && pPlayer->m_bIsCoaching && pPlayer->m_hStudent )
			{
				if ( nCoaches > 0 )
				{
					Q_strncat( szCoachList, ", ", ARRAYSIZE( szCoachList ) );
				}

				Q_strncat( szCoachList, g_PR->GetPlayerName( playerIndex ), ARRAYSIZE( szCoachList ) );
				nCoaches++;
			}
			else
			{
				if ( nSpectators > 0 )
				{
					Q_strncat( szSpectatorList, ", ", ARRAYSIZE( szSpectatorList ) );
				}

				Q_strncat( szSpectatorList, g_PR->GetPlayerName( playerIndex ), ARRAYSIZE( szSpectatorList ) );
				nSpectators++;
			}
		}
	}

	wchar_t wzText[512] = L"";
	if ( nCoaches > 0 )
	{
		const char *pchFormat = ( 1 == nCoaches ? "#ScoreBoard_Coach" : "#ScoreBoard_Coaches" );

		wchar_t wzCount[16];
		wchar_t wzList[1024];
		_snwprintf( wzCount, ARRAYSIZE( wzCount ), L"%i", nCoaches );
		g_pVGuiLocalize->ConvertANSIToUnicode( szCoachList, wzList, sizeof( wzList ) );
		g_pVGuiLocalize->ConstructString_safe( wzText, g_pVGuiLocalize->Find( pchFormat), 2, wzCount, wzList );
	}
	if ( nSpectators > 0 )
	{
		const char *pchFormat = ( 1 == nSpectators ? "#ScoreBoard_Spectator" : "#ScoreBoard_Spectators" );

		wchar_t wzSpectatorText[512] = L"";
		wchar_t wzSpectatorCount[16];
		wchar_t wzSpectatorList[1024];
		_snwprintf( wzSpectatorCount, ARRAYSIZE( wzSpectatorCount ), L"%i", nSpectators );
		g_pVGuiLocalize->ConvertANSIToUnicode( szSpectatorList, wzSpectatorList, sizeof( wzSpectatorList ) );
		g_pVGuiLocalize->ConstructString_safe( wzSpectatorText, g_pVGuiLocalize->Find( pchFormat), 2, wzSpectatorCount, wzSpectatorList );
		if ( nCoaches > 0 )
		{
			V_wcscat_safe( wzText, L". " );
		}
		V_wcscat_safe( wzText, wzSpectatorText );
	}

	SetDialogVariable( "spectators", wzText );
}

//-----------------------------------------------------------------------------
// Purpose: Returns whether the specified player index is an arena spectator
//-----------------------------------------------------------------------------
bool CTFClientScoreBoardDialog::ShouldShowAsArenaWaitingToPlay( int iPlayerIndex )
{
	if ( !g_TF_PR )
		return false;

	// see if player is connected
	if ( g_TF_PR->IsConnected( iPlayerIndex ) ) 
	{
		if ( g_TF_PR->IsArenaSpectator( iPlayerIndex ) == true )
			return false;

		if ( g_TF_PR->GetPlayerClass( iPlayerIndex ) == TF_CLASS_UNDEFINED )
			return false;

		if ( g_TF_PR->GetTeam( iPlayerIndex ) > LAST_SHARED_TEAM )
			return false;

		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Updates the spectator list
//-----------------------------------------------------------------------------
void CTFClientScoreBoardDialog::UpdateArenaWaitingToPlayList()
{
	char szSpectatorList[512] = "" ;
	wchar_t wzSpectators[512] = L"";

	if ( TFGameRules() && TFGameRules()->IsInArenaMode() == true )
	{
		int nSpectators = 0;
		for( int playerIndex = 1 ; playerIndex <= MAX_PLAYERS; playerIndex++ )
		{
			if ( ShouldShowAsArenaWaitingToPlay( playerIndex ) )
			{
				if ( nSpectators > 0 )
				{
					Q_strncat( szSpectatorList, ", ", ARRAYSIZE( szSpectatorList ) );
				}

				Q_strncat( szSpectatorList, g_PR->GetPlayerName( playerIndex ), ARRAYSIZE( szSpectatorList ) );
				nSpectators++;
			}
		}
		
		if ( nSpectators > 0 )
		{
			const char *pchFormat = ( 1 == nSpectators ? "#TF_Arena_ScoreBoard_Spectator" : "#TF_Arena_ScoreBoard_Spectators" );

			wchar_t wzSpectatorCount[16];
			wchar_t wzSpectatorList[1024];
			_snwprintf( wzSpectatorCount, ARRAYSIZE( wzSpectatorCount ), L"%i", nSpectators );
			g_pVGuiLocalize->ConvertANSIToUnicode( szSpectatorList, wzSpectatorList, sizeof( wzSpectatorList ) );
			g_pVGuiLocalize->ConstructString_safe( wzSpectators, g_pVGuiLocalize->Find( pchFormat), 2, wzSpectatorCount, wzSpectatorList );
		}
	}

	SetDialogVariable( "waitingtoplay", wzSpectators );
}

static void PopulateDuelPanel( CTFClientScoreBoardDialog::duel_panel_t &duelPanel, C_TFPlayer *pPlayer, int unScore )
{
	CSteamID steamID;
	if ( duelPanel.m_pAvatar && pPlayer->GetSteamID( &steamID ) )
	{
		duelPanel.m_pAvatar->SetVisible( true );
		duelPanel.m_pAvatar->SetShouldDrawFriendIcon( false );
		duelPanel.m_pAvatar->SetPlayer( steamID, k_EAvatarSize64x64 );
	}

	if ( duelPanel.m_pPlayerNameLabel && g_TF_PR != NULL )
	{
		duelPanel.m_pPanel->SetDialogVariable( "playername", g_TF_PR->GetPlayerName( pPlayer->entindex() ) );
		Color clr = g_PR->GetTeamColor( g_PR->GetTeam( pPlayer->entindex() ) );
		duelPanel.m_pPlayerNameLabel->SetFgColor( clr );
	}
	duelPanel.m_pPanel->SetDialogVariable( "score", unScore );
}

//-----------------------------------------------------------------------------
// Purpose: Updates details about a player
//-----------------------------------------------------------------------------
void CTFClientScoreBoardDialog::UpdatePlayerDetails()
{
	ClearPlayerDetails();

	// by default, hide these, especially for source tv
	m_pLocalPlayerStatsPanel->SetVisible( false );
	m_pLocalPlayerDuelStatsPanel->SetVisible( false );

	if ( !g_TF_PR )
		return;
	
	// Default is local player
	C_TFPlayer *pSelectedPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pSelectedPlayer )
		return;

	int playerIndex = pSelectedPlayer->entindex();
	
	// Change to selected player when using mouse mode
	if ( UseMouseMode() )
	{
		SectionedListPanel *pList = GetSelectedPlayerList();
		if ( pList )
		{
			int iSelectedItem = pList->GetSelectedItem();
			if ( iSelectedItem >= 0 )
			{
				KeyValues *pIssueKeyValues = pList->GetItemData( iSelectedItem );
				if ( pIssueKeyValues )
				{
					playerIndex = pIssueKeyValues->GetInt( "playerIndex", 0 );
					pSelectedPlayer = ToTFPlayer( UTIL_PlayerByIndex( playerIndex ) );
					if ( !pSelectedPlayer || !playerIndex )
						return;
				}
			}
		}
	}

	// Make sure the selected player is still connected. 
	if ( !g_TF_PR->IsConnected( playerIndex ) ) 
		return;

#if defined( REPLAY_ENABLED )
	if ( engine->IsHLTV() || g_pEngineClientReplay->IsPlayingReplayDemo() )
#else
	if ( engine->IsHLTV() )
#endif
	{
		SetDialogVariable( "playername", g_TF_PR->GetPlayerName( playerIndex ) );
		return;
	}

	uint32 unMyScore, unOpponentScore;
	C_TFPlayer *pDuelingPartner = NULL;
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( pSelectedPlayer == pLocalPlayer && DuelMiniGame_GetStats( &pDuelingPartner, unMyScore, unOpponentScore ) )
	{
		PopulateDuelPanel( m_duelPanelLocalPlayer, pLocalPlayer, unMyScore );
		PopulateDuelPanel( m_duelPanelOpponent, pDuelingPartner, unOpponentScore );

		if ( m_pLocalPlayerStatsPanel->IsVisible() == true )
		{
			m_pLocalPlayerStatsPanel->SetVisible( false );
		}
		if ( m_pLocalPlayerDuelStatsPanel->IsVisible() == false )
		{
			m_pLocalPlayerDuelStatsPanel->SetVisible( true );
		}
	}
	else
	{
		if ( m_pLocalPlayerStatsPanel->IsVisible() == false )
		{
			m_pLocalPlayerStatsPanel->SetVisible( true );
		}
		if ( m_pLocalPlayerDuelStatsPanel->IsVisible() == true )
		{
			m_pLocalPlayerDuelStatsPanel->SetVisible( false );
		}

		Color cGreen = Color( 0, 255, 0, 255 );
		Color cWhite = Color( 255, 255, 255, 255 );

		m_pLocalPlayerStatsPanel->SetDialogVariable( "kills", g_TF_PR->GetPlayerScore( playerIndex ) );
		if ( m_pKillsLabel )
		{
			m_pKillsLabel->SetFgColor( g_TF_PR->GetPlayerScore( playerIndex ) ? cGreen : cWhite );
		}
		m_pLocalPlayerStatsPanel->SetDialogVariable( "deaths", g_TF_PR->GetDeaths( playerIndex ) );
		if ( m_pDeathsLabel )
		{
			m_pDeathsLabel->SetFgColor( g_TF_PR->GetDeaths( playerIndex ) ? cGreen : cWhite );
		}
		m_pLocalPlayerStatsPanel->SetDialogVariable( "assists", pSelectedPlayer->m_Shared.GetKillAssists( playerIndex ) );
		if ( m_pAssistLabel )
		{
			m_pAssistLabel->SetFgColor( pSelectedPlayer->m_Shared.GetKillAssists( playerIndex ) ? cGreen : cWhite );
		}
		m_pLocalPlayerStatsPanel->SetDialogVariable( "destruction", pSelectedPlayer->m_Shared.GetBuildingsDestroyed( playerIndex ) );
		if ( m_pDestructionLabel )
		{
			m_pDestructionLabel->SetFgColor( pSelectedPlayer->m_Shared.GetBuildingsDestroyed( playerIndex ) ? cGreen : cWhite );
		}
		m_pLocalPlayerStatsPanel->SetDialogVariable( "captures", pSelectedPlayer->m_Shared.GetCaptures( playerIndex ) );
		if ( m_pCapturesLabel )
		{
			m_pCapturesLabel->SetFgColor( pSelectedPlayer->m_Shared.GetCaptures( playerIndex ) ? cGreen : cWhite );
		}
		m_pLocalPlayerStatsPanel->SetDialogVariable( "defenses", pSelectedPlayer->m_Shared.GetDefenses( playerIndex ) );
		if ( m_pDefensesLabel )
		{
			m_pDefensesLabel->SetFgColor( pSelectedPlayer->m_Shared.GetDefenses( playerIndex ) ? cGreen : cWhite );
		}
		m_pLocalPlayerStatsPanel->SetDialogVariable( "dominations", pSelectedPlayer->m_Shared.GetDominations( playerIndex ) );
		if ( m_pDominationsLabel )
		{
			m_pDominationsLabel->SetFgColor( pSelectedPlayer->m_Shared.GetDominations( playerIndex ) ? cGreen : cWhite );
		}
		m_pLocalPlayerStatsPanel->SetDialogVariable( "revenge", pSelectedPlayer->m_Shared.GetRevenge( playerIndex ) );
		if ( m_pRevengeLabel )
		{
			m_pRevengeLabel->SetFgColor( pSelectedPlayer->m_Shared.GetRevenge( playerIndex ) ? cGreen : cWhite );
		}
		m_pLocalPlayerStatsPanel->SetDialogVariable( "healing", pSelectedPlayer->m_Shared.GetHealPoints( playerIndex ) );
		if ( m_pHealingLabel )
		{
			m_pHealingLabel->SetFgColor( pSelectedPlayer->m_Shared.GetHealPoints( playerIndex ) ? cGreen : cWhite );
		}
		m_pLocalPlayerStatsPanel->SetDialogVariable( "invulns", pSelectedPlayer->m_Shared.GetInvulns( playerIndex ) );
		if ( m_pInvulnsLabel )
		{
			m_pInvulnsLabel->SetFgColor( pSelectedPlayer->m_Shared.GetInvulns( playerIndex ) ? cGreen : cWhite );
		}
		m_pLocalPlayerStatsPanel->SetDialogVariable( "teleports", pSelectedPlayer->m_Shared.GetTeleports( playerIndex ) );
		if ( m_pTeleportsLabel )
		{
			m_pTeleportsLabel->SetFgColor( pSelectedPlayer->m_Shared.GetTeleports( playerIndex ) ? cGreen : cWhite );
		}
		m_pLocalPlayerStatsPanel->SetDialogVariable( "headshots", pSelectedPlayer->m_Shared.GetHeadshots( playerIndex ) );
		if ( m_pHeadshotsLabel )
		{
			m_pHeadshotsLabel->SetFgColor( pSelectedPlayer->m_Shared.GetHeadshots( playerIndex ) ? cGreen : cWhite );
		}
		m_pLocalPlayerStatsPanel->SetDialogVariable( "backstabs", pSelectedPlayer->m_Shared.GetBackstabs( playerIndex ) );
		if ( m_pBackstabsLabel )
		{
			m_pBackstabsLabel->SetFgColor( pSelectedPlayer->m_Shared.GetBackstabs( playerIndex ) ? cGreen : cWhite );
		}
		m_pLocalPlayerStatsPanel->SetDialogVariable( "bonus", pSelectedPlayer->m_Shared.GetBonusPoints( playerIndex ) );
		if ( m_pBonusLabel )
		{
			m_pBonusLabel->SetFgColor( pSelectedPlayer->m_Shared.GetBonusPoints( playerIndex ) ? cGreen : cWhite );
		}

		int nSupport = TFGameRules() ? TFGameRules()->CalcPlayerSupportScore( NULL, playerIndex ) : 0;
		m_pLocalPlayerStatsPanel->SetDialogVariable( "support", nSupport );
		if ( m_pSupportLabel )
		{
			m_pSupportLabel->SetFgColor( nSupport ? cGreen : cWhite );
		}
		m_pLocalPlayerStatsPanel->SetDialogVariable( "damage", g_TF_PR->GetDamage( playerIndex ) );
		if ( m_pDamageLabel )
		{
			m_pDamageLabel->SetFgColor( g_TF_PR->GetDamage( playerIndex ) ? cGreen : cWhite );
		}
	}		

	SetDialogVariable( "playername", g_TF_PR->GetPlayerName( playerIndex ) );

	Color clr = g_PR->GetTeamColor( g_PR->GetTeam( playerIndex ) );
	m_pLabelPlayerName->SetFgColor( clr );
	m_pImagePanelHorizLine->SetFillColor( clr );

	// update our image if our selected player or mode of display has changed
	if ( ( m_hSelectedPlayer != pSelectedPlayer ) || ( m_bUsePlayerModel != cl_hud_playerclass_use_playermodel.GetBool() ) )
	{
		m_hSelectedPlayer = pSelectedPlayer;
		m_bUsePlayerModel = cl_hud_playerclass_use_playermodel.GetBool();

		int iClass = pSelectedPlayer->m_Shared.GetDesiredPlayerClassIndex();
		int iTeam = pSelectedPlayer->GetTeamNumber();
		if ( ( pLocalPlayer->InSameTeam( pSelectedPlayer ) || pLocalPlayer->GetTeamNumber() < FIRST_GAME_TEAM ) && 
			 iTeam >= FIRST_GAME_TEAM && iClass >= TF_FIRST_NORMAL_CLASS && iClass <= TF_LAST_NORMAL_CLASS )
		{
			if ( cl_hud_playerclass_use_playermodel.GetBool() )
			{
				if ( !m_pPlayerModelPanel->IsVisible() )
				{
					m_pPlayerModelPanel->SetVisible( true );
				}

				if ( m_pClassImage->IsVisible() )
				{
					m_pClassImage->SetVisible( false );
				}

				m_nPlayerModelPanelIndex = pSelectedPlayer->entindex();
				UpdatePlayerModel();
			}
			else
			{
				if ( m_pPlayerModelPanel->IsVisible() )
				{
					m_pPlayerModelPanel->SetVisible( false );
				}

				if ( !m_pClassImage->IsVisible() )
				{
					m_pClassImage->SetVisible( true );
				}

				m_pClassImage->SetClass( iTeam, iClass, 0 );
			}
		}
		else
		{
			if ( m_pPlayerModelPanel->IsVisible() )
			{
				m_pPlayerModelPanel->SetVisible( false );
			}
			if ( m_pClassImage->IsVisible() )
			{
				m_pClassImage->SetVisible( false );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Updates Server Time Left
//-----------------------------------------------------------------------------
void CTFClientScoreBoardDialog::UpdateServerTimeLeft()
{
	wchar_t wzServerTimeHrsLeft[128];
	wchar_t wzServerTimeMinLeft[128];
	wchar_t wzServerTimeSecLeft[128];
	wchar_t wzServerTimeLeft[128];
	
	int iTimeLeft = 0;
	int iHours = 0;
	int iMinutes = 0;
	int iSeconds = 0;
	int iServerTimeLimit = mp_timelimit.GetInt() * 60;

	if ( iServerTimeLimit == 0 )
	{ 
		g_pVGuiLocalize->ConstructString_safe( wzServerTimeLeft, g_pVGuiLocalize->Find( "#Scoreboard_NoTimeLimit" ), 0 );
		SetDialogVariable( "servertimeleft", wzServerTimeLeft );

		g_pVGuiLocalize->ConstructString_safe( wzServerTimeLeft, g_pVGuiLocalize->Find( "#Scoreboard_NoTimeLimitNew" ), 0 );
		SetDialogVariable( "servertime", wzServerTimeLeft );

		if ( m_pServerTimeLeftValue && m_pServerTimeLeftValue->IsVisible() &&  ( m_pFontTimeLeftString != vgui::INVALID_FONT ) )
		{
			m_pServerTimeLeftValue->SetFont( m_pFontTimeLeftString );
			m_pServerTimeLeftValue->InvalidateLayout();
		}

		return;
	}

	iTimeLeft = TFGameRules() ? TFGameRules()->GetTimeLeft() : 0;

	if ( iTimeLeft < 0 )
	{
		iTimeLeft = 0;
	}
	if ( iTimeLeft == 0 )
	{
		g_pVGuiLocalize->ConstructString_safe( wzServerTimeLeft, g_pVGuiLocalize->Find( "#Scoreboard_ChangeOnRoundEnd" ), 0 );
		SetDialogVariable( "servertimeleft", wzServerTimeLeft );

		g_pVGuiLocalize->ConstructString_safe( wzServerTimeLeft, g_pVGuiLocalize->Find( "#Scoreboard_ChangeOnRoundEndNew" ), 0 );
		SetDialogVariable( "servertime", wzServerTimeLeft );

		if ( m_pServerTimeLeftValue && m_pServerTimeLeftValue->IsVisible() && ( m_pFontTimeLeftString != vgui::INVALID_FONT ) )
		{
			m_pServerTimeLeftValue->SetFont( m_pFontTimeLeftString );
			m_pServerTimeLeftValue->InvalidateLayout();
		}

		return;
	}

	iHours = iTimeLeft / 3600;
	iMinutes = ( iTimeLeft % 3600 ) / 60;
	iSeconds = ( iTimeLeft % 60 );

	_snwprintf( wzServerTimeHrsLeft, ARRAYSIZE( wzServerTimeHrsLeft ), L"%i", iHours );
	_snwprintf( wzServerTimeMinLeft, ARRAYSIZE( wzServerTimeMinLeft ), L"%02i", iMinutes );
	_snwprintf( wzServerTimeSecLeft, ARRAYSIZE( wzServerTimeSecLeft ), L"%02i", iSeconds );

	if ( m_pServerTimeLeftValue && m_pServerTimeLeftValue->IsVisible() && ( m_pFontTimeLeftNumbers != vgui::INVALID_FONT ) )
	{
		m_pServerTimeLeftValue->SetFont( m_pFontTimeLeftNumbers );
		m_pServerTimeLeftValue->InvalidateLayout();
	}

	if ( iHours == 0 )
	{
		g_pVGuiLocalize->ConstructString_safe( wzServerTimeLeft, g_pVGuiLocalize->Find( "#Scoreboard_TimeLeftNoHours" ), 2, wzServerTimeMinLeft, wzServerTimeSecLeft );
		SetDialogVariable( "servertimeleft", wzServerTimeLeft );

		g_pVGuiLocalize->ConstructString_safe( wzServerTimeLeft, g_pVGuiLocalize->Find( "#Scoreboard_TimeLeftNoHoursNew" ), 2, wzServerTimeMinLeft, wzServerTimeSecLeft );
		SetDialogVariable( "servertime", wzServerTimeLeft );

		return;
	}
	
	g_pVGuiLocalize->ConstructString_safe( wzServerTimeLeft, g_pVGuiLocalize->Find( "#Scoreboard_TimeLeft" ), 3, wzServerTimeHrsLeft, wzServerTimeMinLeft, wzServerTimeSecLeft );
	SetDialogVariable( "servertimeleft", wzServerTimeLeft );

	g_pVGuiLocalize->ConstructString_safe( wzServerTimeLeft, g_pVGuiLocalize->Find( "#Scoreboard_TimeLeftNew" ), 3, wzServerTimeHrsLeft, wzServerTimeMinLeft, wzServerTimeSecLeft );
	SetDialogVariable( "servertime", wzServerTimeLeft );
}

//-----------------------------------------------------------------------------
// Purpose: Clears score details
//-----------------------------------------------------------------------------
void CTFClientScoreBoardDialog::ClearPlayerDetails()
{
	// HLTV has no game stats
#if defined( REPLAY_ENABLED )
	bool bVisible = !engine->IsHLTV() && !g_pEngineClientReplay->IsPlayingReplayDemo();
#else
	bool bVisible = !engine->IsHLTV();
#endif

	SetDialogVariable( "kills", "" ); 
	SetControlVisible( "KillsLabel", bVisible );

	SetDialogVariable( "deaths", "" );
	SetControlVisible( "DeathsLabel", bVisible );

	SetDialogVariable( "captures", "" );
	SetControlVisible( "CapturesLabel", bVisible );

	SetDialogVariable( "defenses", "" );
	SetControlVisible( "DefensesLabel", bVisible );

	SetDialogVariable( "dominations", "" );
	SetControlVisible( "DominationLabel", bVisible );

	SetDialogVariable( "revenge", "" );
	SetControlVisible( "RevengeLabel", bVisible );

	SetDialogVariable( "assists", "" );
	SetControlVisible( "AssistsLabel", bVisible );

	SetDialogVariable( "destruction", "" );
	SetControlVisible( "DestructionLabel", bVisible );

	SetDialogVariable( "healing", "" );
	SetControlVisible( "HealingLabel", bVisible );

	SetDialogVariable( "invulns", "" );
	SetControlVisible( "InvulnLabel", bVisible );

	SetDialogVariable( "teleports", "" );
	SetControlVisible( "TeleportsLabel", bVisible );

	SetDialogVariable( "headshots", "" );
	SetControlVisible( "HeadshotsLabel", bVisible );

	SetDialogVariable( "backstabs", "" );
	SetControlVisible( "BackstabsLabel", bVisible );

	SetDialogVariable( "bonus", "" );
	SetControlVisible( "BonusLabel", bVisible );

	SetDialogVariable( "support", "" );
	SetControlVisible( "SupportLabel", bVisible );

	SetDialogVariable( "damage", "" );
	SetControlVisible( "DamageLabel", bVisible );

	SetDialogVariable( "playername", "" );

//	SetDialogVariable( "playerscore", "" );

	
}

//-----------------------------------------------------------------------------
// Purpose: Used for sorting players
//-----------------------------------------------------------------------------
bool CTFClientScoreBoardDialog::TFPlayerSortFunc( vgui::SectionedListPanel *list, int itemID1, int itemID2 )
{
	KeyValues *it1 = list->GetItemData(itemID1);
	KeyValues *it2 = list->GetItemData(itemID2);
	Assert(it1 && it2);

	// first compare score
	int v1 = it1->GetInt("score");
	int v2 = it2->GetInt("score");
	if (v1 > v2)
		return true;
	else if (v1 < v2)
		return false;

	// sort by connected status next ( 0 is looking for player, 1 is disconnected/connecting/loading, 2 is fully connected )
	int connected1 = it1->GetInt( "connected" );
	int connected2 = it2->GetInt( "connected" );
	if ( ( connected1 != 2 ) || ( connected2 != 2 ) )
		return ( connected1 > connected2 );

	// if score is the same, use player index to get deterministic sort
	int iPlayerIndex1 = it1->GetInt( "playerIndex" );
	int iPlayerIndex2 = it2->GetInt( "playerIndex" );
	return ( iPlayerIndex1 > iPlayerIndex2 );
}

//-----------------------------------------------------------------------------
// Purpose: Returns a localized string of form "1 point", "2 points", etc for specified # of points
//-----------------------------------------------------------------------------
const wchar_t *GetPointsString( int iPoints )
{
	wchar_t wzScoreVal[128];
	static wchar_t wzScore[128];
	_snwprintf( wzScoreVal, ARRAYSIZE( wzScoreVal ), L"%i", iPoints );
	if ( 1 == iPoints ) 
	{
		g_pVGuiLocalize->ConstructString_safe( wzScore, g_pVGuiLocalize->Find( "#TF_ScoreBoard_Point" ), 1, wzScoreVal );
	}
	else
	{
		g_pVGuiLocalize->ConstructString_safe( wzScore, g_pVGuiLocalize->Find( "#TF_ScoreBoard_Points" ), 1, wzScoreVal );
	}
	return wzScore;
}

//-----------------------------------------------------------------------------
// Purpose: Returns whether the specified player index is a spectator
//-----------------------------------------------------------------------------
bool CTFClientScoreBoardDialog::ShouldShowAsSpectator( int iPlayerIndex )
{
	if ( !g_TF_PR )
		return false;

	// see if player is connected
	if ( g_TF_PR->IsConnected( iPlayerIndex ) ) 
	{
		// either spectating or unassigned team should show in spectator list
		int iTeam = g_TF_PR->GetTeam( iPlayerIndex );

		if ( TFGameRules() )
		{
			if ( TFGameRules()->IsInArenaMode() )
			{
				if ( g_TF_PR->IsArenaSpectator( iPlayerIndex ) )
					return true;

				return false;
			}
			else if ( TFGameRules()->IsMannVsMachineMode() )
			{
				if ( g_TF_PR->IsFakePlayer( iPlayerIndex ) )
					return false;
			}
		}

		if ( TEAM_SPECTATOR == iTeam || TEAM_UNASSIGNED == iTeam )
			return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Event handler
//-----------------------------------------------------------------------------
void CTFClientScoreBoardDialog::FireGameEvent( IGameEvent *event )
{
	const char *type = event->GetName();

	if ( FStrEq( type, "server_spawn" ) )
	{		
		// set server name in scoreboard
		const char *hostname = event->GetString( "hostname" );
		wchar_t wzHostName[256];
		wchar_t wzServerLabel[256];
		g_pVGuiLocalize->ConvertANSIToUnicode( hostname, wzHostName, sizeof( wzHostName ) );
		g_pVGuiLocalize->ConstructString_safe( wzServerLabel, g_pVGuiLocalize->Find( "#Scoreboard_Server" ), 1, wzHostName );
		SetDialogVariable( "server", wzServerLabel );
		const char *pMapName = event->GetString( "mapname" );
		SetDialogVariable( "mapname", GetMapDisplayName( pMapName ) );
		// m_pLocalPlayerStatsPanel->SetDialogVariable( "gametype", g_pVGuiLocalize->Find( GetMapType( pMapName ) ) );
	}

	if ( IsVisible() )
	{
		Update();
	}
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
SectionedListPanel *CTFClientScoreBoardDialog::GetSelectedPlayerList( void )
{
	SectionedListPanel *pList = NULL;

	// navigation
	if ( m_pPlayerListBlue->GetSelectedItem() >= 0 )
	{
		pList = m_pPlayerListBlue;
	}
	else if ( m_pPlayerListRed->GetSelectedItem() >= 0 )
	{
		pList = m_pPlayerListRed;
	}

	return pList;
}

//-----------------------------------------------------------------------------
// Purpose: Event handler
//-----------------------------------------------------------------------------
int	CTFClientScoreBoardDialog::HudElementKeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding )
{
	if ( !IsVisible() )
		return 1;

	if ( !down )
	{
		return 1;
	}

	if ( tf_scoreboard_mouse_mode.GetInt() == 2 )
	{
		if ( !m_bMouseActivated && ( keynum == MOUSE_LEFT || keynum == MOUSE_RIGHT ) )
		{
			m_bMouseActivated = true;
			InitializeInputScheme();
			return 0;
		}
	}

#if defined ( _X360 )
	SectionedListPanel *pList = GetSelectedPlayerList();

	switch( keynum )
	{
	case KEY_XBUTTON_UP:
		{
			if ( pList )
			{
				pList->MoveSelectionUp();
			}			
		}
		return 0;

	case KEY_XBUTTON_DOWN:
		{
			if ( pList )
			{
				pList->MoveSelectionDown();
			}		
		}
		return 0;

	case KEY_XBUTTON_RIGHT:
		{
			if ( m_pPlayerListRed->GetItemCount() == 0 )
				return 0;

			// move to the red list

			// get the row we're in now
			int iSelectedBlueItem = m_pPlayerListBlue->GetSelectedItem();

			m_pPlayerListBlue->ClearSelection();

			if ( iSelectedBlueItem >= 0 )
			{
				int row = m_pPlayerListBlue->GetRowFromItemID( iSelectedBlueItem );

				if ( row >= 0 )
				{
					int iNewItem = m_pPlayerListRed->GetItemIDFromRow( row );

					if ( iNewItem >= 0 )
					{
						m_pPlayerListRed->SetSelectedItem( iNewItem );
					}
					else
					{
						// we have fewer items. Select the last one
						int iLastRow = m_pPlayerListRed->GetItemCount()-1;

						iNewItem = m_pPlayerListRed->GetItemIDFromRow( iLastRow );

						if ( iNewItem >= 0 )
						{
							m_pPlayerListRed->SetSelectedItem( iNewItem );
						}
					}
				}
			}
		}
		return 0;

	case KEY_XBUTTON_LEFT:
		{
			if ( m_pPlayerListBlue->GetItemCount() == 0 )
				return 0;

			// move to the blue list

			// get the row we're in now
			int iSelectedRedItem = m_pPlayerListRed->GetSelectedItem();

			if ( iSelectedRedItem < 0 )
				iSelectedRedItem = 0;

			m_pPlayerListRed->ClearSelection();

			if ( iSelectedRedItem >= 0 )
			{
				int row = m_pPlayerListRed->GetRowFromItemID( iSelectedRedItem );

				if ( row >= 0 )
				{
					int iNewItem = m_pPlayerListBlue->GetItemIDFromRow( row );

					if ( iNewItem >= 0 )
					{
						m_pPlayerListBlue->SetSelectedItem( iNewItem );
					}
					else
					{
						// we have fewer items. Select the last one
						int iLastRow = m_pPlayerListBlue->GetItemCount()-1;

						iNewItem = m_pPlayerListBlue->GetItemIDFromRow( iLastRow );

						if ( iNewItem >= 0 )
						{
							m_pPlayerListBlue->SetSelectedItem( iNewItem );
						}
					}
				}
			}
		}
		return 0;

	case KEY_XBUTTON_B:
		{
			ShowPanel( false );
		}
		return 0;

	case KEY_XBUTTON_A:	// Show GamerCard for the selected player
		{
			if ( pList )
			{
				int iSelectedItem = pList->GetSelectedItem();

				if ( iSelectedItem >= 0 )
				{
					KeyValues *pInfo = pList->GetItemData( iSelectedItem );

					DevMsg( 1, "XShowGamerCardUI for player '%s'\n", pInfo->GetString( "name" ) );

					uint64 xuid = matchmaking->PlayerIdToXuid( pInfo->GetInt( "playerIndex" ) );
					XShowGamerCardUI( XBX_GetPrimaryUserId(), xuid );
				}
			}
		}
		return 0;

	case KEY_XBUTTON_X:	// Show player review for the selected player
		{
			if ( pList )
			{
				int iSelectedItem = pList->GetSelectedItem();

				if ( iSelectedItem >= 0 )
				{
					KeyValues *pInfo = pList->GetItemData( iSelectedItem );

					DevMsg( 1, "XShowPlayerReviewUI for player '%s'\n", pInfo->GetString( "name" ) );

					uint64 xuid = matchmaking->PlayerIdToXuid( pInfo->GetInt( "playerIndex" ) );
					XShowPlayerReviewUI( XBX_GetPrimaryUserId(), xuid );
				}
			}
		}
		return 0;
	
	default:
		break;
	}
#endif //_X360

	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool ShouldScoreBoardHandleKeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding )
{
	// We're only looking for specific mouse input
	if ( keynum == MOUSE_LEFT || keynum == MOUSE_RIGHT )
	{
		CTFClientScoreBoardDialog *pScoreBoard = dynamic_cast< CTFClientScoreBoardDialog* >( gViewPortInterface->FindPanelByName( PANEL_SCOREBOARD ) );
		if ( pScoreBoard )
		{
			return !pScoreBoard->HudElementKeyInput( down, keynum, pszCurrentBinding );
		}
	}

	return false;
}
