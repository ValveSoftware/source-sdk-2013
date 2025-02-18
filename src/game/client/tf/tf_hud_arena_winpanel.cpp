
//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "tf_hud_arena_winpanel.h"
#include "tf_hud_statpanel.h"
#include "tf_spectatorgui.h"
#include "vgui_controls/AnimationController.h"
#include "iclientmode.h"
#include "engine/IEngineSound.h"
#include "c_tf_playerresource.h"
#include "c_tf_team.h"
#include "tf_clientscoreboard.h"
#include <vgui_controls/Label.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include "vgui_avatarimage.h"
#include "fmtstr.h"
#include "teamplayroundbased_gamerules.h"
#include "tf_gamerules.h"
#include "tf_hud_menu_taunt_selection.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ConVar tf_arena_max_streak;
extern ConVar mp_bonusroundtime;

extern const char *FormatSeconds( int seconds );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFArenaWinPanel::CTFArenaWinPanel( IViewPort *pViewPort ) : EditablePanel( NULL, "ArenaWinPanel" )
{
	SetAlpha( 0 );
	SetScheme( "ClientScheme" );

	m_pTeamScorePanel = new EditablePanel( this, "ArenaWinPanelScores" );
	m_pWinnerPanel = new EditablePanel( this, "ArenaWinPanelWinnersPanel" );
	m_pLoserPanel = new EditablePanel( this, "ArenaWinPanelLosersPanel" );

	m_flTimeUpdateTeamScore = 0;
	m_flFlipScoresTimes = 0;
	m_iBlueTeamScore = 0;
	m_iRedTeamScore = 0;

	ListenForGameEvent( "arena_win_panel" );
	ListenForGameEvent( "teamplay_round_start" );
	ListenForGameEvent( "teamplay_game_over" );
	ListenForGameEvent( "tf_game_over" );

	m_bShouldBeVisible = false;

	vgui::ivgui()->AddTickSignal( GetVPanel(), 50 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFArenaWinPanel::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFArenaWinPanel::Reset( void )
{
	Update();
	m_bShouldBeVisible = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFArenaWinPanel::Update( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFArenaWinPanel::SetVisible( bool state )
{
	if ( state == IsVisible() )
		return;

	int iRenderGroup = gHUD.LookupRenderGroupIndexByName( "mid" );

	if ( state )
	{
		gHUD.LockRenderGroup( iRenderGroup );
	}
	else
	{
		gHUD.UnlockRenderGroup( iRenderGroup );
	}

	BaseClass::SetVisible( state );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFArenaWinPanel::FireGameEvent( IGameEvent * event )
{
	const char *pEventName = event->GetName();

	if ( Q_strcmp( "teamplay_round_start", pEventName ) == 0 )
	{
		m_bShouldBeVisible = false;
	}
	else if ( Q_strcmp( "teamplay_game_over", pEventName ) == 0 )
	{
		m_bShouldBeVisible = false;
	}
	else if ( Q_strcmp( "tf_game_over", pEventName ) == 0 )
	{
		m_bShouldBeVisible = false;
	}
	else if ( Q_strcmp( "arena_win_panel", pEventName ) == 0 )
	{
		if ( !g_PR )
			return;

		m_vecPlayerScore.RemoveAll();

		InvalidateLayout( false, true );

		if ( TFGameRules() && TFGameRules()->IsInArenaMode() == true && tf_arena_max_streak.GetInt() > 0 && tf_arena_use_queue.GetBool() == true )
		{
			m_pArenaStreakPanel->SetVisible( true );
			m_pArenaStreakLabel->SetVisible( true );

			wchar_t wzStreaNum[16];
			wchar_t wzStreakString[256]=L"";
			_snwprintf( wzStreaNum, ARRAYSIZE( wzStreaNum ), L"%i", tf_arena_max_streak.GetInt() );

			g_pVGuiLocalize->ConstructString_safe( wzStreakString, g_pVGuiLocalize->Find( "#TF_Arena_PlayingTo" ), 1, wzStreaNum );

			m_pTeamScorePanel->SetDialogVariable( "arenastreaktext", wzStreakString );
		}
		else
		{
			m_pArenaStreakPanel->SetVisible( false );
			m_pArenaStreakLabel->SetVisible( false );
		}

		m_iWinningPlayerTeam = event->GetInt( "winning_team" );
		int iWinReason = event->GetInt( "winreason" );
	
		SetDialogVariable( "WinningTeamLabel", "" );
		SetDialogVariable( "AdvancingTeamLabel", "" );
		SetDialogVariable( "WinReasonLabel", "" );
		SetDialogVariable( "DetailsLabel", "" );

		vgui::ImagePanel *pImagePanelBG = dynamic_cast<vgui::ImagePanel *>( FindChildByName("WinPanelBG") );
		Assert( pImagePanelBG );
		if ( !pImagePanelBG )
			return;

		// set the appropriate background image and label text
		const wchar_t *pTeamLabel = L"";
		const wchar_t *pTopPlayersLabel = L"";
		const wchar_t *pLocalizedTeamName = L"";
		const wchar_t *pLocalizedLoserTeamName = L"";

		C_TFTeam *pBlueTeam = GetGlobalTFTeam( TF_TEAM_BLUE );
		const wchar_t *pBlueTeamName = pBlueTeam ? pBlueTeam->Get_Localized_Name() : L"BLU";

		C_TFTeam *pRedTeam = GetGlobalTFTeam( TF_TEAM_RED );
		const wchar_t *pRedTeamName = pRedTeam ? pRedTeam->Get_Localized_Name() : L"RED";

		switch ( m_iWinningPlayerTeam )
		{
		case TF_TEAM_BLUE:
			pImagePanelBG->SetImage( "../hud/winpanel_blue_bg_main.vmt" );
			pTopPlayersLabel = g_pVGuiLocalize->Find( "#Winpanel_BlueMVPs" );
			pLocalizedTeamName = pBlueTeamName;
			pLocalizedLoserTeamName = pRedTeamName;
	  		break;
		case TF_TEAM_RED:
			pImagePanelBG->SetImage( "../hud/winpanel_red_bg_main.vmt" );
			pTopPlayersLabel = g_pVGuiLocalize->Find( "#Winpanel_RedMVPs" );
			pLocalizedTeamName = pRedTeamName;
			pLocalizedLoserTeamName = pBlueTeamName;
			break;
		case TEAM_UNASSIGNED:	// stalemate
			pImagePanelBG->SetImage( "../hud/winpanel_black_bg_main.vmt" );
			pTeamLabel = g_pVGuiLocalize->Find( "#Winpanel_Stalemate" );
			pTopPlayersLabel = g_pVGuiLocalize->Find( "#Winpanel_TopPlayers" );
			break;
		default:
			Assert( false );
			break;
		}

		SetDialogVariable( "TopPlayersLabel", pTopPlayersLabel );
		
		wchar_t wzTeamWin[256] = L"";
		if ( TFGameRules() && TFGameRules()->IsInTournamentMode() )
		{
			g_pVGuiLocalize->ConstructString_safe( wzTeamWin, g_pVGuiLocalize->Find( "#Winpanel_TournamentTeamWins" ), 1, pLocalizedTeamName );
			g_pVGuiLocalize->ConstructString_safe( m_wzTeamLose, g_pVGuiLocalize->Find( "#Winpanel_TournamentTeamLost" ), 2, pLocalizedLoserTeamName );

			wchar_t wzTeamMPVs[256];
			g_pVGuiLocalize->ConstructString_safe( wzTeamMPVs, g_pVGuiLocalize->Find( "#Winpanel_TournamentMVPs" ), 1, pLocalizedTeamName );

			if ( m_iWinningPlayerTeam != TEAM_UNASSIGNED )
			{
				SetDialogVariable( "TopPlayersLabel", wzTeamMPVs );
			}
		}
		else if ( m_iWinningPlayerTeam != TEAM_UNASSIGNED )
		{
			g_pVGuiLocalize->ConstructString_safe( wzTeamWin, g_pVGuiLocalize->Find( "#Winpanel_TeamWins" ), 2, pLocalizedTeamName, g_pVGuiLocalize->Find( "#Winpanel_Team1" ) );
			g_pVGuiLocalize->ConstructString_safe( m_wzTeamLose, g_pVGuiLocalize->Find( "#Winpanel_TeamLost" ), 2, pLocalizedLoserTeamName, g_pVGuiLocalize->Find( "#Winpanel_Team1" ) );
		}

		if ( m_iWinningPlayerTeam != TEAM_UNASSIGNED )
		{
			pTeamLabel = wzTeamWin;
		}
	
		SetDialogVariable( "WinningTeamLabel", pTeamLabel );
		SetDialogVariable( "LosingTeamLabel", "" );
	
		wchar_t wzWinReason[256] = L"";
		switch ( iWinReason )
		{
		case WINREASON_ALL_POINTS_CAPTURED:
			g_pVGuiLocalize->ConstructString_safe( wzWinReason, g_pVGuiLocalize->Find( "#Winreason_AllPointsCaptured" ), 1, pLocalizedTeamName );
			break;
		case WINREASON_OPPONENTS_DEAD:

			if ( TFGameRules() && TFGameRules()->IsInArenaMode() == true )
			{
				g_pVGuiLocalize->ConstructString_safe( wzWinReason, g_pVGuiLocalize->Find( "#Winreason_Arena" ), 1, pLocalizedTeamName );
			}
			else
			{
				g_pVGuiLocalize->ConstructString_safe( wzWinReason, g_pVGuiLocalize->Find( "#Winreason_OpponentsDead" ), 1, pLocalizedTeamName );
			}
			break;

		case WINREASON_STALEMATE:
			if ( !TFGameRules() || !TFGameRules()->IsCompetitiveMode() )
			{
				g_pVGuiLocalize->ConstructString_safe( wzWinReason, g_pVGuiLocalize->Find( "#Winreason_Stalemate" ), 0 );
			}
			break;	

			default:
			Assert( false );
			break;
		}
		SetDialogVariable( "WinReasonLabel", wzWinReason );

		m_bWasFlawlessVictory = IsFlawlessVictory();

		if ( WINREASON_ALL_POINTS_CAPTURED == iWinReason )
		{
			// if this was a full round that ended with point capture or flag capture, show the winning cappers
			const char *pCappers = event->GetString( "cappers" );
			int iCappers = Q_strlen( pCappers );
			if ( iCappers > 0 )
			{	
				char szPlayerNames[256]="";
				wchar_t wzPlayerNames[256]=L"";
				wchar_t wzCapMsg[512]=L"";
				for ( int i = 0; i < iCappers; i++ )
				{
					Q_strncat( szPlayerNames, g_PR->GetPlayerName( (int) pCappers[i] ), ARRAYSIZE( szPlayerNames ) );
					if ( i < iCappers - 1 )
					{
						Q_strncat( szPlayerNames, ", ", ARRAYSIZE( szPlayerNames ) );
					}
				}
				g_pVGuiLocalize->ConvertANSIToUnicode( szPlayerNames, wzPlayerNames, sizeof( wzPlayerNames ) );
				g_pVGuiLocalize->ConstructString_safe( wzCapMsg, g_pVGuiLocalize->Find( "#Winpanel_WinningCapture" ), 1, wzPlayerNames );
				SetDialogVariable( "DetailsLabel", wzCapMsg );
			}
		}
		else if ( TFGameRules()->IsInArenaMode() == true && m_iWinningPlayerTeam != TEAM_UNASSIGNED )
		{
			if ( m_bWasFlawlessVictory )
			{
				SetDialogVariable( "DetailsLabel", g_pVGuiLocalize->Find( "#TF_Arena_FlawlessVictory" ) );
			}
		}

		// get the current & previous team scores
		int iBlueTeamPrevScore = event->GetInt( "blue_score_prev", 0 );
		int iRedTeamPrevScore = event->GetInt( "red_score_prev", 0 );
		m_iBlueTeamScore = event->GetInt( "blue_score", 0 );
		m_iRedTeamScore = event->GetInt( "red_score", 0 );
		
		if ( m_pTeamScorePanel )
		{			
			m_pTeamScorePanel->SetDialogVariable( "blueteamname", pBlueTeamName );
			m_pTeamScorePanel->SetDialogVariable( "redteamname", pRedTeamName );

			// set the previous team scores in scoreboard
			m_pTeamScorePanel->SetDialogVariable( "blueteamscore", iBlueTeamPrevScore );
			m_pTeamScorePanel->SetDialogVariable( "redteamscore", iRedTeamPrevScore );

			if ( ( m_iBlueTeamScore != iBlueTeamPrevScore ) || ( m_iRedTeamScore != iRedTeamPrevScore ) )
			{
				// if the new scores are different, set ourselves to update the scoreboard to the new values after a short delay, so players
				// see the scores tick up
				m_flTimeUpdateTeamScore = gpGlobals->curtime + 3.5f;
			}

			// only show team scores if round is complete
			m_pTeamScorePanel->SetVisible( true );
		}

		if ( !g_TF_PR )
			return;

		int iPanelIndex = 0;

		// look for the top 6 players sent in the event
		for ( int i = 1; i <= 6; i++ )
		{
			iPanelIndex = (iPanelIndex % 3) + 1;

			EditablePanel *pParentPanel = NULL;

			if ( i <= 3 )
			{
				pParentPanel = m_pWinnerPanel;
			}
			else
			{
				pParentPanel = m_pLoserPanel;
			}

			vgui::Label *pPlayerName = dynamic_cast<Label *>( pParentPanel->FindChildByName( CFmtStr( "Player%dName", iPanelIndex ) ) );
			vgui::Label *pPlayerClass = dynamic_cast<Label *>( pParentPanel->FindChildByName( CFmtStr( "Player%dClass", iPanelIndex ) ) );
			vgui::Label *pPlayerDamage = dynamic_cast<Label *>( pParentPanel->FindChildByName( CFmtStr( "Player%dDamage", iPanelIndex ) ) );
			vgui::Label *pPlayerHealing = dynamic_cast<Label *>( pParentPanel->FindChildByName( CFmtStr( "Player%dHealing", iPanelIndex ) ) );
			vgui::Label *pPlayerLifetime = dynamic_cast<Label *>( pParentPanel->FindChildByName( CFmtStr( "Player%dLifetime", iPanelIndex ) ) );
			vgui::Label *pPlayerKills = dynamic_cast<Label *>( pParentPanel->FindChildByName( CFmtStr( "Player%dKills", iPanelIndex ) ) );
			CAvatarImagePanel *pPlayerAvatar = dynamic_cast<CAvatarImagePanel *>( pParentPanel->FindChildByName( CFmtStr( "Player%dAvatar", iPanelIndex) ) );

			if ( !pPlayerName || !pPlayerClass || !pPlayerDamage || !pPlayerHealing || !pPlayerLifetime || !pPlayerKills || !pPlayerAvatar )
				continue;

			pPlayerName->SetText( "" );
			pPlayerClass->SetText( "" );
			pPlayerDamage->SetText( "" );
			pPlayerHealing->SetText( "" );
			pPlayerLifetime->SetText( "" );
			pPlayerKills->SetText( "" );
			pPlayerAvatar->SetPlayer( nullptr );

			char szPlayerIndexVal[64]="", szPlayerDamageVal[64]="", szPlayerHealingVal[64]="", szPlayerTimeAliveVal[64]="", szPlayerKillingBlowsVal[64]="";
			// get player index and round points from the event
			Q_snprintf( szPlayerIndexVal, ARRAYSIZE( szPlayerIndexVal ), "player_%d", i );
			Q_snprintf( szPlayerDamageVal, ARRAYSIZE( szPlayerDamageVal ), "player_%d_damage", i );
			Q_snprintf( szPlayerHealingVal, ARRAYSIZE( szPlayerHealingVal ), "player_%d_healing", i );
			Q_snprintf( szPlayerTimeAliveVal, ARRAYSIZE( szPlayerTimeAliveVal ), "player_%d_lifetime", i );
			Q_snprintf( szPlayerKillingBlowsVal, ARRAYSIZE( szPlayerKillingBlowsVal ), "player_%d_kills", i );

			int iPlayerIndex = event->GetInt( szPlayerIndexVal, 0 );

			if ( iPlayerIndex == 0 )
				continue;

			if ( g_TF_PR->IsConnected( iPlayerIndex ) == false )
				continue;

			int iDamageDone = event->GetInt( szPlayerDamageVal, 0 );
			int iHealingDone = event->GetInt( szPlayerHealingVal, 0 );
			int iTimeAlive = event->GetInt( szPlayerTimeAliveVal, 0 );
			int iKillingBlows = event->GetInt( szPlayerKillingBlowsVal, 0 );

			PlayerArenaRoundScore_t &playerRoundScore = m_vecPlayerScore[m_vecPlayerScore.AddToTail()];

			playerRoundScore.iPlayerIndex = iPlayerIndex;
			playerRoundScore.iTotalDamage = iDamageDone;
			playerRoundScore.iTotalHealing = iHealingDone;
			playerRoundScore.iTimeAlive = iTimeAlive;
			playerRoundScore.iKillingBlows = iKillingBlows;
		}

		m_bShouldBeVisible = true;

		SetupPlayerStats();

		MoveToFront();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFArenaWinPanel::IsFlawlessVictory( void )
{
	C_TFTeam *pTeam = GetGlobalTFTeam( m_iWinningPlayerTeam );
	bool bAllAlive = true;

	if ( pTeam && g_TF_PR )
	{
		if ( pTeam->GetNumPlayers() == 1 )
			return false;

		if ( g_TF_PR->GetNumPlayersForTeam( m_iWinningPlayerTeam, true ) != pTeam->GetNumPlayers() )
		{
			bAllAlive = false;
		}
	}

	return bAllAlive;
}

//-----------------------------------------------------------------------------
// Purpose: Applies scheme settings
//-----------------------------------------------------------------------------
void CTFArenaWinPanel::SetupPlayerStats( void )
{
	if ( !g_TF_PR )
		return;

	m_pWinnerPanel->SetVisible( true );
	m_pLoserPanel->SetVisible( false );

	m_flFlipScoresTimes = gpGlobals->curtime + ( ( TFGameRules() ? TFGameRules()->GetBonusRoundTime() : 5.0f ) * 0.5f );

	int iWinnersAdded = 1;
	int iLosersAdded = 1;

	int iPlayerAdded = 0;

	EditablePanel *pParentPanel = m_pWinnerPanel;

	// look for the top 6 players sent in the event
	for ( int i = 0; i < m_vecPlayerScore.Count(); i++ )
	{
		bool bShow = true;

		int iPlayerIndex = m_vecPlayerScore[i].iPlayerIndex;
		int iIndex = 0;
	
		if ( m_iWinningPlayerTeam != TEAM_UNASSIGNED )
		{
			if ( m_iWinningPlayerTeam == g_TF_PR->GetTeam( iPlayerIndex ) )
			{
				pParentPanel = m_pWinnerPanel;
				iIndex = iWinnersAdded;

				iWinnersAdded++;
			}
			else
			{
				pParentPanel = m_pLoserPanel;
				iIndex = iLosersAdded;

				iLosersAdded++;
			}
		}
		else
		{
			if ( i >= ( m_vecPlayerScore.Count() * 0.5f ) && pParentPanel != m_pLoserPanel )
			{
				iPlayerAdded = 0;
				pParentPanel = m_pLoserPanel;
			}

			iIndex = (iPlayerAdded + 1) % 4;

			iPlayerAdded++;
		}


		int iDamageDone = m_vecPlayerScore[i].iTotalDamage;
		int iHealingDone = m_vecPlayerScore[i].iTotalHealing;
		int iTimeAlive = m_vecPlayerScore[i].iTimeAlive;
		int iKillingBlows = m_vecPlayerScore[i].iKillingBlows;

		CAvatarImagePanel *pPlayerAvatar = dynamic_cast<CAvatarImagePanel *>( pParentPanel->FindChildByName( CFmtStr( "Player%dAvatar", iIndex) ) );
		if ( pPlayerAvatar )
		{
			pPlayerAvatar->SetShouldScaleImage( true );
			pPlayerAvatar->SetShouldDrawFriendIcon( false );

			if ( bShow )
			{
				CBasePlayer *pPlayer = UTIL_PlayerByIndex( iPlayerIndex );
				pPlayerAvatar->SetPlayer( pPlayer );
			}

			pPlayerAvatar->SetVisible( bShow );
		}

		vgui::Label *pPlayerName = dynamic_cast<Label *>( pParentPanel->FindChildByName( CFmtStr( "Player%dName", iIndex ) ) );
		vgui::Label *pPlayerClass = dynamic_cast<Label *>( pParentPanel->FindChildByName( CFmtStr( "Player%dClass", iIndex ) ) );
		vgui::Label *pPlayerDamage = dynamic_cast<Label *>( pParentPanel->FindChildByName( CFmtStr( "Player%dDamage", iIndex ) ) );
		vgui::Label *pPlayerHealing = dynamic_cast<Label *>( pParentPanel->FindChildByName( CFmtStr( "Player%dHealing", iIndex ) ) );
		vgui::Label *pPlayerLifetime = dynamic_cast<Label *>( pParentPanel->FindChildByName( CFmtStr( "Player%dLifetime", iIndex ) ) );
		vgui::Label *pPlayerKills = dynamic_cast<Label *>( pParentPanel->FindChildByName( CFmtStr( "Player%dKills", iIndex ) ) );

		if ( !pPlayerName || !pPlayerClass || !pPlayerDamage || !pPlayerHealing || !pPlayerLifetime || !pPlayerKills )
			continue;

		if ( bShow )
		{
			// set the player labels to team color
			Color clr = g_TF_PR->GetTeamColor( g_TF_PR->GetTeam( iPlayerIndex ) );				
			pPlayerName->SetFgColor( clr );
			pPlayerClass->SetFgColor( clr );
			pPlayerDamage->SetFgColor( clr );
			pPlayerHealing->SetFgColor( clr );
			pPlayerLifetime->SetFgColor( clr );
			pPlayerKills->SetFgColor( clr );

			// set label contents
			pPlayerName->SetText( g_TF_PR->GetPlayerName( iPlayerIndex ) );
			pPlayerClass->SetText( g_aPlayerClassNames[g_TF_PR->GetPlayerClass( iPlayerIndex )] );
			pPlayerDamage->SetText( CFmtStr( "%d", iDamageDone ) );
			pPlayerHealing->SetText( CFmtStr( "%d", iHealingDone ) );
			pPlayerLifetime->SetText( CFmtStr( "%s", FormatSeconds( iTimeAlive ) ) );
			pPlayerKills->SetText( CFmtStr( "%d", iKillingBlows ) );
		}

		// show or hide labels for this player position
		pPlayerName->SetVisible( bShow );
		pPlayerClass->SetVisible( bShow );
		pPlayerDamage->SetVisible( bShow );
		pPlayerHealing->SetVisible( bShow );
		pPlayerLifetime->SetVisible( bShow );
		pPlayerKills->SetVisible( bShow );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Applies scheme settings
//-----------------------------------------------------------------------------
void CTFArenaWinPanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "resource/UI/HudArenaWinPanel.res" );	

	m_pArenaStreakPanel = m_pTeamScorePanel->FindChildByName( "ArenaStreaksBG" );
	m_pArenaStreakLabel = m_pTeamScorePanel->FindChildByName("ArenaStreakLabel" );
}

//-----------------------------------------------------------------------------
// Purpose: panel think method
//-----------------------------------------------------------------------------
void CTFArenaWinPanel::OnTick()
{
	// if we've scheduled ourselves to update the team scores, handle it now
	if ( m_flTimeUpdateTeamScore > 0 && ( gpGlobals->curtime > 	m_flTimeUpdateTeamScore ) && m_pTeamScorePanel )
	{
		// play a sound
		CLocalPlayerFilter filter;
		C_BaseEntity::EmitSound( filter, SOUND_FROM_LOCAL_PLAYER, "Hud.EndRoundScored" );

		// update the team scores
		m_pTeamScorePanel->SetDialogVariable( "blueteamscore", m_iBlueTeamScore );
		m_pTeamScorePanel->SetDialogVariable( "redteamscore", m_iRedTeamScore );
		m_flTimeUpdateTeamScore = 0;
	}

	if ( m_flFlipScoresTimes > 0 && ( gpGlobals->curtime > 	m_flFlipScoresTimes ) )
	{
		//flip the scores
		m_pWinnerPanel->SetVisible( false );
		m_pLoserPanel->SetVisible( true );

		m_flFlipScoresTimes = 0;

		if ( m_iWinningPlayerTeam != TEAM_UNASSIGNED )
		{
			const wchar_t *pTopPlayersLabel = NULL;

			vgui::ImagePanel *pImagePanelBG = dynamic_cast<vgui::ImagePanel *>( FindChildByName("WinPanelBG") );

			if ( pImagePanelBG == NULL )
				return;
		
			switch ( m_iWinningPlayerTeam )
			{
			case TF_TEAM_BLUE:
				pTopPlayersLabel = g_pVGuiLocalize->Find( "#Winpanel_RedMVPs" );
				pImagePanelBG->SetImage( "../hud/winpanel_red_bg_main.vmt" );
				break;
			case TF_TEAM_RED:
				pTopPlayersLabel = g_pVGuiLocalize->Find( "#Winpanel_BlueMVPs" );
				pImagePanelBG->SetImage( "../hud/winpanel_blue_bg_main.vmt" );
				break;
			default:
				Assert( false );
				break;
			}

			SetDialogVariable( "TopPlayersLabel", pTopPlayersLabel );

			SetDialogVariable( "LosingTeamLabel", m_wzTeamLose );
			SetDialogVariable( "WinningTeamLabel", "" );


			if ( m_bWasFlawlessVictory )
			{
				CLocalPlayerFilter filter;

				if ( m_iWinningPlayerTeam == GetLocalPlayerTeam() )
				{
					C_BaseEntity::EmitSound( filter, SOUND_FROM_LOCAL_PLAYER, "Announcer.AM_FlawlessVictoryRandom" );
				}
				else if ( GetLocalPlayerTeam() == TEAM_SPECTATOR )
				{
					C_BaseEntity::EmitSound( filter, SOUND_FROM_LOCAL_PLAYER, "Announcer.AM_FlawlessVictory01" );
				}
				else
				{
					C_BaseEntity::EmitSound( filter, SOUND_FROM_LOCAL_PLAYER, "Announcer.AM_FlawlessDefeatRandom" );
				}
			}
		}
	}

	if ( m_bShouldBeVisible == true )
	{
		IViewPortPanel *scoreboard = gViewPortInterface->FindPanelByName( PANEL_SCOREBOARD );
		if ( ( scoreboard && scoreboard->IsVisible() ) || IsInFreezeCam() )
		{
			SetVisible( false );
			return;
		}

		CHudMenuTauntSelection *pMenuTauntSelection = ( CHudMenuTauntSelection * )GET_HUDELEMENT( CHudMenuTauntSelection );
		if ( pMenuTauntSelection && pMenuTauntSelection->IsVisible() )
		{
			SetVisible( false );
			return;
		}

		if ( TFGameRules() && TFGameRules()->State_Get() != GR_STATE_TEAM_WIN )
		{
			m_bShouldBeVisible = false;
		}
	}

	SetVisible( m_bShouldBeVisible );
}
