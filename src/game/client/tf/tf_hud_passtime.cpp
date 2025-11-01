//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "c_func_passtime_goal.h"
#include "c_tf_passtime_ball.h"
#include "c_tf_passtime_logic.h"
#include "tf_hud_passtime.h"
#include "tf_hud_passtime_ball_offscreen_arrow.h"
#include "tf_weapon_passtime_gun.h"
#include "passtime_convars.h"
#include "passtime_game_events.h"
#include "tf_hud_freezepanel.h"
#include "tf_gamerules.h"
#include "c_tf_team.h"
#include "c_tf_player.h"
#include "c_tf_playerresource.h"
#include "iclientmode.h"
#include "vgui_controls/AnimationController.h"
#include "vgui_controls/CircularProgressBar.h"
#include "vgui_controls/ProgressBar.h"
#include "vgui/ISurface.h"
#include <algorithm>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
using namespace vgui;

//-----------------------------------------------------------------------------
// The team colors from g_PR are wrong for probably good reasons that I don't understand.
static Color GetTeamColor( int iTeam, byte alpha = 255 )
{
	switch ( iTeam )
	{
	case TF_TEAM_RED:	return Color( 159, 55, 34, alpha );
	case TF_TEAM_BLUE:	return Color( 76, 109, 128, alpha );
	default:			return Color( 245, 231, 222, alpha );
	}
}

//-----------------------------------------------------------------------------
static const char *GetProgressBallImageForTeam( int iTeam )
{
	switch( iTeam )
	{
	case TF_TEAM_RED: return "../passtime/hud/passtime_ballcontrol_red";
	case TF_TEAM_BLUE: return "../passtime/hud/passtime_ballcontrol_blue";
	default: return "../passtime/hud/passtime_ballcontrol_none";
	};
}
static const char *GetProgressBallImageForTeam( C_BaseEntity *pEnt )
{
	if ( !pEnt ) 
	{
		return "../passtime/hud/passtime_ball";
	}
	return GetProgressBallImageForTeam( pEnt->GetTeamNumber() );
}

//-----------------------------------------------------------------------------
static const char *GetPlayerProgressPortrait( C_TFPlayer *pPlayer )
{
	if ( !pPlayer )
	{
		return "../passtime/hud/portrait_scout_red";
	}

	int iTeam = pPlayer->GetTeamNumber();
	int iClass = pPlayer->GetPlayerClass()->GetClassIndex();

	switch(iClass)
	{
		case TF_CLASS_SOLDIER:
			return (iTeam == TF_TEAM_RED) ? "../passtime/hud/portrait_soldier_red"	: "../passtime/hud/portrait_soldier_blu";
		case TF_CLASS_SCOUT:
			return (iTeam == TF_TEAM_RED) ? "../passtime/hud/portrait_scout_red"		: "../passtime/hud/portrait_scout_blu";
		case TF_CLASS_SNIPER:
			return (iTeam == TF_TEAM_RED) ? "../passtime/hud/portrait_sniper_red"		: "../passtime/hud/portrait_sniper_blu";
		case TF_CLASS_DEMOMAN:
			return (iTeam == TF_TEAM_RED) ? "../passtime/hud/portrait_demo_red"		: "../passtime/hud/portrait_demo_blu";
		case TF_CLASS_MEDIC:
			return (iTeam == TF_TEAM_RED) ? "../passtime/hud/portrait_medic_red"		: "../passtime/hud/portrait_medic_blu";
		case TF_CLASS_HEAVYWEAPONS:
			return (iTeam == TF_TEAM_RED) ? "../passtime/hud/portrait_heavy_red"		: "../passtime/hud/portrait_heavy_blu";
		case TF_CLASS_PYRO:
			return (iTeam == TF_TEAM_RED) ? "../passtime/hud/portrait_pyro_red"		: "../passtime/hud/portrait_pyro_blu";
		case TF_CLASS_SPY:
			return (iTeam == TF_TEAM_RED) ? "../passtime/hud/portrait_spy_red"		: "../passtime/hud/portrait_spy_blu";
		case TF_CLASS_ENGINEER:
			return (iTeam == TF_TEAM_RED) ? "../passtime/hud/portrait_eng_red"		: "../passtime/hud/portrait_eng_blu";
		default:
			return (iTeam == TF_TEAM_RED) ? "../passtime/hud/portrait_scout_red"		: "../passtime/hud/portrait_scout_blu";
	}
}

//-----------------------------------------------------------------------------
static const char *GetProgressGoalImage( const C_FuncPasstimeGoal *pGoal )
{
	if ( !pGoal ) 
	{
		return "";
	}

	// NOTE: keep in mind that a goal that's on team blue is the goal where blue scores
	// and should look red on the hud since it's in the red part of the map. oops.
	bool bRedIcon = pGoal->GetTeamNumber() == TF_TEAM_BLUE;

	if ( pGoal->GetGoalType() == C_FuncPasstimeGoal::TYPE_TOWER )
	{
		if ( pGoal->BGoalTriggerDisabled() )
		{
			return bRedIcon
				? "../passtime/hud/passtime_goal_red_locked"
				: "../passtime/hud/passtime_goal_blue_locked";
		}
		else
		{
			return bRedIcon
				? "../passtime/hud/passtime_goal_red_unlocked"
				: "../passtime/hud/passtime_goal_blue_unlocked";
		}
	}
	else if ( pGoal->GetGoalType() == C_FuncPasstimeGoal::TYPE_ENDZONE )
	{
		return bRedIcon
			? "../passtime/hud/passtime_endzone_red_icon"
			: "../passtime/hud/passtime_endzone_blue_icon";
	}
	else
	{
		return bRedIcon
			? "../passtime/hud/passtime_goal_red_icon"
			: "../passtime/hud/passtime_goal_blue_icon";
	}
}

//-----------------------------------------------------------------------------
// CTFHudPasstimePanel
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
CTFHudPasstimePanel::CTFHudPasstimePanel( vgui::Panel *pParent, const char* name )
	: EditablePanel( pParent, name )
{}

//-----------------------------------------------------------------------------
bool CTFHudPasstimePanel::IsVisible()
{
	if ( IsTakingAFreezecamScreenshot() )
	{
		return false;
	}
	return BaseClass::IsVisible();
}


//-----------------------------------------------------------------------------
// CTFHudTeamScore
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
CTFHudTeamScore::CTFHudTeamScore( vgui::Panel *pParent ) 
	: CTFHudPasstimePanel( pParent, "HudTeamScore" )
	, m_pPlayingToCluster( 0 )
{
}

//-----------------------------------------------------------------------------
void CTFHudTeamScore::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );
	LoadControlSettings( "resource/UI/HudPasstimeTeamScore.res", NULL, NULL, NULL );
	m_pPlayingToCluster = FindControl<EditablePanel>( "PlayingToCluster" );

	if ( m_pPlayingToCluster )
	{
		m_pPlayingToCluster->SetVisible( true );
	}

	OnTick();
	vgui::ivgui()->AddTickSignal( GetVPanel() );
}

//-----------------------------------------------------------------------------
void CTFHudTeamScore::OnTick()
{
	// I would rather not do this every tick, but i couldn't find a reliable way
	// to do it from events.
	if( !g_pPasstimeLogic )
	{
		return;
	}

	int iBlueScore = 0;
	int iRedScore = 0;

	C_TFTeam *pTeam = GetGlobalTFTeam( TF_TEAM_BLUE );
	if ( pTeam )
	{
		iBlueScore = pTeam->GetFlagCaptures();
	}

	pTeam = GetGlobalTFTeam( TF_TEAM_RED );
	if ( pTeam )
	{
		iRedScore = pTeam->GetFlagCaptures();
	}

	if ( m_pPlayingToCluster )
	{
		m_pPlayingToCluster->SetDialogVariable( "rounds", 
			tf_passtime_scores_per_round.GetInt() );
	}

	SetDialogVariable( "bluescore", iBlueScore );
	SetDialogVariable( "redscore", iRedScore );
}

