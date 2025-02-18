//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "c_tf_player.h"
#include "iclientmode.h"
#include "ienginevgui.h"
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <vgui/IVGui.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/EditablePanel.h>
#include "tf_imagepanel.h"
#include "tf_gamerules.h"
#include "c_tf_team.h"
#include "c_team_objectiveresource.h"
#include "tf_hud_match_status.h"
#include "tf_hud_teamgoal_tournament.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

void AddSubKeyNamed( KeyValues *pKeys, const char *pszName );

DECLARE_HUDELEMENT( CHudTeamGoalTournament );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudTeamGoalTournament::CHudTeamGoalTournament( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudTeamGoalTournament" )
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	SetHiddenBits( HIDEHUD_MISCSTATUS );

	m_flShowAt = -1.f;

	RegisterForRenderGroup( "commentary" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudTeamGoalTournament::LevelInit( void )
{
	m_flShowAt = -1.f;

	ListenForGameEvent( "tournament_stateupdate" );
	ListenForGameEvent( "teamplay_round_start" );
	ListenForGameEvent( "competitive_state_changed" );
}
C_TFTeam *GetTeamRoles( int iTeamRole )
{
	for ( int i = LAST_SHARED_TEAM+1; i < GetNumberOfTeams(); i++ )
	{
		C_TFTeam *pTeam = GetGlobalTFTeam( i );

		if ( pTeam )
		{
			if ( pTeam->GetRole() == iTeamRole )
			{
				return pTeam;
			}
		}
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CHudTeamGoalTournament::HudElementKeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding )
{
	if ( IsVisible() && TFGameRules() && TFGameRules()->IsInTournamentMode() && TFGameRules()->IsInStopWatch() )
	{
		if ( ( TFGameRules()->State_Get() == GR_STATE_PREROUND ) || TFGameRules()->InSetup() )
		{
			if ( down && ( keynum == KEY_F1 ) )
			{
				m_flShowAt = -1.f;
				return 0;
			}
		}
	}

	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudTeamGoalTournament::FireGameEvent( IGameEvent * event )
{
	const char *pEventName = event->GetName();

	if ( FStrEq( "tournament_stateupdate", pEventName ) )
	{
		m_flShowAt = -1.f;
	}
	else if ( FStrEq( "teamplay_round_start", pEventName ) )
	{
		if ( TFGameRules() && TFGameRules()->IsInTournamentMode() && TFGameRules()->IsInStopWatch() )
		{
			C_TFTeam *pAttacker = GetTeamRoles( TEAM_ROLE_ATTACKERS );
			C_TFTeam *pDefender = GetTeamRoles( TEAM_ROLE_DEFENDERS );

			if( !pAttacker || !pDefender )
				return;

			if ( ( pAttacker->Get_Score() != 0 ) && ( pDefender->Get_Score() != 0 ) )
				return;
		
			m_flShowAt = gpGlobals->curtime + 1.f;
		}
	}
	else if ( FStrEq( "competitive_state_changed", pEventName ) )
	{
		InvalidateLayout( false, true );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudTeamGoalTournament::ApplySchemeSettings( IScheme *pScheme )
{
	KeyValues *pConditions = NULL;
	if ( ShouldUseMatchHUD() )
	{
		pConditions = new KeyValues( "conditions" );
		AddSubKeyNamed( pConditions, "if_comp" );
	}

	// load control settings...
	LoadControlSettings( "resource/UI/HudTeamGoalTournament.res", NULL, NULL, pConditions );

	BaseClass::ApplySchemeSettings( pScheme );

	if ( pConditions )
	{
		pConditions->deleteThis();
	}

	m_cRegularColor = pScheme->GetColor( "TanLight", Color( 0, 0, 0 ) );
	m_cHighlightColor = pScheme->GetColor( "GoalOrange", Color( 0, 0, 0 ) );

	m_pStopWatchGoal = dynamic_cast<EditablePanel *>( FindChildByName( "HudStopWatchObjective" ) );

	if ( m_pStopWatchGoal )
	{
		m_pStopWatchGoalText = dynamic_cast<CExRichText *>( m_pStopWatchGoal->FindChildByName( "HudStopWatchObjectiveText1" ) );
	}

	if ( m_pStopWatchGoalText )
	{
		m_pStopWatchGoalText->SetVisible( true );
		m_pStopWatchGoalText->InsertColorChange( m_cRegularColor );
	}

	if ( m_pStopWatchGoal )
	{
		m_pStopWatchGoalText2 = dynamic_cast<CExRichText *>( m_pStopWatchGoal->FindChildByName( "HudStopWatchObjectiveText2" ) );
	}

	if ( m_pStopWatchGoalText2 )
	{
		m_pStopWatchGoalText2->SetVisible( true );
		m_pStopWatchGoalText2->InsertColorChange( m_cRegularColor );
	}

	if ( m_pStopWatchGoal )
	{
		m_pStopWatchGoalBGLarge = m_pStopWatchGoal->FindChildByName( "HudStopWatchObjectiveBG" );
		m_pStopWatchGoalBGSmall = m_pStopWatchGoal->FindChildByName( "HudStopWatchObjectiveBGSmall" );
		m_pStopWatchGoalDivider = m_pStopWatchGoal->FindChildByName( "HudStopWatchObjectiveShadedBar" );
	}

	m_pStopWatchGoalArrow = FindChildByName( "HudStopWatchObjectiveArrow" );
}
inline wchar_t *CloneWString( const wchar_t *str );

enum
{
	STOPWATCH_COLOR_NORMAL = 1,
	STOPWATCH_COLOR_SUB = 3,
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudTeamGoalTournament::SetVisible( bool bState )
{
	if ( bState )
	{
		SetupStopWatchLabel();
	}

	BaseClass::SetVisible( bState );
}

//-----------------------------------------------------------------------------
// Purpose: Prepares the string with the different colors
// This is kinda nasty since I just copied this code from the chat
// This should be abstracted and made all nice.
//-----------------------------------------------------------------------------
void CHudTeamGoalTournament::PrepareStopWatchString( wchar_t *pszString, CExRichText *pText )
{
	if ( m_text )
	{
		delete[] m_text;
		m_text = NULL;
	}

	m_textRanges.RemoveAll();

	m_text = CloneWString( pszString );

	wchar_t *txt = m_text;
	int lineLen = wcslen( m_text );

	TextRange range;
	range.start = 0;
	range.end = wcslen( m_text );
	range.color = Color( 235, 226, 202 );
	m_textRanges.AddToTail( range );

	while ( txt && *txt )
	{
		switch ( *txt )
		{
		case STOPWATCH_COLOR_SUB:
		case STOPWATCH_COLOR_NORMAL:
			{
				// save this start
				range.start = (txt-m_text) + 1;

				if ( *txt == STOPWATCH_COLOR_NORMAL )
					range.color = m_cRegularColor;
				else
					range.color = m_cHighlightColor;

				range.end = lineLen;

				int count = m_textRanges.Count();
				if ( count )
				{
					m_textRanges[count-1].end = range.start - 1;
				}

				m_textRanges.AddToTail( range );
			}
			++txt;
			break;

		default:
			++txt;
		}
	}

	for ( int i=0; i<m_textRanges.Count(); ++i )
	{
		wchar_t * start = m_text + m_textRanges[i].start;
		if ( *start > 0 && *start < COLOR_MAX )
		{
			m_textRanges[i].start += 1;
		}
	}

	wchar_t wText[4096];
	Color color;
	for ( int i=0; i<m_textRanges.Count(); ++i )
	{
		wchar_t * start = m_text + m_textRanges[i].start;
		int len = m_textRanges[i].end - m_textRanges[i].start + 1;
		if ( len > 1 )
		{
			wcsncpy( wText, start, len );
			wText[len-1] = 0;
			color = m_textRanges[i].color;
			color[3] = 255;
			pText->InsertColorChange( color );
			pText->InsertString( wText );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudTeamGoalTournament::SetupStopWatchLabel( void )
{
	int iPoints = 0;

	if ( !TFGameRules() )
		return;

	if ( !ObjectiveResource() )
		return;

	C_TFTeam *pAttacker = GetTeamRoles( TEAM_ROLE_ATTACKERS );
	C_TFTeam *pDefender = GetTeamRoles( TEAM_ROLE_DEFENDERS );

	if( !pAttacker || !pDefender )
		return;

	int iActiveTimer = ObjectiveResource()->GetStopWatchTimer();

	CTeamRoundTimer *pTimer = dynamic_cast< CTeamRoundTimer* >( ClientEntityList().GetEnt( iActiveTimer ) );

	if ( !pTimer )
		return;
		
	if ( !pTimer->IsWatchingTimeStamps() )
	{
		iPoints = pDefender->Get_Score();

		if ( m_pStopWatchGoal )
		{
			m_pStopWatchGoal->SetVisible( true );
		}

		if ( m_pStopWatchGoalArrow )
		{
			m_pStopWatchGoalArrow->SetVisible( true );
		}
	}
	else
	{
		if ( m_pStopWatchGoal )
		{
			m_pStopWatchGoal->SetVisible( false );
		}
		
		if ( m_pStopWatchGoalArrow )
		{
			m_pStopWatchGoalArrow->SetVisible( false );
		}

		return;
	}

	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

	if ( !pLocalPlayer )
		return;

	wchar_t wzHelp[256];

	C_TFTeam *pBlueTeam = GetGlobalTFTeam( TF_TEAM_BLUE );
	const wchar_t *pBlueTeamName = pBlueTeam ? pBlueTeam->Get_Localized_Name() : L"BLU";

	C_TFTeam *pRedTeam = GetGlobalTFTeam( TF_TEAM_RED );
	const wchar_t *pRedTeamName = pRedTeam ? pRedTeam->Get_Localized_Name() : L"RED";

	wchar_t wszAttackersName[MAX_TEAM_NAME_LENGTH];
	wchar_t wszDefendersName[MAX_TEAM_NAME_LENGTH];

	V_wcscpy_safe( wszAttackersName, ( pAttacker->GetTeamNumber() == TF_TEAM_BLUE ) ? pBlueTeamName : pRedTeamName ); 
	V_wcscpy_safe( wszDefendersName, ( pDefender->GetTeamNumber() == TF_TEAM_BLUE ) ? pBlueTeamName : pRedTeamName );

#ifdef WIN32
#define INT_CHAR_FMT L"%d %s"
#else
#define INT_CHAR_FMT L"%d %S"	
#endif
	
	wchar_t wszPoints[256];
	_snwprintf( wszPoints, ARRAYSIZE( wszPoints ), INT_CHAR_FMT, iPoints, iPoints == 1 ? g_pVGuiLocalize->Find( "Tournament_StopWatch_Point" ) : g_pVGuiLocalize->Find( "Tournament_StopWatch_Points" ) );

	wchar_t wszTime[256];
	wchar_t wszLabel[256];

	int iMinutes = pTimer->GetTimeRemaining() / 60;
	int iSeconds = ((int)pTimer->GetTimeRemaining()) % 60;

	if ( iMinutes > 0 )
	{
#ifdef WIN32
		_snwprintf( wszTime, ARRAYSIZE( wszTime ) , L"%d %s %d %s", iMinutes, iMinutes == 1 ? g_pVGuiLocalize->Find( "Tournament_WinConditionsMinute" ) : g_pVGuiLocalize->Find( "Tournament_WinConditionsMinutes" ), iSeconds, iSeconds == 1 ? g_pVGuiLocalize->Find( "Tournament_WinConditionsSecond" ) : g_pVGuiLocalize->Find( "Tournament_WinConditionsSeconds" ) );
#else
		_snwprintf( wszTime, ARRAYSIZE( wszTime ) , L"%d %S %d %S", iMinutes, iMinutes == 1 ? g_pVGuiLocalize->Find( "Tournament_WinConditionsMinute" ) : g_pVGuiLocalize->Find( "Tournament_WinConditionsMinutes" ), iSeconds, iSeconds == 1 ? g_pVGuiLocalize->Find( "Tournament_WinConditionsSecond" ) : g_pVGuiLocalize->Find( "Tournament_WinConditionsSeconds" ) );
#endif
	}
	else
	{
		_snwprintf( wszTime, ARRAYSIZE( wszTime ), INT_CHAR_FMT, iSeconds, iSeconds == 1 ? g_pVGuiLocalize->Find( "Tournament_WinConditionsSecond" ) : g_pVGuiLocalize->Find( "Tournament_WinConditionsSeconds" ) );
	}

	if ( m_pStopWatchGoalText )
	{
		m_pStopWatchGoalText->SetText("");
	}

	if ( m_pStopWatchGoalText2 )
	{
		m_pStopWatchGoalText2->SetText("");
	}

	if ( m_pStopWatchGoal )
	{
		m_pStopWatchGoal->SetDialogVariable( "objectivelabel", "" );	
	}

	if ( pLocalPlayer->GetTeam() == pDefender )
	{
		g_pVGuiLocalize->ConstructString_safe( wszLabel, g_pVGuiLocalize->Find( "Tournament_StopWatch_LabelDefender" ), 1, wszAttackersName );

		if ( m_pStopWatchGoal )
		{
			m_pStopWatchGoal->SetDialogVariable( "objectivelabel", wszLabel );	
		}
	}
	
	//Attackers capped something last round case
	if ( iPoints > 0 )
	{
		bool bCappedAllPoints = ( iPoints == ObjectiveResource()->GetNumControlPoints() );

		g_pVGuiLocalize->ConstructString_safe( wzHelp, g_pVGuiLocalize->Find( bCappedAllPoints ? "Tournament_StopWatch_GoalTextPointsAndTimeAndClose" : "Tournament_StopWatch_GoalTextPointsAndTime" ), 5, wszDefendersName, wszPoints, wszTime, wszAttackersName, wszPoints );

		if ( m_pStopWatchGoalText )
		{
			PrepareStopWatchString( wzHelp, m_pStopWatchGoalText );
		}

		if ( bCappedAllPoints )
		{
			if ( m_pStopWatchGoalText2 )
			{
				m_pStopWatchGoalText2->SetVisible( false );
			}

			if ( m_pStopWatchGoalBGLarge )
			{
				m_pStopWatchGoalBGLarge->SetVisible( false );
			}

			if ( m_pStopWatchGoalBGSmall )
			{
				m_pStopWatchGoalBGSmall->SetVisible( true );
			}

			if ( m_pStopWatchGoalDivider )
			{
				m_pStopWatchGoalDivider->SetVisible( false );
			}
		}
		else
		{
			if ( m_pStopWatchGoalText2 )
			{
				m_pStopWatchGoalText2->SetVisible( true );
			}

			if ( m_pStopWatchGoalBGLarge )
			{
				m_pStopWatchGoalBGLarge->SetVisible( true );
			}

			if ( m_pStopWatchGoalBGSmall )
			{
				m_pStopWatchGoalBGSmall->SetVisible( false );
			}

			if ( m_pStopWatchGoalDivider )
			{
				m_pStopWatchGoalDivider->SetVisible( true );
			}

			iPoints += 1;
			_snwprintf( wszPoints, ARRAYSIZE( wszPoints ), INT_CHAR_FMT, iPoints, iPoints == 1 ? g_pVGuiLocalize->Find( "Tournament_StopWatch_Point" ) : g_pVGuiLocalize->Find( "Tournament_StopWatch_Points" ) );
			g_pVGuiLocalize->ConstructString_safe( wzHelp, g_pVGuiLocalize->Find( "Tournament_StopWatch_GoalTextPointsAndTime2" ), 4, wszAttackersName, wszDefendersName, wszAttackersName, wszPoints );

			if ( m_pStopWatchGoalText2 )
			{
				PrepareStopWatchString( wzHelp, m_pStopWatchGoalText2 );
			}
		}

		if ( pLocalPlayer->GetTeam() == pAttacker )
		{
			g_pVGuiLocalize->ConstructString_safe( wszLabel, g_pVGuiLocalize->Find( "Tournament_StopWatch_TimeVictory" ), 1, wszDefendersName );

			if ( m_pStopWatchGoal )
			{
				m_pStopWatchGoal->SetDialogVariable( "objectivelabel", wszLabel );	
			}
		}
	}
	else
	{
		if ( m_pStopWatchGoalText2 )
		{
			m_pStopWatchGoalText2->SetVisible( false );
		}

		if ( m_pStopWatchGoalBGLarge )
		{
			m_pStopWatchGoalBGLarge->SetVisible( false );
		}

		if ( m_pStopWatchGoalBGSmall )
		{
			m_pStopWatchGoalBGSmall->SetVisible( true );
		}

		if ( m_pStopWatchGoalDivider )
		{
			m_pStopWatchGoalDivider->SetVisible( false );
		}

		g_pVGuiLocalize->ConstructString_safe( wzHelp, g_pVGuiLocalize->Find( "Tournament_StopWatch_GoalTextPoints" ), 4, wszDefendersName, wszAttackersName );


		if ( m_pStopWatchGoalText )
		{
			PrepareStopWatchString( wzHelp, m_pStopWatchGoalText );
		}

		if ( pLocalPlayer->GetTeam() == pAttacker )
		{
			g_pVGuiLocalize->ConstructString_safe( wszLabel, g_pVGuiLocalize->Find( "Tournament_StopWatch_AttackerScore" ), 1, wszDefendersName );

			if ( m_pStopWatchGoal )
			{
				m_pStopWatchGoal->SetDialogVariable( "objectivelabel", wszLabel );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHudTeamGoalTournament::ShouldDraw( void )
{
	if ( !TFGameRules() || 
		 !TFGameRules()->IsInTournamentMode() || 
		 !TFGameRules()->IsInStopWatch() ||
		 ( ( TFGameRules()->State_Get() != GR_STATE_PREROUND ) && !TFGameRules()->InSetup() ) )
	{
		m_flShowAt = -1.f;
		return false;
	}
	
	if ( ObjectiveResource() )
	{
		int iActiveTimer = ObjectiveResource()->GetStopWatchTimer();
		CTeamRoundTimer *pTimer = dynamic_cast<CTeamRoundTimer*>( ClientEntityList().GetEnt( iActiveTimer ) );
		if ( pTimer && pTimer->IsWatchingTimeStamps() )
		{
			m_flShowAt = -1.f;
			return false;
		}
	}

	if ( ( m_flShowAt < 0 ) || ( m_flShowAt > gpGlobals->curtime ) )
	{
		return false;
	}

	// only show the panel for 15 seconds after it is first draws
	if ( gpGlobals->curtime - m_flShowAt > 15.f )
	{
		m_flShowAt = -1.f;
		return false;
	}

	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( pPlayer && pPlayer->IsAlive() && ( pPlayer->GetTeamNumber() >= FIRST_GAME_TEAM ) )
	{
		if ( CHudElement::ShouldDraw() )
		{
			CHudElement *pHudSwitch = gHUD.FindElement( "CHudTeamSwitch" );
			if ( pHudSwitch && pHudSwitch->ShouldDraw() )
				return false;
			
			return true;
		}
	}

	return false;
}
