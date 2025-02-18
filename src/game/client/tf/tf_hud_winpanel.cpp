
//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "tf_hud_winpanel.h"
#include "tf_hud_statpanel.h"
#include "tf_spectatorgui.h"
#include "vgui_controls/AnimationController.h"
#include "iclientmode.h"
#include "engine/IEngineSound.h"
#include "c_tf_playerresource.h"
#include "c_team.h"
#include "tf_clientscoreboard.h"
#include <vgui_controls/Label.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include "vgui_avatarimage.h"
#include "fmtstr.h"
#include "teamplayroundbased_gamerules.h"
#include "tf_gamerules.h"
#include "tf_logic_halloween_2014.h"
#include "c_tf_team.h"
#include "tf_badge_panel.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

DECLARE_HUDELEMENT_DEPTH( CTFWinPanel, 1 );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFWinPanel::CTFWinPanel( const char *pElementName ) : EditablePanel( NULL, "WinPanel" ), CHudElement( pElementName )
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );
	m_bShouldBeVisible = false;
	SetAlpha( 0 );
	SetScheme( "ClientScheme" );

	m_pTeamScorePanel = new EditablePanel( this, "TeamScoresPanel" );
	m_pRedTeamName = new CExLabel( m_pTeamScorePanel, "RedTeamLabel", "" );
	m_pBlueTeamName = new CExLabel( m_pTeamScorePanel, "BlueTeamLabel", "" );
	m_pRedLeaderAvatarImage = new CAvatarImagePanel( m_pTeamScorePanel, "RedLeaderAvatar" );
	m_pBlueLeaderAvatarImage = new CAvatarImagePanel( m_pTeamScorePanel, "BlueLeaderAvatar" );
	m_pRedLeaderAvatarBG = new EditablePanel( m_pTeamScorePanel, "RedLeaderAvatarBG" );
	m_pBlueLeaderAvatarBG = new EditablePanel( m_pTeamScorePanel, "BlueLeaderAvatarBG" );
	m_flTimeUpdateTeamScore = 0;
	m_iBlueTeamScore = 0;
	m_iRedTeamScore = 0;

	RegisterForRenderGroup( "mid" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWinPanel::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWinPanel::Reset()
{
	m_bShouldBeVisible = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWinPanel::Init()
{
	// listen for events
	ListenForGameEvent( "teamplay_win_panel" );
	ListenForGameEvent( "teamplay_round_start" );
	ListenForGameEvent( "teamplay_game_over" );
	ListenForGameEvent( "tf_game_over" );
	ListenForGameEvent( "training_complete" );
	ListenForGameEvent( "show_match_summary" );

	m_bShouldBeVisible = false;

	CHudElement::Init();
}

void CTFWinPanel::SetVisible( bool state )
{
	if ( state == IsVisible() )
		return;

	if ( state )
	{
		HideLowerPriorityHudElementsInGroup( "mid" );
	}
	else
	{
		UnhideLowerPriorityHudElementsInGroup( "mid" );
	}

	BaseClass::SetVisible( state );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWinPanel::FireGameEvent( IGameEvent * event )
{
	const char *pEventName = event->GetName();

	if ( FStrEq( "teamplay_round_start", pEventName ) ||
	     FStrEq( "teamplay_game_over", pEventName ) ||
		 FStrEq( "tf_game_over", pEventName ) ||
		 FStrEq( "training_complete", pEventName ) ||
		 FStrEq( "show_match_summary", pEventName ) )
	{
		m_bShouldBeVisible = false;
	}
	else if ( Q_strcmp( "teamplay_win_panel", pEventName ) == 0 )
	{
		if ( !g_PR )
			return;

		vgui::IScheme* pScheme = scheme()->GetIScheme( GetScheme() );
		int iWinningTeam = event->GetInt( "winning_team" );
		int iWinReason = event->GetInt( "winreason" );
		int iFlagCapLimit = event->GetInt( "flagcaplimit" );
		bool bRoundComplete = (bool) event->GetInt( "round_complete" );
		int iRoundsRemaining = event->GetInt( "rounds_remaining" );
		bool bGameOver = event->GetBool( "game_over", false );
		bool bUseMoreOpaqueBorder = false;
		if ( TFGameRules() && bGameOver )
		{
			if ( TFGameRules()->IsMatchTypeCompetitive() )
			{
				bUseMoreOpaqueBorder = true;
			}
		}

		// non-final rounds of stopwatch mode should say something different
		CTeamRoundTimer *pTimer = NULL;
		if ( TFGameRules() && TFGameRules()->IsInTournamentMode() && TFGameRules()->IsInStopWatch() )
		{
			int iActiveTimer = ObjectiveResource()->GetStopWatchTimer();
			pTimer = dynamic_cast< CTeamRoundTimer* >( ClientEntityList().GetEnt( iActiveTimer ) );
			if ( pTimer )
			{
				if ( pTimer->IsWatchingTimeStamps() )
				{
					iWinningTeam = TEAM_INVALID;
					iWinReason = bRoundComplete ? WINREASON_STOPWATCH_WATCHING_FINAL_ROUND : WINREASON_STOPWATCH_WATCHING_ROUNDS;
				}
				else
				{
					if ( !TFGameRules()->HaveStopWatchWinner() && !bRoundComplete )
					{
						iWinningTeam = TEAM_INVALID;
						iWinReason = WINREASON_STOPWATCH_PLAYING_ROUNDS;
					}
				}
			}
		}

		LoadControlSettings( "resource/UI/WinPanel.res" );		
		InvalidateLayout( false, true );

		SetDialogVariable( "WinningTeamLabel", "" );
		SetDialogVariable( "AdvancingTeamLabel", "" );
		SetDialogVariable( "WinReasonLabel", "" );
		SetDialogVariable( "DetailsLabel", "" );

		EditablePanel *pBGPanel = dynamic_cast<EditablePanel *>( FindChildByName("WinPanelBGBorder") );
		Assert( pBGPanel );
		if ( !pBGPanel )
			return;

		EditablePanel *pBlueBGPanel = FindControl< EditablePanel >( "BlueScoreBG", true );
		Assert( pBlueBGPanel );
		EditablePanel *pRedBGPanel = FindControl< EditablePanel >( "RedScoreBG", true );
		Assert( pRedBGPanel );
		if( !pBlueBGPanel || !pRedBGPanel )
			return;

		pBlueBGPanel->SetBorder( pScheme->GetBorder( bUseMoreOpaqueBorder ? "TFFatLineBorderBlueBGMoreOpaque" : "TFFatLineBorderBlueBG" ) );
		pRedBGPanel->SetBorder( pScheme->GetBorder( bUseMoreOpaqueBorder ? "TFFatLineBorderRedBGMoreOpaque" : "TFFatLineBorderRedBG" ) ) ;

		// we want to suppress the winreason for sd_doomsday_event and plr_hightower_event
		if ( TFGameRules() )
		{
			if ( ( TFGameRules()->IsHalloweenScenario( CTFGameRules::HALLOWEEN_SCENARIO_DOOMSDAY ) && CTFMinigameLogic::GetMinigameLogic() && CTFMinigameLogic::GetMinigameLogic()->GetActiveMinigame() ) ||
				 ( TFGameRules()->IsHalloweenScenario( CTFGameRules::HALLOWEEN_SCENARIO_HIGHTOWER ) ) )
			{
				iWinReason = WINREASON_NONE;
			}
		}

		// this is an area defense, but not a round win, if this was a successful defend until time limit but not a complete round
		bool bIsAreaDefense = ( ( WINREASON_DEFEND_UNTIL_TIME_LIMIT == iWinReason ) && !bRoundComplete );

		// set the appropriate background image and label text
		const wchar_t *pTeamLabel = L"";
		const wchar_t *pTopPlayersLabel = L"";
		const wchar_t *pLocalizedTeamName = L"";
		const char *pWinTeamLabel = ( bRoundComplete ? "#Winpanel_TeamWins" : ( bIsAreaDefense ? "#Winpanel_TeamDefends" : "#Winpanel_TeamAdvances" ) );

		C_TFTeam *pBlueTeam = GetGlobalTFTeam( TF_TEAM_BLUE );
		const wchar_t *pBlueTeamName = pBlueTeam ? pBlueTeam->Get_Localized_Name() : L"BLU";

		C_TFTeam *pRedTeam = GetGlobalTFTeam( TF_TEAM_RED );
		const wchar_t *pRedTeamName = pRedTeam ? pRedTeam->Get_Localized_Name() : L"RED";

		if ( TFGameRules() && TFGameRules()->IsInTournamentMode() )
		{
			pWinTeamLabel = ( bRoundComplete ? "#Winpanel_TournamentTeamWins" : ( bIsAreaDefense ? "#Winpanel_TournamentTeamDefends" : "#Winpanel_TournamentTeamAdvances" ) );
		}

		wchar_t wzTeamWin[256] = L"";
		switch ( iWinningTeam )
		{
		case TF_TEAM_BLUE:
			pBGPanel->SetBorder( pScheme->GetBorder( bUseMoreOpaqueBorder ? "TFFatLineBorderBlueBGMoreOpaque" : "TFFatLineBorderBlueBG" ) );
			pTopPlayersLabel = g_pVGuiLocalize->Find( "#Winpanel_BlueMVPs" );
			pLocalizedTeamName = pBlueTeamName;
			g_pVGuiLocalize->ConstructString_safe( wzTeamWin, g_pVGuiLocalize->Find( pWinTeamLabel ), 2, pLocalizedTeamName, g_pVGuiLocalize->Find( "#Winpanel_Team1" ) );
			pTeamLabel = wzTeamWin;
			break;
		case TF_TEAM_RED:
			pBGPanel->SetBorder( pScheme->GetBorder( bUseMoreOpaqueBorder ? "TFFatLineBorderRedBGMoreOpaque" : "TFFatLineBorderRedBG" ) );
			pTopPlayersLabel = g_pVGuiLocalize->Find( "#Winpanel_RedMVPs" );
			pLocalizedTeamName = pRedTeamName;
			break;
		case TEAM_UNASSIGNED:	// stalemate
			pBGPanel->SetBorder( pScheme->GetBorder( "TFFatLineBorder" ) );
			pTeamLabel = g_pVGuiLocalize->Find( "#Winpanel_Stalemate" );
			pTopPlayersLabel = g_pVGuiLocalize->Find( "#Winpanel_TopPlayers" );
			break;
		case TEAM_INVALID:		// used for stopwatch mode when it's not the final victory yet
			pBGPanel->SetBorder( pScheme->GetBorder( "TFFatLineBorder" ) );
			pTopPlayersLabel = g_pVGuiLocalize->Find( "#Winpanel_TopPlayers" );
			pTeamLabel = L"";
			if ( pBlueTeam && pBlueTeamName && pRedTeamName )
			{
				if ( bRoundComplete )
				{
					pTeamLabel = g_pVGuiLocalize->Find( "#WinPanel_StopWatch_Watching_RoundFinal" );
				}
				else
				{
					bool bBlueAttackers = ( pBlueTeam->GetRole() == TEAM_ROLE_ATTACKERS );
					g_pVGuiLocalize->ConstructString_safe( wzTeamWin,
						g_pVGuiLocalize->Find( "#WinPanel_StopWatch_Round_Complete" ),
						1,
						bBlueAttackers ? pBlueTeamName : pRedTeamName );

					pTeamLabel = wzTeamWin;
				}
			}
			break;
		default:
			Assert( false );
			break;
		}

		SetDialogVariable( "TopPlayersLabel", pTopPlayersLabel );

		if ( TFGameRules() && TFGameRules()->IsInTournamentMode() && !TFGameRules()->IsInStopWatch() )
		{
			g_pVGuiLocalize->ConstructString_safe( wzTeamWin, g_pVGuiLocalize->Find( pWinTeamLabel ), 1, pLocalizedTeamName );

			wchar_t wzTeamMPVs[256];
			g_pVGuiLocalize->ConstructString_safe( wzTeamMPVs, g_pVGuiLocalize->Find( "#Winpanel_TournamentMVPs" ), 1, pLocalizedTeamName );

			if ( iWinningTeam != TEAM_UNASSIGNED )
			{
				SetDialogVariable( "TopPlayersLabel", wzTeamMPVs );
			}
		}
		else if ( ( iWinningTeam != TEAM_UNASSIGNED ) && ( iWinningTeam != TEAM_INVALID ) )
		{
			g_pVGuiLocalize->ConstructString_safe( wzTeamWin, g_pVGuiLocalize->Find( pWinTeamLabel ), 2, pLocalizedTeamName, g_pVGuiLocalize->Find( "#Winpanel_Team1" ) );
		}

		if ( ( iWinningTeam != TEAM_UNASSIGNED ) && ( iWinningTeam != TEAM_INVALID ) )
		{
			pTeamLabel = wzTeamWin;
		}

		SetDialogVariable( bRoundComplete ? "WinningTeamLabel" : "AdvancingTeamLabel", pTeamLabel );

		wchar_t wzWinReason[256] = L"";
		switch ( iWinReason )
		{
		case WINREASON_ALL_POINTS_CAPTURED:
			{
				if ( TFGameRules() && ( TFGameRules()->GetGameType() == TF_GAMETYPE_ESCORT ) && ( TFGameRules()->HasMultipleTrains() == true ) && ( iRoundsRemaining == 0 ) )
				{
					g_pVGuiLocalize->ConstructString_safe( wzWinReason, g_pVGuiLocalize->Find( "#Winreason_PayloadRace" ), 1, pLocalizedTeamName );
				}
				else
				{
					g_pVGuiLocalize->ConstructString_safe( wzWinReason, g_pVGuiLocalize->Find( "#Winreason_AllPointsCaptured" ), 1, pLocalizedTeamName );
				}
			}
			break;
		case WINREASON_FLAG_CAPTURE_LIMIT:
			{
				wchar_t wzFlagCaptureLimit[16];
				_snwprintf( wzFlagCaptureLimit, ARRAYSIZE( wzFlagCaptureLimit), L"%i", iFlagCapLimit );

				const wchar_t *wpszFormatString = NULL;
				if ( iFlagCapLimit == 1 )
				{
					wpszFormatString = g_pVGuiLocalize->Find( "#Winreason_FlagCaptureLimit_One" );
				}
				if ( !wpszFormatString )
				{
					wpszFormatString = g_pVGuiLocalize->Find( "#Winreason_FlagCaptureLimit" );
				}

				g_pVGuiLocalize->ConstructString_safe( wzWinReason, wpszFormatString, 2,
					pLocalizedTeamName, wzFlagCaptureLimit );
			}			
			break;
		case WINREASON_OPPONENTS_DEAD:
			g_pVGuiLocalize->ConstructString_safe( wzWinReason, g_pVGuiLocalize->Find( "#Winreason_OpponentsDead" ), 1, pLocalizedTeamName );
			break;
		case WINREASON_DEFEND_UNTIL_TIME_LIMIT:
			g_pVGuiLocalize->ConstructString_safe( wzWinReason, g_pVGuiLocalize->Find( "#Winreason_DefendedUntilTimeLimit" ), 1, pLocalizedTeamName );
			break;
		case WINREASON_STALEMATE:
			if ( !TFGameRules() || !TFGameRules()->IsCompetitiveMode() )
			{
				g_pVGuiLocalize->ConstructString_safe( wzWinReason, g_pVGuiLocalize->Find( "#Winreason_Stalemate" ), 0 );
			}
			break;	
		case WINREASON_TIMELIMIT:
			g_pVGuiLocalize->ConstructString_safe( wzWinReason, g_pVGuiLocalize->Find( "#Winreason_TimeLimit" ), 1, pLocalizedTeamName );
			break;
		case WINREASON_WINLIMIT:
			g_pVGuiLocalize->ConstructString_safe( wzWinReason, g_pVGuiLocalize->Find( "#Winreason_WinLimit" ), 1, pLocalizedTeamName );
			break;
		case WINREASON_WINDIFFLIMIT:
			g_pVGuiLocalize->ConstructString_safe( wzWinReason, g_pVGuiLocalize->Find( "#Winreason_WinDiffLimit" ), 1, pLocalizedTeamName );
			break;
		case WINREASON_RD_REACTOR_CAPTURED:
			g_pVGuiLocalize->ConstructString_safe( wzWinReason, g_pVGuiLocalize->Find( "#Winreason_ReactorCaptured" ), 1, pLocalizedTeamName );
			break;
		case WINREASON_RD_CORES_COLLECTED:
			g_pVGuiLocalize->ConstructString_safe( wzWinReason, g_pVGuiLocalize->Find( "#Winreason_CoresCollected" ), 1, pLocalizedTeamName );
			break;
		case WINREASON_RD_REACTOR_RETURNED:
			g_pVGuiLocalize->ConstructString_safe( wzWinReason, g_pVGuiLocalize->Find( "#Winreason_ReactorReturned" ), 1, pLocalizedTeamName );
			break;
		case WINREASON_PD_POINTS:
			g_pVGuiLocalize->ConstructString_safe( wzWinReason, g_pVGuiLocalize->Find( "#Winreason_PlayerDestructionPoints" ), 1, pLocalizedTeamName );
			break;
		case WINREASON_SCORED:
			{
				wchar_t wzScoreLimit[16];
				_snwprintf( wzScoreLimit, ARRAYSIZE( wzScoreLimit ), L"%i", iFlagCapLimit );

				const wchar_t *wpszFormatString = NULL;
				if ( iFlagCapLimit == 1 )
				{
					wpszFormatString = g_pVGuiLocalize->Find( "#Winreason_ScoreLimit_One" );
				}
				if ( !wpszFormatString )
				{
					wpszFormatString = g_pVGuiLocalize->Find( "#Winreason_ScoreLimit" );
				}

				g_pVGuiLocalize->ConstructString_safe( wzWinReason, wpszFormatString, 2,
					pLocalizedTeamName, wzScoreLimit );
			}			
			break;
		case WINREASON_STOPWATCH_WATCHING_ROUNDS:
			if ( pBlueTeam && pBlueTeamName && pRedTeamName )
			{
				bool bBlueAttackers = ( pBlueTeam->GetRole() == TEAM_ROLE_ATTACKERS );
				g_pVGuiLocalize->ConstructString_safe( wzWinReason,
					g_pVGuiLocalize->Find( "#Winreason_Stopwatch_Watching_Rounds" ),
					2,
					bBlueAttackers ? pBlueTeamName : pRedTeamName,
					bBlueAttackers ? pRedTeamName : pBlueTeamName );
			}
			break;
		case WINREASON_STOPWATCH_WATCHING_FINAL_ROUND:
			if ( pBlueTeam && pBlueTeamName && pRedTeamName )
			{
				bool bBlueAttackers = ( pBlueTeam->GetRole() == TEAM_ROLE_ATTACKERS );
				g_pVGuiLocalize->ConstructString_safe( wzWinReason,
					g_pVGuiLocalize->Find( "#Winreason_Stopwatch_SwitchSides" ),
					2,
					bBlueAttackers ? pRedTeamName : pBlueTeamName,
					bBlueAttackers ? pBlueTeamName : pRedTeamName );
			}
			break;
		case WINREASON_STOPWATCH_PLAYING_ROUNDS:
			if ( pBlueTeam && pBlueTeamName && pRedTeamName )
			{
				bool bBlueAttackers = ( pBlueTeam->GetRole() == TEAM_ROLE_ATTACKERS );
				g_pVGuiLocalize->ConstructString_safe( wzWinReason,
					g_pVGuiLocalize->Find( "#Winreason_Stopwatch_Playing_Rounds" ),
					2,
					bBlueAttackers ? pBlueTeamName : pRedTeamName,
					bBlueAttackers ? pRedTeamName : pBlueTeamName );
			}
			break;
		default:
			// This happens at the end of the Soldier training mission, FYI
			Assert( false );
			break;
		}
		SetDialogVariable( "WinReasonLabel", wzWinReason );

		if ( !bRoundComplete && ( WINREASON_STALEMATE != iWinReason ) && ( WINREASON_STOPWATCH_WATCHING_ROUNDS != iWinReason ) && ( WINREASON_STOPWATCH_WATCHING_FINAL_ROUND != iWinReason ) && ( WINREASON_STOPWATCH_PLAYING_ROUNDS != iWinReason ) )
		{			
			// if this was a mini-round, show # of capture points remaining
			wchar_t wzNumCapturesRemaining[16];
			wchar_t wzCapturesRemainingMsg[256]=L"";
			_snwprintf( wzNumCapturesRemaining, ARRAYSIZE( wzNumCapturesRemaining ), L"%i", iRoundsRemaining );
			g_pVGuiLocalize->ConstructString_safe( wzCapturesRemainingMsg,  
				g_pVGuiLocalize->Find( 1 == iRoundsRemaining ? "#Winpanel_CapturePointRemaining" : "Winpanel_CapturePointsRemaining" ),
				1, wzNumCapturesRemaining );
			SetDialogVariable( "DetailsLabel", wzCapturesRemainingMsg );
		}
		else if ( ( WINREASON_ALL_POINTS_CAPTURED == iWinReason ) || ( WINREASON_FLAG_CAPTURE_LIMIT == iWinReason ) )
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

		// get the current & previous team scores
		int iBlueTeamPrevScore = event->GetInt( "blue_score_prev", 0 );
		int iRedTeamPrevScore = event->GetInt( "red_score_prev", 0 );
		m_iBlueTeamScore = event->GetInt( "blue_score", 0 );
		m_iRedTeamScore = event->GetInt( "red_score", 0 );

		if ( m_pTeamScorePanel )
		{
			m_pTeamScorePanel->SetDialogVariable( "blueteamname", pBlueTeamName );
			m_pTeamScorePanel->SetDialogVariable( "redteamname", pRedTeamName );

			if ( bRoundComplete )
			{
				// set the previous team scores in scoreboard
				m_pTeamScorePanel->SetDialogVariable( "blueteamscore", iBlueTeamPrevScore );
				m_pTeamScorePanel->SetDialogVariable( "redteamscore", iRedTeamPrevScore );

				if ( ( m_iBlueTeamScore != iBlueTeamPrevScore ) || ( m_iRedTeamScore != iRedTeamPrevScore ) )
				{
					// if the new scores are different, set ourselves to update the scoreboard to the new values after a short delay, so players
					// see the scores tick up
					m_flTimeUpdateTeamScore = gpGlobals->curtime + 2.0f;
				}
			}
			// only show team scores if round is complete
			m_pTeamScorePanel->SetVisible( bRoundComplete );
		}

		if ( !g_TF_PR )
			return;

		// look for the top 3 players sent in the event
		for ( int i = 1; i <= 3; i++ )
		{
			bool bShow = false;
			char szPlayerIndexVal[64]="", szPlayerScoreVal[64]="";
			// get player index and round points from the event
			Q_snprintf( szPlayerIndexVal, ARRAYSIZE( szPlayerIndexVal ), "player_%d", i );
			Q_snprintf( szPlayerScoreVal, ARRAYSIZE( szPlayerScoreVal ), "player_%d_points", i );
			int iPlayerIndex = event->GetInt( szPlayerIndexVal, 0 );
			int iRoundScore = event->GetInt( szPlayerScoreVal, 0 );
			// round score of 0 means no player to show for that position (not enough players, or didn't score any points that round)
			if ( iRoundScore > 0 )
				bShow = true;

			CAvatarImagePanel *pPlayerAvatar = dynamic_cast<CAvatarImagePanel *>( FindChildByName( CFmtStr( "Player%dAvatar", i ) ) );
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

			CTFBadgePanel *pBadgePanel = dynamic_cast<CTFBadgePanel *>( FindChildByName( CFmtStr( "Player%dBadge", i ) ) );
			if ( pBadgePanel )
			{
				const IMatchGroupDescription *pMatchDesc = TFGameRules() ? GetMatchGroupDescription( TFGameRules()->GetCurrentMatchGroup() ) : NULL;

				bool bVisible = pMatchDesc && pMatchDesc->m_pProgressionDesc;
				if ( bVisible )
				{
					if ( !bGameOver && TFGameRules()->IsMatchTypeCompetitive() )
					{
						bVisible = false;
					}
				}

				if ( bVisible )
				{
					const CSteamID steamID = GetSteamIDForPlayerIndex( iPlayerIndex );
					if ( steamID.IsValid() )
					{
						pBadgePanel->SetupBadge( pMatchDesc, steamID );
					}
					else
					{
						bVisible = false;
					}
				}

				if ( pBadgePanel->IsVisible() != bVisible )
				{
					pBadgePanel->SetVisible( bVisible );
				}
			}

			vgui::Label *pPlayerName = dynamic_cast<Label *>( FindChildByName( CFmtStr( "Player%dName", i ) ) );
			vgui::Label *pPlayerClass = dynamic_cast<Label *>( FindChildByName( CFmtStr( "Player%dClass", i ) ) );
			vgui::Label *pPlayerScore = dynamic_cast<Label *>( FindChildByName( CFmtStr( "Player%dScore", i ) ) );

			if ( !pPlayerName || !pPlayerClass || !pPlayerScore )
				return;

			if ( bShow )
			{
				// set the player labels to team color
				Color clr = g_PR->GetTeamColor( g_PR->GetTeam( iPlayerIndex ) );				
				pPlayerName->SetFgColor( clr );
				pPlayerClass->SetFgColor( clr );
				pPlayerScore->SetFgColor( clr );

				// set label contents
				pPlayerName->SetText( g_PR->GetPlayerName( iPlayerIndex ) );
				pPlayerClass->SetText( g_aPlayerClassNames[g_TF_PR->GetPlayerClass( iPlayerIndex )] );
				pPlayerScore->SetText( CFmtStr( "%d", iRoundScore ) );

				// send an achievement event
				IGameEvent *pEvent = gameeventmanager->CreateEvent( "player_mvp" );
				if ( pEvent )
				{
					pEvent->SetInt( "player", iPlayerIndex );
					gameeventmanager->FireEventClientSide( pEvent );
				}
			}

			// show or hide labels for this player position
			pPlayerName->SetVisible( bShow );
			pPlayerClass->SetVisible( bShow );
			pPlayerScore->SetVisible( bShow );
		}

		// Top killstreak
		const int nMaxKillStreaks = 1;
		for ( int i = 1; i <= nMaxKillStreaks; ++i )
		{
			char szPlayerIndexVal[64]="", szPlayerScoreVal[64]="";
			Q_snprintf( szPlayerIndexVal, ARRAYSIZE( szPlayerIndexVal ), "killstreak_player_%d", i );
			Q_snprintf( szPlayerScoreVal, ARRAYSIZE( szPlayerScoreVal ), "killstreak_player_%d_count", i );
			int iPlayerIndex = event->GetInt( szPlayerIndexVal, 0 );
			int iCount = event->GetInt( szPlayerScoreVal, 0 );
			
			vgui::Label *pKillStreakPlayerName = dynamic_cast<Label *>( FindChildByName( CFmtStr( "KillStreakPlayer%dName", i ) ) );
			vgui::Label *pKillStreakPlayerClass = dynamic_cast<Label *>( FindChildByName( CFmtStr( "KillStreakPlayer%dClass", i ) ) );
			vgui::Label *pKillStreakPlayerScore = dynamic_cast<Label *>( FindChildByName( CFmtStr( "KillStreakPlayer%dScore", i ) ) );
			if ( !pKillStreakPlayerName || !pKillStreakPlayerClass || !pKillStreakPlayerScore )
				continue;

			bool bShow = iCount > 0;
			if ( bShow )
			{
				CAvatarImagePanel *pPlayerAvatar = dynamic_cast<CAvatarImagePanel *>( FindChildByName( CFmtStr( "KillStreakPlayer%dAvatar", i ) ) );
				if ( pPlayerAvatar )
				{
					pPlayerAvatar->SetShouldScaleImage( true );
					pPlayerAvatar->SetShouldDrawFriendIcon( false );

					CBasePlayer *pPlayer = UTIL_PlayerByIndex( iPlayerIndex );
					pPlayerAvatar->SetPlayer( pPlayer );
					pPlayerAvatar->SetVisible( true );
				}

				// set the player labels to team color
				Color clr = g_PR->GetTeamColor( g_PR->GetTeam( iPlayerIndex ) );				
				pKillStreakPlayerName->SetFgColor( clr );
				pKillStreakPlayerClass->SetFgColor( clr );
				pKillStreakPlayerScore->SetFgColor( clr );

				// set label contents
				pKillStreakPlayerName->SetText( g_PR->GetPlayerName( iPlayerIndex ) );
				pKillStreakPlayerClass->SetText( g_aPlayerClassNames[g_TF_PR->GetPlayerClass( iPlayerIndex )] );
				pKillStreakPlayerScore->SetText( CFmtStr( "%d", iCount ) );
			}

			CTFBadgePanel *pBadgePanel = dynamic_cast<CTFBadgePanel *>( FindChildByName( CFmtStr( "KillStreakPlayer%dBadge", i ) ) );
			if ( pBadgePanel )
			{
				const IMatchGroupDescription *pMatchDesc = GetMatchGroupDescription( TFGameRules()->GetCurrentMatchGroup() );

				bool bVisible = ( bShow && pMatchDesc );
				if ( bVisible )
				{
					const CSteamID steamID = GetSteamIDForPlayerIndex( iPlayerIndex );
					if ( steamID.IsValid() )
					{
						pBadgePanel->SetupBadge( pMatchDesc, steamID );
					}
					else
					{
						bVisible = false;
					}
				}

				if ( pBadgePanel->IsVisible() != bVisible )
				{
					pBadgePanel->SetVisible( bVisible );
				}
			}

			// show or hide labels for this player position
			pKillStreakPlayerName->SetVisible( bShow );
			pKillStreakPlayerClass->SetVisible( bShow );
			pKillStreakPlayerScore->SetVisible( bShow );
		}

		UpdateTeamInfo();

		m_bShouldBeVisible = true;

		MoveToFront();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWinPanel::UpdateTeamInfo()
{
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
	m_pRedTeamName->SetVisible( !bShowAvatars );

	m_pBlueLeaderAvatarImage->SetVisible( bShowAvatars );
	m_pBlueLeaderAvatarBG->SetVisible( bShowAvatars );
	m_pBlueTeamName->SetVisible( !bShowAvatars );
}

//-----------------------------------------------------------------------------
// Purpose: Applies scheme settings
//-----------------------------------------------------------------------------
void CTFWinPanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );
}

//-----------------------------------------------------------------------------
// Purpose: returns whether panel should be drawn
//-----------------------------------------------------------------------------
bool CTFWinPanel::ShouldDraw()
{
	if ( !m_bShouldBeVisible )
		return false;

	return CHudElement::ShouldDraw();
}

//-----------------------------------------------------------------------------
// Purpose: panel think method
//-----------------------------------------------------------------------------
void CTFWinPanel::OnThink()
{
	// if we've scheduled ourselves to update the team scores, handle it now
	if ( m_flTimeUpdateTeamScore > 0 && ( gpGlobals->curtime > 	m_flTimeUpdateTeamScore ) && m_pTeamScorePanel )
	{
		IGameEvent *event = gameeventmanager->CreateEvent( "winpanel_show_scores" );
		if ( event )
		{
			gameeventmanager->FireEventClientSide( event );
		}

		// play a sound
		CLocalPlayerFilter filter;
		C_BaseEntity::EmitSound( filter, SOUND_FROM_LOCAL_PLAYER, "Hud.EndRoundScored" );

		// update the team scores
		if ( m_pTeamScorePanel )
		{
			m_pTeamScorePanel->SetDialogVariable( "blueteamscore", m_iBlueTeamScore );
			m_pTeamScorePanel->SetDialogVariable( "redteamscore", m_iRedTeamScore );
		}
		m_flTimeUpdateTeamScore = 0;
	}
}