//-----------------------------------------------------------------------------
int CTFHudTeamScore::GetTeamScore( int iTeam )
{
	C_TFTeam *pTeam = GetGlobalTFTeam( iTeam );
	return pTeam 
		? pTeam->Get_Score()
		: 0;
}

//-----------------------------------------------------------------------------
// CTFHudPasstimePassNotify
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
CTFHudPasstimePassNotify::CTFHudPasstimePassNotify( vgui::Panel *pParent ) 
	: CTFHudPasstimePanel( pParent, "HudPasstimePassNotify" )
{
}

//-----------------------------------------------------------------------------
void CTFHudPasstimePassNotify::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );
	LoadControlSettings( "resource/UI/HudPasstimePassNotify.res", NULL, NULL, NULL );

	m_pTextBox = FindControl<EditablePanel>( "TextBox" );
	m_pTextInPassRange = m_pTextBox ? m_pTextBox->FindControl<Label>( "TextInPassRange" ) : NULL;
	m_pTextLockedOn = m_pTextBox ? m_pTextBox->FindControl<Label>( "TextLockedOn" ) : NULL;
	m_pTextPassIncoming = m_pTextBox ? m_pTextBox->FindControl<Label>( "TextPassIncoming" ) : NULL;
	m_pTextPlayerName = m_pTextBox ? m_pTextBox->FindControl<Label>( "TextPlayerName" ) : NULL;
	m_pSpeechIndicator = FindControl<ImagePanel>( "SpeechIndicator" );
	m_pPassLockIndicator = FindControl<ImagePanel>( "PassLockIndicator" );
	m_pTextBoxBorderNormal = pScheme->GetBorder( "TFFatLineBorder" );
	m_pTextBoxBorderIncomingRed = pScheme->GetBorder( "TFFatLineBorderRedBG" );
	m_pTextBoxBorderIncomingBlu = pScheme->GetBorder( "TFFatLineBorderBlueBG" );
	vgui::ivgui()->AddTickSignal( GetVPanel() );

	Assert( m_pTextInPassRange && m_pTextLockedOn && m_pTextPassIncoming && m_pTextPlayerName
		&& m_pSpeechIndicator && m_pPassLockIndicator );
}

//-----------------------------------------------------------------------------
void CTFHudPasstimePassNotify::OnTick()
{
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pLocalPlayer || !g_pPasstimeLogic || !g_pPasstimeLogic->GetBall() ) 
	{
		// nothing can work
		if ( m_pTextBox )
		{
			m_pTextBox->SetVisible( false );
		}

		if ( m_pSpeechIndicator )
		{
			m_pSpeechIndicator->SetVisible( false );
		}

		if ( m_pPassLockIndicator )
		{
			m_pPassLockIndicator->SetVisible( false );
		}
		return;
	}

	C_PasstimeBall *pBall = g_pPasstimeLogic->GetBall();
	CTFPlayer *pBallCarrier = pBall ? pBall->GetCarrier() : NULL;
	if ( !pBallCarrier ) 
	{
		if ( pBall && pBall->GetHomingTarget() == pLocalPlayer )
		{
			//
			// Incoming pass
			//
			if ( m_pSpeechIndicator )
			{
				m_pSpeechIndicator->SetVisible( false );
			}

			if ( m_pPassLockIndicator )
			{
				m_pPassLockIndicator->SetVisible( false );
			}

			if ( m_pTextBox )
			{
				m_pTextBox->SetVisible( true );
			}

			// this should really be GetThrower instead of PrevCarrier, 
			// but it doesn't exist on the client and this will have the
			// desired value anyway.
			C_TFPlayer *pThrower = pBall->GetPrevCarrier(); 
			if ( pThrower )
			{
				if ( m_pTextPlayerName )
				{
					m_pTextPlayerName->SetText( pThrower->GetPlayerName() );
				}
			}
			else
			{
				if ( m_pTextPlayerName )
				{
					m_pTextPlayerName->SetText( "..." );
				}
			}

			if ( m_pTextLockedOn )
			{
				m_pTextLockedOn->SetVisible( false );
			}

			if ( m_pTextPassIncoming )
			{
				m_pTextPassIncoming->SetVisible( true );
			}

			if ( m_pTextInPassRange )
			{
				m_pTextInPassRange->SetVisible( false );
			}

			if ( m_pTextBox )
			{
				m_pTextBox->SetBorder( (pLocalPlayer->GetTeamNumber() == TF_TEAM_RED)
					? m_pTextBoxBorderIncomingRed 
					: m_pTextBoxBorderIncomingBlu );
			}

			return;
		}
	}

	//
	// Can't be an incoming pass at this point
	//

	if( !pBallCarrier 
		|| pLocalPlayer->IsObserver() 
		|| (pBallCarrier == pLocalPlayer) 
		|| (pBallCarrier->GetTeamNumber() != pLocalPlayer->GetTeamNumber()) )
	{
		// 
		// No carrier, or carrier is on enemy team, or carrier is local player
		//

		if ( m_pTextBox )
		{
			m_pTextBox->SetVisible( false );
		}

		if ( m_pSpeechIndicator )
		{
			m_pSpeechIndicator->SetVisible( false );
		}

		if ( m_pPassLockIndicator )
		{
			m_pPassLockIndicator->SetVisible( false );
		}
		return;
	}

	//
	// Carrier has the ball and is on our team
	//

	float flMaxPassRangeSqr = g_pPasstimeLogic->GetMaxPassRange();
	flMaxPassRangeSqr *= flMaxPassRangeSqr;
	bool bTargetable = pBallCarrier->EyePosition().DistToSqr( pLocalPlayer->EyePosition() ) < flMaxPassRangeSqr;
	if ( bTargetable )
	{
		trace_t	tr;
		CTraceFilterIgnorePlayers tracefilter( pLocalPlayer, COLLISION_GROUP_PROJECTILE );
		UTIL_TraceLine( pBallCarrier->EyePosition(), pLocalPlayer->EyePosition(), MASK_PLAYERSOLID, &tracefilter, &tr );
		bTargetable = tr.fraction == 1;
	}

	if ( !bTargetable ) 
	{
		if ( m_pTextBox )
		{
			m_pTextBox->SetVisible( false );
		}

		if ( m_pSpeechIndicator )
		{
			m_pSpeechIndicator->SetVisible( false );
		}

		if ( m_pPassLockIndicator )
		{
			m_pPassLockIndicator->SetVisible( false );
		}
		return;
	}

	bool bTargeted = pLocalPlayer->m_Shared.IsTargetedForPasstimePass();

	if ( m_pTextBox )
	{
		m_pTextBox->SetVisible( true );
	}

	if ( m_pPassLockIndicator )
	{
		m_pPassLockIndicator->SetVisible( bTargeted );
	}

	if ( m_pSpeechIndicator )
	{
		m_pSpeechIndicator->SetVisible( pLocalPlayer->m_Shared.AskForBallTime() > gpGlobals->curtime );
	}

	if ( bTargeted ) 
	{
		if ( m_pPassLockIndicator )
		{
			m_pPassLockIndicator->SetDrawColor( GetTeamColor( pLocalPlayer->GetTeamNumber() ) );
		}

		if ( m_pTextLockedOn )
		{
			m_pTextLockedOn->SetVisible( true );
		}

		if ( m_pTextPassIncoming )
		{
			m_pTextPassIncoming->SetVisible( false );
		}

		if ( m_pTextInPassRange )
		{
			m_pTextInPassRange->SetVisible( false );
		}
	}
	else if ( bTargetable )
	{
		if ( m_pTextLockedOn )
		{
			m_pTextLockedOn->SetVisible( false );
		}

		if ( m_pTextPassIncoming )
		{
			m_pTextPassIncoming->SetVisible( false );
		}

		if ( m_pTextInPassRange )
		{
			m_pTextInPassRange->SetVisible( true );
		}
	}
	//else if ( !bTargetable )
	//{
	//	m_pTopText->SetText( "CAN'T SEE YOU" );
	//}

	if ( m_pTextPlayerName )
	{
		m_pTextPlayerName->SetText( pBallCarrier->GetPlayerName() );
	}

	if ( m_pTextBox )
	{
		m_pTextBox->SetBorder( m_pTextBoxBorderNormal );
	}
}

//-----------------------------------------------------------------------------
// CTFHudPasstimeEventText
//-----------------------------------------------------------------------------

namespace HudPasstimeEventText 
{
	static const float flInSec = 0.1f;
	static const float flOutSec = 0.25f;
	static const float flShowSec = 3.0f;
	static const float flPauseSec = 0.1f;
	static const int iQueueDepthPanic = 3;
	static char const * const pKeyTeam = "team";
	static char const * const pKeySubject = "subject";
	static char const * const pKeySource = "source";
}

//-----------------------------------------------------------------------------
CTFHudPasstimeEventText::QueueElement::QueueElement()
{
	title[0] = (wchar_t)0;
	detail[0] = (wchar_t)0;
	bonus[0] = (wchar_t)0;
}

//-----------------------------------------------------------------------------
CTFHudPasstimeEventText::CTFHudPasstimeEventText()
	: m_localizeKeys( "" )
{
	m_bValid = false;
	m_pTitleLabel = 0;
	m_pDetailLabel = 0;
	m_pBonusLabel = 0;
	m_state = State::Idle;
}

//-----------------------------------------------------------------------------
CTFHudPasstimeEventText::~CTFHudPasstimeEventText()
{
	Clear();
}

//-----------------------------------------------------------------------------
// static
void CTFHudPasstimeEventText::SetLabelText( vgui::Label *pLabel, const wchar_t *pText )
{
	if ( pText && pText[0] )
	{
		pLabel->SetVisible( true );
		pLabel->SetText( pText );
	}
	else
	{
		pLabel->SetVisible( false );
		pLabel->SetText( L"" );
	}
}

//-----------------------------------------------------------------------------
void CTFHudPasstimeEventText::EnterState( State state, float duration )
{
	m_state = state;
	
	// move things faster if the queue is backlogged, but State::Pause is always the same duration
	if ( (state != State::Pause) && (m_queue.Count() >= HudPasstimeEventText::iQueueDepthPanic) ) 
	{
		duration /= 2.0f;
	}
	m_displayTimer.Start( duration );
}

//-----------------------------------------------------------------------------
void CTFHudPasstimeEventText::SetAlpha( int ia )
{
	if ( m_pTitleLabel )
	{
		m_pTitleLabel->SetAlpha( ia );
		m_pTitleLabel->SetVisible( ia != 0 );
	}

	if ( m_pDetailLabel )
	{
		m_pDetailLabel->SetAlpha( ia );
		m_pDetailLabel->SetVisible( ia != 0 );
	}

	if ( m_pBonusLabel )
	{
		m_pBonusLabel->SetAlpha( ia );
		m_pBonusLabel->SetVisible( ia != 0 );
	}
}

//-----------------------------------------------------------------------------
void CTFHudPasstimeEventText::Tick()
{
	if ( !m_bValid ) 
		return;

	switch( m_state )
	{
	case State::Idle:
		if ( !m_queue.IsEmpty() ) 
		{
			SetAlpha( 1 );
			auto msg = m_queue.RemoveAtHead();
			if ( m_pTitleLabel )
			{
				SetLabelText( m_pTitleLabel, msg.title );
			}
			if ( m_pDetailLabel )
			{
				SetLabelText( m_pDetailLabel, msg.detail );
			}
			if ( m_pBonusLabel )
			{
				SetLabelText( m_pBonusLabel, msg.bonus );
			}
			EnterState( State::In, HudPasstimeEventText::flInSec );
		}
		else
		{
			SetAlpha( 0 );
		}
		break;

	case State::In:
		if ( m_displayTimer.IsElapsed() )
		{
			EnterState( State::Show, HudPasstimeEventText::flShowSec );
			SetAlpha( 255 );
		}
		else
		{
			// animate alpha
			// note: GetElapsedTime()
			float flFrac = m_displayTimer.GetElapsedTime() / m_displayTimer.GetCountdownDuration();
			SetAlpha( (int) (flFrac * flFrac * 255.0f) );
		}
		break;

	case State::Show:
		if ( m_displayTimer.IsElapsed() )
		{
			EnterState( State::Out, HudPasstimeEventText::flOutSec );
		}
		break;

	case State::Out:
		if ( m_displayTimer.IsElapsed() )
		{
			EnterState( State::Pause, HudPasstimeEventText::flPauseSec );
			SetAlpha( 0 );
		}
		else
		{
			// animate alpha
			// note: GetRemainingTime()
			float flFrac = m_displayTimer.GetRemainingTime() / m_displayTimer.GetCountdownDuration();
			SetAlpha( (int) (flFrac * flFrac * 255.0f) );
		}
		break;

	case State::Pause:
		if ( m_displayTimer.IsElapsed() )
		{
			m_state = State::Idle;
		}
		break;
	}
}

//-----------------------------------------------------------------------------
void CTFHudPasstimeEventText::Clear()
{
	while( !m_queue.IsEmpty() )
	{
		m_queue.RemoveAtTail();
	}
}

//-----------------------------------------------------------------------------
void CTFHudPasstimeEventText::SetControls( vgui::Label *pTitleLabel, vgui::Label *pDetailLabel, vgui::Label *pBonusLabel )
{
	m_pTitleLabel = pTitleLabel;
	m_pDetailLabel = pDetailLabel;
	m_pBonusLabel = pBonusLabel;
	m_bValid = pTitleLabel && pDetailLabel && pBonusLabel;
}

//-----------------------------------------------------------------------------
void CTFHudPasstimeEventText::SetPlayerName( C_TFPlayer *pPlayer, const char *pKey )
{
	if ( pPlayer )
	{
		g_pVGuiLocalize->ConvertANSIToUnicode( pPlayer->GetPlayerName(), m_pwcsBuf, sizeof(m_pwcsBuf) );
		m_localizeKeys->SetWString( pKey,  m_pwcsBuf );
	}
	else
	{
		m_localizeKeys->SetWString( pKey, L"" );
	}
}

//-----------------------------------------------------------------------------
void CTFHudPasstimeEventText::SetTeam( C_TFPlayer *pPlayer )
{
	if ( pPlayer ) 
	{
		C_TFTeam *pTeam = GetGlobalTFTeam( pPlayer->GetTeamNumber() );
		m_localizeKeys->SetWString( "team", pTeam ? pTeam->Get_Localized_Name() : L"" );
	}
	else
	{
		m_localizeKeys->SetWString( "team", L"" );
	}
}

//-----------------------------------------------------------------------------
template< int TArraySize >
void CTFHudPasstimeEventText::ConstructNewString( const char *pLocTag, wchar_t (&out)[TArraySize] )
{
	// FIXME calling find is redundant 
	if ( pLocTag && pLocTag[0] && g_pVGuiLocalize->Find( pLocTag ) )
	{
		g_pVGuiLocalize->ConstructString_safe( out, pLocTag, m_localizeKeys );
	}
	else
	{
		out[0] = (wchar_t)0;
	}
}

//-----------------------------------------------------------------------------
void CTFHudPasstimeEventText::Enqueue( C_TFPlayer *pSource, C_TFPlayer *pSubject, const char *pTitle, const char *pDetail, const char *pBonus )
{
	if ( !m_bValid || !pSubject )
		return;

	SetTeam( pSubject );
	SetPlayerName( pSubject, HudPasstimeEventText::pKeySubject );
	SetPlayerName( pSource, HudPasstimeEventText::pKeySource );

	auto *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	auto bShowBonus = (pSubject == pLocalPlayer)
		|| (pLocalPlayer->IsObserver() && pLocalPlayer->GetObserverTarget() == pLocalPlayer);
	
	QueueElement e;
	ConstructNewString( pTitle, e.title );
	ConstructNewString( pDetail, e.detail );
	ConstructNewString( bShowBonus ? pBonus : nullptr, e.bonus );
	m_queue.Insert( e );
}

//-----------------------------------------------------------------------------
void CTFHudPasstimeEventText::EnqueueGeneric( const char *pTitle, const char *pDetail, const char *pBonus )
{
	QueueElement e;
	ConstructNewString( pTitle, e.title );
	ConstructNewString( pDetail, e.detail );
	ConstructNewString( pBonus, e.bonus );
	m_queue.Insert( e );
}

//-----------------------------------------------------------------------------
void CTFHudPasstimeEventText::EnqueueSteal( C_TFPlayer *pVictim, C_TFPlayer *pStealer )
{
	Enqueue( pVictim, pStealer, "#Msg_PasstimeEventStealTitle", "#Msg_PasstimeEventStealDetail", "#Msg_PasstimeEventStealBonus" );
}

//-----------------------------------------------------------------------------
void CTFHudPasstimeEventText::EnqueuePass( C_TFPlayer *pThrower, C_TFPlayer *pCatcher )
{
	Enqueue( pThrower, pCatcher, "#Msg_PasstimeEventPassTitle", "#Msg_PasstimeEventPassDetail", "#Msg_PasstimeEventPassBonus" );
}

//-----------------------------------------------------------------------------
void CTFHudPasstimeEventText::EnqueueInterception( C_TFPlayer *pThrower, C_TFPlayer *pCatcher )
{
	Enqueue( pThrower, pCatcher, "#Msg_PasstimeEventInterceptTitle", "#Msg_PasstimeEventInterceptDetail", "#Msg_PasstimeEventInterceptBonus" );
}

//-----------------------------------------------------------------------------
void CTFHudPasstimeEventText::EnqueueScore( C_TFPlayer *pThrower, C_TFPlayer *pAssister )
{
	if ( pAssister )
		Enqueue( pAssister, pThrower, "#Msg_PasstimeEventScoreTitle", "#Msg_PasstimeEventScoreDetail_Assist", "#Msg_PasstimeEventScoreBonus" );
	else
		Enqueue( pAssister, pThrower, "#Msg_PasstimeEventScoreTitle", "#Msg_PasstimeEventScoreDetail_NoAssist", "#Msg_PasstimeEventScoreBonus" );
}

//-----------------------------------------------------------------------------
// CTFHudPasstimeBallStatus
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
CTFHudPasstimeBallStatus::CTFHudPasstimeBallStatus( Panel *pParent )
	: CTFHudPasstimePanel( pParent, "HudPasstimeBallStatus" )
{
	m_bInitialized = false;
	m_bReset = false;
	m_bGoalsFound = false;
	m_iXBlueProgress = -100.0f;
	m_iXRedProgress = -100.0f;
	m_iYBlueProgress = -100.0f;
	m_iYRedProgress = -100.0f;
	memset( m_pGoalIconsBlue, 0, sizeof( m_pGoalIconsBlue ) );
	memset( m_pGoalIconsRed, 0, sizeof( m_pGoalIconsRed ) );
	memset( m_pPlayerIcons, 0, sizeof( m_pPlayerIcons ) );
	m_pProgressBall = 0;
	m_pProgressBallCarrierName = 0;
	m_pProgressLevelBar = 0;
	m_pSelfPlayerIcon = 0;
	m_pEventText = new CTFHudPasstimeEventText();
	m_pBallPowerMeterFillContainer = nullptr;
	m_pBallPowerMeterFill = nullptr;
	m_pBallPowerMeterFrame = nullptr;
}

//-----------------------------------------------------------------------------
CTFHudPasstimeBallStatus::~CTFHudPasstimeBallStatus()
{
	delete m_pEventText;
	m_pEventText = nullptr;
}

//-----------------------------------------------------------------------------
void CTFHudPasstimeBallStatus::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );
	m_bInitialized = false;

	LoadControlSettings( "resource/UI/HudPasstimeBallStatus.res", NULL, NULL, NULL );
	m_pPowerCluster = FindControl<EditablePanel>( "BallPowerCluster" );
	m_pProgressBall = FindControl<ImagePanel>( "ProgressBallIcon" );
	m_pProgressBallCarrierName = FindControl<Label>( "ProgressBallCarrierName" );
	m_pProgressLevelBar = FindControl<Panel>( "ProgressLevelBar" );
	m_pSelfPlayerIcon = FindControl<ImagePanel>( "ProgressSelfPlayerIcon" );

	Panel *pBallPowerRoot = FindControl<Panel>( "BallPowerCluster" );
	if ( !pBallPowerRoot ) 
	{
		pBallPowerRoot = this;
	}
	m_pBallPowerMeterFillContainer = pBallPowerRoot->FindControl<Panel>( "BallPowerMeterFillContainer", true );
	m_pBallPowerMeterFill = pBallPowerRoot->FindControl<ImagePanel>( "BallPowerMeterFill", true );
	m_pBallPowerMeterFrame = pBallPowerRoot->FindControl<Panel>( "BallPowerMeterFrame", true );
	m_pBallPowerMeterFinalSection = pBallPowerRoot->FindControl<Panel>( "BallPowerMeterFinalSectionContainer", true );

	m_pEventText->SetControls( FindControl<Label>( "EventTitleLabel" ), 
		FindControl<Label>( "EventDetailLabel" ), 
		FindControl<Label>( "EventBonusLabel" ) );

	m_bInitialized = m_pProgressBall
		&& m_pProgressBallCarrierName
		&& m_pProgressLevelBar
		&& m_pSelfPlayerIcon
		&& m_pBallPowerMeterFillContainer
		&& m_pBallPowerMeterFill
		&& m_pBallPowerMeterFinalSection
		&& m_pBallPowerMeterFrame;

	if ( !m_bInitialized )
	{
		// just bail if the res file is missing required controls
		// this prevents a lot of stupid null checks in the future
		return;
	}

	// set up some ballpower stuff
	m_iBallPowerMeterFillWidth = m_pBallPowerMeterFillContainer ? m_pBallPowerMeterFillContainer->GetWide() : 0;
	int iFinalSectionWidth = (int)(m_iBallPowerMeterFillWidth * 0.2f);
	if ( m_pBallPowerMeterFinalSection )
	{
		m_pBallPowerMeterFinalSection->SetWide( iFinalSectionWidth );
		m_pBallPowerMeterFinalSection->SetPos( m_pBallPowerMeterFinalSection->GetXPos() + (m_iBallPowerMeterFillWidth - iFinalSectionWidth), m_pBallPowerMeterFinalSection->GetYPos() );
	}
	m_iPrevBallPower = g_pPasstimeLogic 
		? g_pPasstimeLogic->GetBallPower() 
		: 0;
	
	// find the left/right markers in the res files
	{
		auto *pRedEnd = FindChildByName( "RedProgressEnd" );
		auto *pBlueEnd = FindChildByName( "BlueProgressEnd" );
		if ( pRedEnd && pBlueEnd ) 
		{
			pRedEnd->GetPos( m_iXRedProgress, m_iYRedProgress );
			pBlueEnd->GetPos( m_iXBlueProgress, m_iYBlueProgress );
		}
		else 
		{
			// no markers in the res file, just give it offscreen but valid coords
			m_iXBlueProgress = -100.0f;
			m_iXRedProgress = -100.0f;
			m_iYBlueProgress = -100.0f;
			m_iYRedProgress = -100.0f;
		}
	}

	// find all the goal icon image panels
	{
		char buf[16];
		for ( auto i = 0; i < NumGoalIcons; ++i ) 
		{
			V_sprintf_safe( buf, "GoalRed%i", i );
			m_pGoalIconsRed[i] = FindControl<vgui::ImagePanel>( buf );
			V_sprintf_safe( buf, "GoalBlue%i", i );
			m_pGoalIconsBlue[i] = FindControl<vgui::ImagePanel>( buf );
		}
	}

	ListenForGameEvent( PasstimeGameEvents::BallFree::s_eventName );
	ListenForGameEvent( PasstimeGameEvents::BallGet::s_eventName );
	ListenForGameEvent( PasstimeGameEvents::BallStolen::s_eventName );
	ListenForGameEvent( PasstimeGameEvents::PassCaught::s_eventName );
	ListenForGameEvent( PasstimeGameEvents::Score::s_eventName );
	Reset(); // this ensures players will try to guess game state for the hud if they join a game in progress
	vgui::ivgui()->AddTickSignal( GetVPanel() );

	// find all the player icon image panels
	{
		char controlname[32];
		for ( auto i = 0; i < MAX_PLAYERS; ++i ) 
		{
			V_sprintf_safe( controlname, "playericon%i", i ); // ugh
			m_pPlayerIcons[i] = FindControl<vgui::ImagePanel>( controlname );
			if ( m_pPlayerIcons[i] )
			{
				m_pPlayerIcons[i]->SetEnabled( true );
				m_pPlayerIcons[i]->SetShouldScaleImage( true );
			}
		}
	}
}

//-----------------------------------------------------------------------------
bool CTFHudPasstimeBallStatus::BShouldDraw() const
{
	CBasePlayer *pPlayer = CBasePlayer::GetLocalPlayer();
	if ( !pPlayer || !pPlayer->IsAlive() || ( pPlayer->GetObserverMode() == OBS_MODE_FREEZECAM ) )
	{
		return false;
	}

	gamerules_roundstate_t roundstate = TFGameRules()->State_Get();
	return ((roundstate == GR_STATE_RND_RUNNING) || (roundstate == GR_STATE_STALEMATE))
		&& !TFGameRules()->InSetup();
}

//-----------------------------------------------------------------------------
// FIXME copypasta with tf_passtime_logic.cpp
static float CalcProgressFrac( const Vector& vecOrigin )
{
	Assert( g_pPasstimeLogic && (g_pPasstimeLogic->GetNumSections() > 0) );

	Vector arrTrackPoints[16];
	g_pPasstimeLogic->GetTrackPoints( arrTrackPoints );

	float flBestDist = FLT_MAX;
	float flBestLen = 0;
	float flTotalLen = 1; // don't set 0 so div by zero is impossible

	Vector vecThisPoint;
	Vector vecPointOnLine;
	Vector vecPrevPoint = arrTrackPoints[0];
	float flThisFrac = 0;
	float flThisLen = 0;
	float flThisDist = 0;
	for ( int i = 1; i < ARRAYSIZE(arrTrackPoints); ++i )
	{
		vecThisPoint = arrTrackPoints[i];
		if ( vecThisPoint.IsZero() )
		{
			break;
		}
		flThisLen = (vecThisPoint - vecPrevPoint).Length();
		flTotalLen += flThisLen;
		CalcClosestPointOnLineSegment( vecOrigin, vecPrevPoint, vecThisPoint, vecPointOnLine, &flThisFrac );
		flThisDist = (vecPointOnLine - vecOrigin).Length();
		if ( flThisDist < flBestDist )
		{
			flBestDist = flThisDist;
			flBestLen = flTotalLen - (flThisLen * (1.0f - flThisFrac));
		}
		vecPrevPoint = vecThisPoint;
	}

	return (float)(flBestLen / flTotalLen);
}

//-----------------------------------------------------------------------------
// FIXME copypasta with tf_passtime_logic.cpp
static float CalcBallProgressFrac()
{
	Assert( g_pPasstimeLogic && g_pPasstimeLogic->GetBall() && (g_pPasstimeLogic->GetNumSections() > 0) );
	CPasstimeBall *pBall = g_pPasstimeLogic->GetBall();
	CTFPlayer *pCarrier = pBall->GetCarrier();
	return CalcProgressFrac( pCarrier 
		? pCarrier->GetNetworkOrigin()
		: pBall->GetNetworkOrigin() );
}

//-----------------------------------------------------------------------------
void CTFHudPasstimeBallStatus::UpdateGoalIcon( vgui::ImagePanel *pIcon, C_FuncPasstimeGoal *pGoal )
{
	Assert( m_bInitialized );
	// TODO: animations on enable/disable?
	if ( !pIcon ) 
	{
		return;
	}

	if ( !pGoal )
	{
		pIcon->SetVisible( false );
		return;
	}

	pIcon->SetVisible( true );

	const auto iDisabledAlpha = 50;
	const auto iEnabledAlpha = 255;
	pIcon->SetAlpha( ( pGoal->BGoalTriggerDisabled() && ( pGoal->GetGoalType() != C_FuncPasstimeGoal::TYPE_TOWER ) )
		? iDisabledAlpha
		: iEnabledAlpha );

	// TODO don't call SetImage(char*) every frame, wtf.
	pIcon->SetImage( GetProgressGoalImage( pGoal ) );

	const float flProgressFrac = CalcProgressFrac( pGoal->GetAbsOrigin() );
	const int iActualBarHalfHeight = m_pProgressLevelBar ? m_pProgressLevelBar->GetTall() / 6 : 0; // magic number because NPOT waste in image
	int iX = Lerp( flProgressFrac, m_iXBlueProgress, m_iXRedProgress ) - ( pIcon->GetWide() / 2 );
	int iY = Lerp( flProgressFrac, m_iYBlueProgress, m_iYRedProgress ) - iActualBarHalfHeight;
	pIcon->SetPos( iX, iY );
}

//-----------------------------------------------------------------------------
void CTFHudPasstimeBallStatus::OnTickHidden()
{
	m_bGoalsFound = false;
	
	if ( m_pProgressBall )
	{
		m_pProgressBall->SetVisible( false );
	}

	if ( m_pProgressBallCarrierName )
	{
		m_pProgressBallCarrierName->SetVisible( false );
	}

	if ( m_pSelfPlayerIcon )
	{
		m_pSelfPlayerIcon->SetVisible( false );
	}

	if ( m_pProgressLevelBar )
	{
		m_pProgressLevelBar->SetVisible( false );
	}

	if ( m_pPowerCluster )
	{
		m_pPowerCluster->SetVisible( false );
	}

	for ( int i = 0; i < MAX_PLAYERS; ++i )
	{
		if ( m_pPlayerIcons[i] )
			m_pPlayerIcons[i]->SetVisible( false );
	}

	m_iPrevBallPower = g_pPasstimeLogic 
		? g_pPasstimeLogic->GetBallPower() 
		: 0;

	HideGoalIcons();
}

//-----------------------------------------------------------------------------
void CTFHudPasstimeBallStatus::OnTickVisible( C_TFPlayer *pLocalPlayer, C_PasstimeBall *pBall)
{
	if ( m_pProgressLevelBar )
	{
		m_pProgressLevelBar->SetVisible( true );
	}

	//
	// Update ball icon
	//
	float flBallProgressFrac = CalcBallProgressFrac();
	int iX_Ball = m_pProgressBall ? Lerp( flBallProgressFrac, m_iXBlueProgress, m_iXRedProgress ) - ( m_pProgressBall->GetWide() / 2 ) : 0;
	int iY_Ball = m_pProgressBall ? Lerp( flBallProgressFrac, m_iYBlueProgress, m_iYRedProgress ) - ( m_pProgressBall->GetTall() / 2 ) : 0;

	if ( m_pProgressBall )
	{
		m_pProgressBall->SetVisible( true );
		m_pProgressBall->SetPos( iX_Ball, iY_Ball );
	}

	// todo setimage from event, not every frame
	// todo look in to letting entities manage their own hud elements
	CTFPlayer *pCarrier = pBall->GetCarrier();
	int iTeam = pCarrier
		? pCarrier->GetTeamNumber()
		: pBall->GetTeamNumber();
	if ( m_pProgressBall )
	{
		m_pProgressBall->SetImage( GetProgressBallImageForTeam( iTeam ) );
	}

	//
	// Update ball power
	//
	if ( m_pPowerCluster )
	{
		m_pPowerCluster->SetVisible( true );
	}
	{
		// update the power bar
		int iCurPower = g_pPasstimeLogic->GetBallPower();
		int iThreshold = tf_passtime_powerball_threshold.GetInt();
		int iAlpha = (iCurPower > iThreshold)
			? FLerp( 150, 255, (1 + sin(gpGlobals->curtime*5)) / 2.0f)
			: 222;
		if ( m_pBallPowerMeterFill )
		{
			m_pBallPowerMeterFill->SetDrawColor( GetTeamColor( iTeam, iAlpha ) );
		}
		float flPowerFrac = (iCurPower / 100.f);
		if ( m_pBallPowerMeterFillContainer )
		{
			m_pBallPowerMeterFillContainer->SetWide( m_iBallPowerMeterFillWidth * flPowerFrac );
		}

		// show power up/down notifications
		// this should really be done with a game event but this is a temp experimental feature for now
		bool bWasAboveThreshold = m_iPrevBallPower > iThreshold;
		bool bIsAboveThreshold = iCurPower > iThreshold;
		m_iPrevBallPower = iCurPower;
		if ( bWasAboveThreshold && !bIsAboveThreshold ) 
		{
			m_pEventText->Clear(); // push everything else out of the way
			m_pEventText->EnqueueGeneric( "#Msg_PasstimeEventPowerDownTitle", "#Msg_PasstimeEventPowerDownDetail", "#Msg_PasstimeEventPowerDownBonus" );
		}
		else if ( !bWasAboveThreshold && bIsAboveThreshold )
		{
			m_pEventText->Clear(); // push everything else out of the way
			m_pEventText->EnqueueGeneric( "#Msg_PasstimeEventPowerUpTitle", "#Msg_PasstimeEventPowerUpDetail", "#Msg_PasstimeEventPowerUpBonus" );
		}
	}

	//
	// Update player icons
	// TODO make less bad
	//
	C_TFPlayer *pSpecTarget = ToTFPlayer( pLocalPlayer->GetObserverTarget() );
	if ( m_pSelfPlayerIcon )
	{
		m_pSelfPlayerIcon->SetVisible( false );
	}
	const int iActualBarHalfHeight = m_pProgressLevelBar ? m_pProgressLevelBar->GetTall() / 7 : 0; // magic number because NPOT waste in image
	for ( int iEntIndex = 1; iEntIndex <= MAX_PLAYERS; iEntIndex++ )
	{
		vgui::ImagePanel *pIcon = m_pPlayerIcons[iEntIndex - 1];
		if ( !pIcon )
			break;

		if ( !g_TF_PR->IsConnected( iEntIndex ) )
		{
			pIcon->SetVisible( false );
			continue;
		}

		int iEntTeam = g_TF_PR->GetTeam( iEntIndex );
		if ( !g_TF_PR->IsAlive( iEntIndex )  // no dead people; not the same as IsDead
			|| ( ( iEntTeam != TF_TEAM_RED ) && ( iEntTeam != TF_TEAM_BLUE) )  ) // no spectators
		{
			pIcon->SetVisible( false );
			continue;
		}

		// needed for origin
		// dormant means "not available on this client right now"
		C_TFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( iEntIndex ) );
		if ( !pPlayer || pPlayer->IsDormant() )
		{
			pIcon->SetVisible( false );
			continue;
		}

		// don't show cloaked spies
		bool bSpy = g_TF_PR->GetPlayerClass( iEntIndex ) == TF_CLASS_SPY;
		if ( bSpy )
		{
			if ( pPlayer->m_Shared.IsFullyInvisible() )
			{
				pIcon->SetVisible( false );
				continue;
			}

			int iDisguiseTeam = pPlayer->m_Shared.GetDisguiseTeam();
			if ( iDisguiseTeam == TF_TEAM_RED || iDisguiseTeam == TF_TEAM_BLUE )
			{
				iEntTeam = iDisguiseTeam;
			}
		}

		const float flProgressFrac = CalcProgressFrac( pPlayer->GetNetworkOrigin() );
		// Player pips
		{
			pIcon->SetVisible( true );
			if ( iEntTeam == TF_TEAM_RED )
			{
				int iX = Lerp( flProgressFrac, m_iXBlueProgress, m_iXRedProgress ) - (pIcon->GetWide() / 2);
				int iY = Lerp( flProgressFrac, m_iYBlueProgress, m_iYRedProgress ) - pIcon->GetTall() - iActualBarHalfHeight;
				pIcon->SetPos( iX, iY );
				pIcon->SetImage( "../passtime/hud/passtime_ballcontrol_team_red" );
			}
			else
			{
				int iX = Lerp( flProgressFrac, m_iXBlueProgress, m_iXRedProgress ) - (pIcon->GetWide() / 2);
				int iY = Lerp( flProgressFrac, m_iYBlueProgress, m_iYRedProgress ) + iActualBarHalfHeight;
				pIcon->SetPos( iX, iY );
				pIcon->SetImage( "../passtime/hud/passtime_ballcontrol_team_blue" );
			}
		}

		// Local player image
		if ( g_TF_PR->IsLocalPlayer( iEntIndex ) || ( pLocalPlayer->IsObserver() && ( pPlayer == pSpecTarget ) ) )
		{
			if ( m_pSelfPlayerIcon )
			{
				// TODO don't call SetImage(char*) every frame, wtf.
				if ( pLocalPlayer->IsObserver() && pSpecTarget )
				{
					m_pSelfPlayerIcon->SetImage( GetPlayerProgressPortrait( pSpecTarget ) );
				}
				else
				{
					m_pSelfPlayerIcon->SetImage( GetPlayerProgressPortrait( pLocalPlayer ) );
				}

				int iX = Lerp( flProgressFrac, m_iXBlueProgress, m_iXRedProgress ) - (m_pSelfPlayerIcon->GetWide() / 2);
				int iY = Lerp( flProgressFrac, m_iYBlueProgress, m_iYRedProgress ) - m_pSelfPlayerIcon->GetTall() - iActualBarHalfHeight;
				m_pSelfPlayerIcon->SetVisible( true );
				m_pSelfPlayerIcon->SetPos( iX, iY );
			}
		}
	}

	//
	// Refresh goal icons if necessary
	//
	bool bReadyToFindGoals = (g_pPasstimeLogic->GetNumSections() > 0);
	if ( !m_bGoalsFound && bReadyToFindGoals ) 
	{
		// release any existing handles to goals
		for ( auto i = 0; i < NumGoalIcons; ++i )
		{
			m_hGoalsBlue[i].Term();
			m_hGoalsRed[i].Term();
		}

		// Update the goal list and pair hud icons with actual goals
		const int iMaxSortedGoals = 8; // arbitrary
		const auto &goals = C_FuncPasstimeGoal::GetAutoList();
		const int iNumGoals = goals.Count();
		if ( ( iNumGoals > 1 ) && ( iNumGoals < iMaxSortedGoals ) )
		{
			// sort goals by position in world, from blue to red (blue progress is always 0, red is always 1))
			C_FuncPasstimeGoal* sortedgoals[iMaxSortedGoals];
			std::copy( goals.begin(), goals.end(), sortedgoals );
			std::sort( sortedgoals, sortedgoals + iNumGoals, []( C_FuncPasstimeGoal* a, C_FuncPasstimeGoal* b ) 
			{
				// this is wasteful but it should only happen one frame per round
				// The order of the icons in the hud res file determines which direction to sort these
				// so that the iteration below visits the goals in the same order.
				return CalcProgressFrac( a->GetAbsOrigin() ) < CalcProgressFrac( b->GetAbsOrigin() );
			});

			// Pair goals and icons so the tick function can update the icon state based on the actual goal state
			// This should work for any number of goals and icons in any order.
			// If there are more goals than icons, just ignore them, but make sure the hud shows at least
			// the goals farthest from the middle of the map in a symmetrical way, so iterate from both ends
			// of the array of sorted goals.

			// pair first N blue goals in left-right order
			// first icon in array is leftmost blue goal on hud is first goal in sorted array
			int iRedIcon = 0;
			int iBlueIcon = 0;
			for ( int iGoal = 0; iGoal < iNumGoals; ++iGoal ) 
			{
				// NOTE: goal's teams are backwards: the blue-colored goals on the blue side of the map are
				// on team red because that's where red scores. doh.
				auto *pGoal = sortedgoals[iGoal];
				if ( pGoal && ( iBlueIcon < NumGoalIcons ) && ( pGoal->GetTeamNumber() == TF_TEAM_RED ) )
				{
					m_hGoalsBlue[iBlueIcon++] = pGoal;
				}
				
				pGoal = sortedgoals[iNumGoals - iGoal - 1];
				if ( pGoal && ( iRedIcon < NumGoalIcons ) && ( pGoal->GetTeamNumber() == TF_TEAM_BLUE ) )
				{
					m_hGoalsRed[iRedIcon++] = pGoal;
				}
			}

			m_bGoalsFound = true;
		}
	}
			
	//
	// Update goal icons
	//
	if ( m_bGoalsFound )
	{
		for ( auto i = 0; i < NumGoalIcons; ++i ) 
		{
			UpdateGoalIcon( m_pGoalIconsBlue[i], m_hGoalsBlue[i].Get() );
			UpdateGoalIcon( m_pGoalIconsRed[i], m_hGoalsRed[i].Get() );
		}
	}
	else 
	{
		HideGoalIcons();
	}
}

//-----------------------------------------------------------------------------
void CTFHudPasstimeBallStatus::HideGoalIcons() 
{
	for ( int i = 0; i < NumGoalIcons; ++i )
	{
		auto *pIcon = m_pGoalIconsBlue[i];
		if ( pIcon ) 
			pIcon->SetVisible( false );
		pIcon = m_pGoalIconsRed[i];
		if ( pIcon ) 
			pIcon->SetVisible( false );
	}
}

//-----------------------------------------------------------------------------
void CTFHudPasstimeBallStatus::OnTick()
{
	if ( !g_pPasstimeLogic || !m_bInitialized ) 
	{
		// happens randomly during map load
		m_pEventText->Clear();
		return;
	}

	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pLocalPlayer )
	{
		// can happen during exit
		return; 
	}

	// Tick the event text regardless of the state of the rest of the hud so
	// that messages can continue to display even after the round ends or whatever.
	m_pEventText->Tick();

	// I wish there were a reliable way to do this in an event-driven way instead of every frame
	C_PasstimeBall *pBall = g_pPasstimeLogic->GetBall();
	if ( !BShouldDraw() || !pBall ) 
	{
		OnTickHidden();
		
		// this is the easiest way I could find to refresh the goals when switching maps
		// todo this is dumb
		m_bGoalsFound = false;
	}
	else
	{
		OnTickVisible( pLocalPlayer, pBall );
	}

	//
	// Try to figure out game state that's usually managed by events if hud was reloaded mid-game
	// This happens at the end to ensure that the hud is fully initialized before trying to reset
	// on the first frame a player joins a game in progress
	//
	if ( m_bReset )
	{
		m_pEventText->Clear();

		// I don't *think* it's possible that this could accidentally end up 
		// running every frame.

		// Order of operations here is important, see functions for details
		// todo make less bad
		if ( TryForceBallGet() || TryForceBallFree() )
		{
			m_bReset = false;
		}
	};
}

//-----------------------------------------------------------------------------
bool CTFHudPasstimeBallStatus::TryForceBallGet()
{
	Assert( m_bInitialized );

	// The HUD was reset during play, or possibly due to joining a game inprogroess,
	// so try to get into a BallGet state if possible
	if ( !g_TF_PR ) 
	{
		// is this even possible?
		return false;
	}

	for ( int iPlayer = 1 ; iPlayer <= MAX_PLAYERS; iPlayer++ )
	{
		if ( !g_TF_PR->IsConnected( iPlayer ) )
		{
			continue;
		}

		C_TFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( iPlayer ) );
		if ( !pPlayer || !pPlayer->m_Shared.HasPasstimeBall() )
		{
			continue;
		}
		
		if ( g_TF_PR->IsLocalPlayer( iPlayer ) )
		{
			OnBallGetSelf( iPlayer );
		}
		else
		{
			OnBallGetOther( iPlayer );
		}

		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
bool CTFHudPasstimeBallStatus::TryForceBallFree()
{
	Assert( m_bInitialized );
	
	// The HUD was reset during play, or possibly due to joining a game inprogroess,
	// so try to get into a BallGet state if possible
	if ( !g_pPasstimeLogic )
	{
		return false;
	}

	C_PasstimeBall *pBall = g_pPasstimeLogic->GetBall();
	if ( !m_bInitialized || !pBall ) 
	{
		return false;
	}

	if ( m_pProgressBallCarrierName )
	{
		m_pProgressBallCarrierName->SetVisible( false );
	}
	return true;
}

//-----------------------------------------------------------------------------
void CTFHudPasstimeBallStatus::OnBallGet( int getterIndex )
{
	Assert( m_bInitialized && g_PR );

	if ( g_PR->IsLocalPlayer( getterIndex ) )
	{
		OnBallGetSelf( getterIndex );
	}
	else
	{
		OnBallGetOther( getterIndex );
	}
}

//-----------------------------------------------------------------------------
void CTFHudPasstimeBallStatus::FireGameEvent( IGameEvent *pEvent )
{
	if ( !m_bInitialized || !g_TF_PR || !g_PR ) 
	{
		return;
	}

	const char *pszEventName = pEvent->GetName();
	if ( FStrEq( pszEventName, PasstimeGameEvents::BallFree::s_eventName ) )
	{
		PasstimeGameEvents::BallFree ev( pEvent );
		C_TFPlayer *pOwner = ToTFPlayer( UTIL_PlayerByIndex( ev.ownerIndex ) );
		C_TFPlayer *pAttacker = ToTFPlayer( UTIL_PlayerByIndex( ev.attackerIndex ) );
		const bool wasMyBall = pOwner == C_TFPlayer::GetLocalPlayer();
		if ( wasMyBall )
		{
			OnBallFreeSelf( pOwner, pAttacker );
		}
		else
		{
			OnBallFreeOther( pOwner, pAttacker );
		}
	}
	else if ( FStrEq( pszEventName, PasstimeGameEvents::BallGet::s_eventName ) )
	{
		OnBallGet( PasstimeGameEvents::BallGet( pEvent ).ownerIndex );
	}
	else if ( FStrEq( pszEventName, PasstimeGameEvents::BallStolen::s_eventName ) )
	{
		PasstimeGameEvents::BallStolen ballStolenEvent( pEvent );
		OnBallGet( ballStolenEvent.attackerIndex );

		auto *pAttacker = ToTFPlayer( UTIL_PlayerByIndex(ballStolenEvent.attackerIndex) );
		auto *pVictim = ToTFPlayer( UTIL_PlayerByIndex(ballStolenEvent.victimIndex) );
		m_pEventText->EnqueueSteal( pVictim, pAttacker );
	}
	else if ( FStrEq( pszEventName, PasstimeGameEvents::PassCaught::s_eventName ) )
	{
		PasstimeGameEvents::PassCaught passCaughtEvent( pEvent );
		OnBallGet( passCaughtEvent.catcherIndex );
		
		auto *pCatcher = ToTFPlayer( UTIL_PlayerByIndex( passCaughtEvent.catcherIndex ) );
		auto *pThrower = ToTFPlayer( UTIL_PlayerByIndex( passCaughtEvent.passerIndex ) );
		if ( pCatcher && pThrower ) 
		{
			if ( pCatcher->GetTeamNumber() == pThrower->GetTeamNumber() )
			{
				m_pEventText->EnqueuePass( pThrower, pCatcher );
			}
			else
			{
				m_pEventText->EnqueueInterception( pThrower, pCatcher );
			}
		}
	}
	else if ( FStrEq( pszEventName, PasstimeGameEvents::Score::s_eventName ) )
	{
		OnBallScore();
		PasstimeGameEvents::Score scoreEvent( pEvent );
		auto *pScorer = ToTFPlayer( UTIL_PlayerByIndex( scoreEvent.scorerIndex ) );
		auto *pAssister = ToTFPlayer( UTIL_PlayerByIndex( scoreEvent.assisterIndex ) );
		m_pEventText->EnqueueScore( pScorer, pAssister );
	}
}

//-----------------------------------------------------------------------------
void CTFHudPasstimeBallStatus::Reset() 
{
	m_bReset = true;

	// delay the actual reset for a bit because Reset is called before
	// the game entities are settled into their new state (e.g. the ball
	// is about to be deleted).
}

//-----------------------------------------------------------------------------
void CTFHudPasstimeBallStatus::OnBallFreeOther( C_TFPlayer *pOwner, 
	C_TFPlayer *pAttacker )
{
	Assert( m_bInitialized );	
	if ( m_pProgressBallCarrierName )
	{
		m_pProgressBallCarrierName->SetVisible( false );
	}
}

//-----------------------------------------------------------------------------
void CTFHudPasstimeBallStatus::OnBallFreeSelf( C_TFPlayer *pOwner, 
	C_TFPlayer *pAttacker )
{
	Assert( m_bInitialized );	
	if ( m_pProgressBallCarrierName )
	{
		m_pProgressBallCarrierName->SetVisible( false );
	}
}

//-----------------------------------------------------------------------------
void CTFHudPasstimeBallStatus::OnBallGetOther( int iPlayer )
{
	Assert( m_bInitialized );	
	Assert( g_PR );

	wchar_t wszFinalText[128];
	wchar_t wszPlayerName[MAX_PLAYER_NAME_LENGTH];
	wchar_t *pwszFormatString = g_pVGuiLocalize->Find( "#TF_Passtime_CarrierName" );
	if ( !pwszFormatString )
	{
		pwszFormatString = L"%s1";
	}
	g_pVGuiLocalize->ConvertANSIToUnicode( ( iPlayer > 0 ) ? g_PR->GetPlayerName( iPlayer ) : "", wszPlayerName, sizeof( wszPlayerName ) );
	g_pVGuiLocalize->ConstructString_safe( wszFinalText, pwszFormatString, 1, wszPlayerName );

	if ( m_pProgressBallCarrierName )
	{
		m_pProgressBallCarrierName->SetText( wszFinalText );
		m_pProgressBallCarrierName->SetVisible( true );
	}
}

//-----------------------------------------------------------------------------
void CTFHudPasstimeBallStatus::OnBallGetSelf( int iPlayer )
{
	Assert( m_bInitialized );	
	Assert( g_PR );

	wchar_t wszFinalText[128];
	wchar_t wszPlayerName[MAX_PLAYER_NAME_LENGTH];
	wchar_t *pwszFormatString = g_pVGuiLocalize->Find( "#TF_Passtime_CarrierName" );
	if ( !pwszFormatString )
	{
		pwszFormatString = L"%s1";
	}
	g_pVGuiLocalize->ConvertANSIToUnicode( ( iPlayer > 0 ) ? g_PR->GetPlayerName( iPlayer ) : "", wszPlayerName, sizeof( wszPlayerName ) );
	g_pVGuiLocalize->ConstructString_safe( wszFinalText, pwszFormatString, 1, wszPlayerName );

	if ( m_pProgressBallCarrierName )
	{
		m_pProgressBallCarrierName->SetText( wszFinalText );
		m_pProgressBallCarrierName->SetVisible( true );
	}
}

//-----------------------------------------------------------------------------
void CTFHudPasstimeBallStatus::OnBallScore()
{
	Assert( m_bInitialized );	
	if ( m_pProgressBallCarrierName )
	{
		m_pProgressBallCarrierName->SetVisible( false );
	}
}

//-----------------------------------------------------------------------------
// CTFHudPasstime
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
CTFHudPasstime::CTFHudPasstime( Panel *pParent ) 
	: CTFHudPasstimePanel( pParent, "HudPasstime" )
	, m_pBallStatus( new CTFHudPasstimeBallStatus( this ) )
	, m_pTeamScore( new CTFHudTeamScore( this ) )
	, m_pBallOffscreenArrow( new CTFHudPasstimeBallOffscreenArrow( this ) )
	, m_pPassNotify( new CTFHudPasstimePassNotify( this ) )
{
	for ( int i = 0; i < MAX_PLAYERS; ++i )
	{
		m_pPlayerArrows[i] = new CTFHudPasstimePlayerOffscreenArrow( this, i );
	}
}

//-----------------------------------------------------------------------------
CTFHudPasstime::~CTFHudPasstime()
{
	// this is only called when the game quits, so don't bother deleting members
}

//-----------------------------------------------------------------------------
void CTFHudPasstime::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );
	LoadControlSettings( "resource/UI/HudPasstime.res", NULL, NULL, NULL );
	vgui::ivgui()->AddTickSignal( GetVPanel() );
	OnTick();
}

//-----------------------------------------------------------------------------
void CTFHudPasstime::Reset() 
{
	if ( m_pBallStatus )
	{
		m_pBallStatus->Reset();
	}
}

//-----------------------------------------------------------------------------
void CTFHudPasstime::OnTick() 
{
	if ( m_pTeamScore ) 
	{
		m_pTeamScore->SetVisible( IsVisible() );
	}
}
