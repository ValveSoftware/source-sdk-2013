//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#include "cbase.h"
#include <vgui_controls/AnimationController.h>
#include <vgui_controls/EditablePanel.h>

#include "iclientmode.h"
#include "tf_time_panel.h"
#include "tf_gamerules.h"
#include "c_tf_team.h"
#include "vgui_controls/ScalableImagePanel.h"
#include "econ_controls.h"
#include "vgui/ISurface.h"
#include "tf_hud_arena_player_count.h"
#include "tf_match_description.h"
#include "tf_matchmaking_shared.h"

using namespace vgui;
extern ConVar tf_hud_show_servertimelimit;
extern ConVar tf_arena_round_time;

void AddSubKeyNamed( KeyValues *pKeys, const char *pszName );
bool ShouldUseMatchHUD();

DECLARE_BUILD_FACTORY( CTFProgressBar );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFProgressBar::CTFProgressBar( Panel *parent, const char *name ) 
	: ImagePanel( parent, name )
{
	m_flPercent = 0.0f;

	m_iTexture = surface()->DrawGetTextureId( "hud/objectives_timepanel_progressbar" );
	if ( m_iTexture == -1 ) // we didn't find it, so create a new one
	{
		m_iTexture = surface()->CreateNewTextureID();	
		surface()->DrawSetTextureFile( m_iTexture, "hud/objectives_timepanel_progressbar", true, false );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFProgressBar::Paint()
{
	int wide, tall;
	GetSize( wide, tall );

	float uv1 = 0.0f, uv2 = 1.0f;
	Vector2D uv11( uv1, uv1 );
	Vector2D uv21( uv2, uv1 );
	Vector2D uv22( uv2, uv2 );
	Vector2D uv12( uv1, uv2 );

	Vertex_t verts[4];	
	verts[0].Init( Vector2D( 0, 0 ), uv11 );
	verts[1].Init( Vector2D( wide, 0 ), uv21 );
	verts[2].Init( Vector2D( wide, tall ), uv22 );
	verts[3].Init( Vector2D( 0, tall ), uv12  );

	// first, just draw the whole thing inactive.
	surface()->DrawSetTexture( m_iTexture );
	surface()->DrawSetColor( m_clrInActive );
	surface()->DrawTexturedPolygon( 4, verts );

	// now, let's calculate the "active" part of the progress bar
	if ( m_flPercent < m_flPercentWarning )
	{
		surface()->DrawSetColor( m_clrActive );
	}
	else
	{
		surface()->DrawSetColor( m_clrWarning );
	}

	// we're going to do this using quadrants
	//  -------------------------
	//  |           |           |
	//  |           |           |
	//  |     4     |     1     |
	//  |           |           |
	//  |           |           |
	//  -------------------------
	//  |           |           |
	//  |           |           |
	//  |     3     |     2     |
	//  |           |           |
	//  |           |           |
	//  -------------------------

	float flCompleteCircle = ( 2.0f * M_PI );
	float fl90degrees = flCompleteCircle / 4.0f;

	float flEndAngle = flCompleteCircle * ( 1.0f - m_flPercent ); // count DOWN (counter-clockwise)
	//float flEndAngle = flCompleteCircle * m_flPercent; // count UP (clockwise)

	float flHalfWide = (float)wide / 2.0f;
	float flHalfTall = (float)tall / 2.0f;

	if ( flEndAngle >= fl90degrees * 3.0f ) // >= 270 degrees
	{
		// draw the first and second quadrants
		uv11.Init( 0.5f, 0.0f );
		uv21.Init( 1.0f, 0.0f );
		uv22.Init( 1.0f, 1.0f );
		uv12.Init( 0.5, 1.0f );

		verts[0].Init( Vector2D( flHalfWide, 0.0f ), uv11 );
		verts[1].Init( Vector2D( wide, 0.0f ), uv21 );
		verts[2].Init( Vector2D( wide, tall ), uv22 );
		verts[3].Init( Vector2D( flHalfWide, tall ), uv12  );

		surface()->DrawTexturedPolygon( 4, verts );

		// draw the third quadrant
		uv11.Init( 0.0f, 0.5f );
		uv21.Init( 0.5f, 0.5f );
		uv22.Init( 0.5f, 1.0f );
		uv12.Init( 0.0f, 1.0f );

		verts[0].Init( Vector2D( 0.0f, flHalfTall ), uv11 );
		verts[1].Init( Vector2D( flHalfWide, flHalfTall ), uv21 );
		verts[2].Init( Vector2D( flHalfWide, tall ), uv22 );
		verts[3].Init( Vector2D( 0.0f, tall ), uv12  );

		surface()->DrawTexturedPolygon( 4, verts );

		// draw the partial fourth quadrant
		if ( flEndAngle > fl90degrees * 3.5f ) // > 315 degrees
		{
			uv11.Init( 0.0f, 0.0f );
			uv21.Init( 0.5f - ( tan(fl90degrees * 4.0f - flEndAngle) * 0.5 ), 0.0f );
			uv22.Init( 0.5f, 0.5f );
			uv12.Init( 0.0f, 0.5f );

			verts[0].Init( Vector2D( 0.0f, 0.0f ), uv11 );
			verts[1].Init( Vector2D( flHalfWide - ( tan(fl90degrees * 4.0f - flEndAngle) * flHalfTall ), 0.0f ), uv21 );
			verts[2].Init( Vector2D( flHalfWide, flHalfTall ), uv22 );
			verts[3].Init( Vector2D( 0.0f, flHalfTall ), uv12 );

			surface()->DrawTexturedPolygon( 4, verts );
		}
		else // <= 315 degrees
		{
			uv11.Init( 0.0f, 0.5f );
			uv21.Init( 0.0f, 0.5f - ( tan(flEndAngle - fl90degrees * 3.0f) * 0.5 ) );
			uv22.Init( 0.5f, 0.5f );
			uv12.Init( 0.0f, 0.5f );

			verts[0].Init( Vector2D( 0.0f, flHalfTall ), uv11 );
			verts[1].Init( Vector2D( 0.0f, flHalfTall - ( tan(flEndAngle - fl90degrees * 3.0f) * flHalfWide ) ), uv21 );
			verts[2].Init( Vector2D( flHalfWide, flHalfTall ), uv22 );
			verts[3].Init( Vector2D( 0.0f, flHalfTall ), uv12  );

			surface()->DrawTexturedPolygon( 4, verts );
		}
	}
	else if ( flEndAngle >= fl90degrees * 2.0f ) // >= 180 degrees
	{
		// draw the first and second quadrants
		uv11.Init( 0.5f, 0.0f );
		uv21.Init( 1.0f, 0.0f );
		uv22.Init( 1.0f, 1.0f );
		uv12.Init( 0.5, 1.0f );

		verts[0].Init( Vector2D( flHalfWide, 0.0f ), uv11 );
		verts[1].Init( Vector2D( wide, 0.0f ), uv21 );
		verts[2].Init( Vector2D( wide, tall ), uv22 );
		verts[3].Init( Vector2D( flHalfWide, tall ), uv12  );

		surface()->DrawTexturedPolygon( 4, verts );

		// draw the partial third quadrant
		if ( flEndAngle > fl90degrees * 2.5f ) // > 225 degrees
		{
			uv11.Init( 0.5f, 0.5f );
			uv21.Init( 0.5f, 1.0f );
			uv22.Init( 0.0f, 1.0f );
			uv12.Init( 0.0f, 0.5f + ( tan(fl90degrees * 3.0f - flEndAngle) * 0.5 ) );

			verts[0].Init( Vector2D( flHalfWide, flHalfTall ), uv11 );
			verts[1].Init( Vector2D( flHalfWide, tall ), uv21 );
			verts[2].Init( Vector2D( 0.0f, tall ), uv22 );
			verts[3].Init( Vector2D( 0.0f, flHalfTall + ( tan(fl90degrees * 3.0f - flEndAngle) * flHalfWide ) ), uv12 );

			surface()->DrawTexturedPolygon( 4, verts );
		}
		else // <= 225 degrees
		{
			uv11.Init( 0.5f, 0.5f );
			uv21.Init( 0.5f, 1.0f );
			uv22.Init( 0.5f - ( tan( flEndAngle - fl90degrees * 2.0f) * 0.5 ), 1.0f );
			uv12.Init( 0.5f, 0.5f );

			verts[0].Init( Vector2D( flHalfWide, flHalfTall ), uv11 );
			verts[1].Init( Vector2D( flHalfWide, tall ), uv21 );
			verts[2].Init( Vector2D( flHalfWide - ( tan(flEndAngle - fl90degrees * 2.0f) * flHalfTall ), tall ), uv22 );
			verts[3].Init( Vector2D( flHalfWide, flHalfTall ), uv12  );

			surface()->DrawTexturedPolygon( 4, verts );
		}
	}
	else if ( flEndAngle >= fl90degrees ) // >= 90 degrees
	{
		// draw the first quadrant
		uv11.Init( 0.5f, 0.0f );
		uv21.Init( 1.0f, 0.0f );
		uv22.Init( 1.0f, 0.5f );
		uv12.Init( 0.5f, 0.5f );

		verts[0].Init( Vector2D( flHalfWide, 0.0f ), uv11 );
		verts[1].Init( Vector2D( wide, 0.0f ), uv21 );
		verts[2].Init( Vector2D( wide, flHalfTall ), uv22 );
		verts[3].Init( Vector2D( flHalfWide, flHalfTall ), uv12  );

		surface()->DrawTexturedPolygon( 4, verts );

		// draw the partial second quadrant
		if ( flEndAngle > fl90degrees * 1.5f ) // > 135 degrees
		{
			uv11.Init( 0.5f, 0.5f );
			uv21.Init( 1.0f, 0.5f );
			uv22.Init( 1.0f, 1.0f );
			uv12.Init( 0.5f + ( tan(fl90degrees * 2.0f - flEndAngle) * 0.5f ), 1.0f );

			verts[0].Init( Vector2D( flHalfWide, flHalfTall ), uv11 );
			verts[1].Init( Vector2D( wide, flHalfTall ), uv21 );
			verts[2].Init( Vector2D( wide, tall ), uv22 );
			verts[3].Init( Vector2D( flHalfWide + ( tan(fl90degrees * 2.0f - flEndAngle) * flHalfTall ), tall ), uv12  );

			surface()->DrawTexturedPolygon( 4, verts );
		}
		else // <= 135 degrees
		{
			uv11.Init( 0.5f, 0.5f );
			uv21.Init( 1.0f, 0.5f );
			uv22.Init( 1.0f, 0.5f + ( tan(flEndAngle - fl90degrees) * 0.5f ) );
			uv12.Init( 0.5f, 0.5f );

			verts[0].Init( Vector2D( flHalfWide, flHalfTall ), uv11 );
			verts[1].Init( Vector2D( wide, flHalfTall ), uv21 );
			verts[2].Init( Vector2D( wide, flHalfTall + ( tan(flEndAngle - fl90degrees) * flHalfWide ) ), uv22 );
			verts[3].Init( Vector2D( flHalfWide, flHalfTall ), uv12  );

			surface()->DrawTexturedPolygon( 4, verts );
		}
	}
	else // > 0 degrees
	{
		if ( flEndAngle > fl90degrees / 2.0f ) // > 45 degrees
		{
			uv11.Init( 0.5f, 0.0f );
			uv21.Init( 1.0f, 0.0f );
			uv22.Init( 1.0f, 0.5f - ( tan(fl90degrees - flEndAngle) * 0.5 ) );
			uv12.Init( 0.5f, 0.5f );

			verts[0].Init( Vector2D( flHalfWide, 0.0f ), uv11 );
			verts[1].Init( Vector2D( wide, 0.0f ), uv21 );
			verts[2].Init( Vector2D( wide, flHalfTall - ( tan(fl90degrees - flEndAngle) * flHalfWide ) ), uv22 );
			verts[3].Init( Vector2D( flHalfWide, flHalfTall ), uv12  );

			surface()->DrawTexturedPolygon( 4, verts );
		}
		else // <= 45 degrees
		{
			uv11.Init( 0.5f, 0.0f );
			uv21.Init( 0.5 + ( tan(flEndAngle) * 0.5 ), 0.0f );
			uv22.Init( 0.5f, 0.5f );
			uv12.Init( 0.5f, 0.0f );

			verts[0].Init( Vector2D( flHalfWide, 0.0f ), uv11 );
			verts[1].Init( Vector2D( flHalfWide + ( tan(flEndAngle) * flHalfTall ), 0.0f ), uv21 );
			verts[2].Init( Vector2D( flHalfWide, flHalfTall ), uv22 );
			verts[3].Init( Vector2D( flHalfWide, 0.0f ), uv12  );

			surface()->DrawTexturedPolygon( 4, verts );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFHudTimeStatus::CTFHudTimeStatus( Panel *parent, const char *name ) : EditablePanel( parent, name )
{
	m_pTimeValue = new CExLabel( this, "TimePanelValue", L"" );
	m_pProgressBar = NULL;
	m_pOvertimeLabel = NULL;
	m_pOvertimeBG = NULL;
	m_pSuddenDeathLabel = NULL;
	m_pSuddenDeathBG = NULL;
	m_pWaitingForPlayersBG = NULL;
	m_pWaitingForPlayersLabel = NULL;
	m_pSetupLabel = NULL;
	m_pSetupBG = NULL;
	m_pTimerBG = NULL;
	m_pServerTimeLabel = NULL;
	m_pServerTimeLabelBG = NULL;

	m_flNextThink = 0.0f;
	m_iTimerIndex = 0;

	m_iTimerDeltaHead = 0;
	for( int i = 0 ; i < NUM_TIMER_DELTA_ITEMS ; i++ )
	{
		m_TimerDeltaItems[i].m_flDieTime = 0.0f;
	}

	ListenForGameEvent( "teamplay_update_timer" );
	ListenForGameEvent( "teamplay_timer_time_added" );
	ListenForGameEvent( "localplayer_changeteam" );

	m_nTeam = TEAM_UNASSIGNED;
	m_bKothMode = false;
	m_bCachedOvertime = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudTimeStatus::SetTeamBackground( void )
{
	m_pTimerBG = dynamic_cast<ScalableImagePanel *>( FindChildByName( "TimePanelBG" ) );
	if ( m_pTimerBG )
	{
		if ( TFGameRules() && ( !TFGameRules()->IsInKothMode() || TFGameRules()->IsInWaitingForPlayers() ) )
		{
			int iTeam = GetLocalPlayerTeam();
			if ( iTeam == TF_TEAM_RED )
			{
				m_pTimerBG->SetImage( "../hud/objectives_timepanel_red_bg" );
			}
			else
			{
				m_pTimerBG->SetImage( "../hud/objectives_timepanel_blue_bg" );
			}
		}
		else
		{
			if ( m_nTeam == TF_TEAM_RED )
			{
				m_pTimerBG->SetImage( "../hud/objectives_timepanel_red_bg" );
			}
			else
			{
				m_pTimerBG->SetImage( "../hud/objectives_timepanel_blue_bg" );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudTimeStatus::FireGameEvent( IGameEvent *event )
{
	const char *eventName = event->GetName();

	if ( !Q_strcmp( eventName, "teamplay_update_timer" ) )
	{
		if ( TFGameRules() && TFGameRules()->IsInKothMode() )
		{
			InvalidateLayout( false, true );
		}

		SetExtraTimePanels();
	}
	else if ( !Q_strcmp( eventName, "teamplay_timer_time_added" ) )
	{
		int iIndex = event->GetInt( "timer", -1 );
		int nSeconds = event->GetInt( "seconds_added", 0 );

		SetTimeAdded( iIndex, nSeconds );
	}
	else if ( !Q_strcmp( eventName, "localplayer_changeteam" ) )
	{
		SetTeamBackground();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudTimeStatus::SetTimeAdded( int iIndex, int nSeconds )
{
	if (  m_iTimerIndex != iIndex ) // make sure this is the timer we're displaying in the HUD
		return;

	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( nSeconds != 0 && pPlayer )
	{
		// create a delta item that floats off the top
		timer_delta_t *pNewDeltaItem = &m_TimerDeltaItems[m_iTimerDeltaHead];

		m_iTimerDeltaHead++;
		m_iTimerDeltaHead %= NUM_TIMER_DELTA_ITEMS;

		pNewDeltaItem->m_flDieTime = gpGlobals->curtime + m_flDeltaLifetime;
		pNewDeltaItem->m_nAmount = nSeconds;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudTimeStatus::CheckClockLabelLength( CExLabel *pLabel, Panel *pBG )
{
	if ( !pLabel || ! pBG )
		return;

	int textWide, textTall;
	pLabel->GetContentSize( textWide, textTall );

	// make sure our string isn't longer than the label it's in
	if ( textWide > pLabel->GetWide() )
	{
		int xStart, yStart, wideStart, tallStart;
		pLabel->GetBounds( xStart, yStart, wideStart, tallStart );
		
		int newXPos = xStart + ( wideStart / 2.0f ) - ( textWide / 2.0f );
		pLabel->SetBounds(  newXPos, yStart, textWide, tallStart );
	}

	// turn off the background if our text label is wider than it is
	if ( pLabel->GetWide() > pBG->GetWide() )
	{
		pBG->SetVisible( false );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudTimeStatus::SetExtraTimePanels()
{
	if ( !TFGameRules() )
		return;

	CTeamRoundTimer *pTimer = dynamic_cast< CTeamRoundTimer* >( ClientEntityList().GetEnt( m_iTimerIndex ) );

	if ( pTimer && pTimer->IsStopWatchTimer() == true )
	{
		if( m_pTimerBG ) 
			m_pTimerBG->SetVisible( false );
		if( m_pProgressBar ) 
			m_pProgressBar->SetVisible( false );
		if( m_pWaitingForPlayersLabel ) 
			m_pWaitingForPlayersLabel->SetVisible( false );
		if( m_pWaitingForPlayersBG ) 
			m_pWaitingForPlayersBG->SetVisible( false );
		if( m_pOvertimeLabel ) 
			m_pOvertimeLabel->SetVisible( false );
		if( m_pOvertimeBG ) 
			m_pOvertimeBG->SetVisible( false );
		if( m_pSetupLabel ) 
			m_pSetupLabel->SetVisible( false );
		if( m_pSetupBG ) 
			m_pSetupBG->SetVisible( false );
		if( m_pSuddenDeathLabel )
			m_pSuddenDeathLabel->SetVisible( false );
		if( m_pSuddenDeathBG ) 
			m_pSuddenDeathBG->SetVisible( false );
		if( m_pServerTimeLabel ) 
			m_pServerTimeLabel->SetVisible( false );
		if ( m_pServerTimeLabelBG ) 
			m_pServerTimeLabelBG->SetVisible( false );
		return;
	}

	bool bInSetup = TFGameRules()->InSetup();
	bool bInWaitingForPlayers = TFGameRules()->IsInWaitingForPlayers();

	if ( m_pSetupBG && m_pSetupLabel )
	{
		// get the time remaining (in seconds)
		if ( pTimer )
		{
			m_pSetupBG->SetVisible( bInSetup );
			m_pSetupLabel->SetVisible( bInSetup );
		}
	}

	// Set the Sudden Death panels to be visible
	if ( m_pSuddenDeathBG && m_pSuddenDeathLabel )
	{
		bool bInSD = TFGameRules()->InStalemate() == true && TFGameRules()->IsInArenaMode() == false;

		if ( bInSD != m_pSuddenDeathLabel->IsVisible() )
		{
			if ( bInSD )
			{
				g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "SuddenDeathLabelPulseRed" );
			}
			else
			{
				g_pClientMode->GetViewportAnimationController()->StopAnimationSequence( this, "SuddenDeathLabelPulseRed" );
			}
		}

		m_pSuddenDeathBG->SetVisible( bInSD );
		m_pSuddenDeathLabel->SetVisible( bInSD );
	}

	if ( m_pOvertimeBG && m_pOvertimeLabel )
	{
		bool bInOver = TFGameRules()->InOvertime();

		if ( TFGameRules()->IsInKothMode() )
		{
			if ( pTimer )
			{
				if ( bInOver )
				{
					if ( pTimer->GetTimeRemaining() <= 0 )
					{
						bInOver = true;
						m_bCachedOvertime = true;
					}
					else
					{
						bInOver = false;
					}
				}
				else
				{
					if ( m_bCachedOvertime )
					{
						if ( pTimer->GetTimeRemaining() <= 0 )
						{
							bInOver = true;
						}
						else
						{
							m_bCachedOvertime = false;
						}
					}
				}
			}
		}

		if ( bInOver )
		{
			// need to turn off the SuddenDeath images if they're on
			if ( m_pSuddenDeathBG && m_pSuddenDeathLabel )
			{
				m_pSuddenDeathBG->SetVisible( false );
				m_pSuddenDeathLabel->SetVisible( false );
			}

			if ( !m_pOvertimeLabel->IsVisible() )
			{
				m_pOvertimeBG->SetVisible( true );
				m_pOvertimeLabel->SetVisible( true );
				g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( this, "OvertimeLabelPulseRed" );

				CheckClockLabelLength( m_pOvertimeLabel, m_pOvertimeBG );
			}
		}
		else
		{
			m_pOvertimeBG->SetVisible( false );
			m_pOvertimeLabel->SetVisible( false );
			g_pClientMode->GetViewportAnimationController()->StopAnimationSequence( this, "OvertimeLabelPulseRed" );
		}
	}

	if ( m_pWaitingForPlayersBG && m_pWaitingForPlayersLabel )
	{
		m_pWaitingForPlayersBG->SetVisible( bInWaitingForPlayers );
		m_pWaitingForPlayersLabel->SetVisible( bInWaitingForPlayers );

		if ( bInWaitingForPlayers )
		{
			// can't be waiting for players *AND* in setup at the same time
			if ( m_pSetupBG && m_pSetupLabel )
			{
				m_pSetupBG->SetVisible( false );
				m_pSetupLabel->SetVisible( false );
			}

			CheckClockLabelLength( m_pWaitingForPlayersLabel, m_pWaitingForPlayersBG );
		}
	}

	if ( m_pServerTimeLabel && m_pServerTimeLabelBG )
	{
		// This appears in the same space after SetUp and WaitingForPlayers is gone
		bool bDisplayServerTimerEnabled = tf_hud_show_servertimelimit.GetInt() && !bInSetup && !bInWaitingForPlayers;

		if ( m_pServerTimeLabel->IsVisible() != bDisplayServerTimerEnabled )
			m_pServerTimeLabel->SetVisible( bDisplayServerTimerEnabled );

		if ( m_pServerTimeLabelBG->IsVisible() != bDisplayServerTimerEnabled )
			m_pServerTimeLabelBG->SetVisible( bDisplayServerTimerEnabled );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudTimeStatus::Reset()
{
	m_flNextThink = gpGlobals->curtime + 0.05f;
	m_iTimerIndex = 0;

	m_iTimerDeltaHead = 0;
	for( int i = 0 ; i < NUM_TIMER_DELTA_ITEMS ; i++ )
	{
		m_TimerDeltaItems[i].m_flDieTime = 0.0f;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudTimeStatus::ApplySchemeSettings( IScheme *pScheme )
{
	KeyValues *pConditions = NULL;

	if ( TFGameRules() )
	{
		if ( ShouldUseMatchHUD() )
		{
			pConditions = new KeyValues( "conditions" );
			AddSubKeyNamed( pConditions, "if_match" );
		}
	}

	// load control settings...
	LoadControlSettings( "resource/UI/HudObjectiveTimePanel.res", NULL, NULL, pConditions );

	if ( pConditions )
	{
		pConditions->deleteThis();
	}

	m_pProgressBar = dynamic_cast<CTFProgressBar *>( FindChildByName( "TimePanelProgressBar" ) );

	m_pOvertimeLabel = dynamic_cast<CExLabel *>( FindChildByName( "OvertimeLabel" ) );
	m_pOvertimeBG = FindChildByName( "OvertimeBG" );

	m_pSuddenDeathLabel = dynamic_cast<CExLabel *>( FindChildByName( "SuddenDeathLabel" ) );
	m_pSuddenDeathBG = FindChildByName( "SuddenDeathBG" );

	m_pWaitingForPlayersLabel = dynamic_cast<CExLabel *>( FindChildByName( "WaitingForPlayersLabel" ) );
	m_pWaitingForPlayersBG = FindChildByName("WaitingForPlayersBG" );

	m_pSetupLabel = dynamic_cast<CExLabel *>( FindChildByName( "SetupLabel" ) );
	m_pSetupBG = FindChildByName("SetupBG" );

	m_pTimerBG = dynamic_cast<ScalableImagePanel *>( FindChildByName( "TimePanelBG" ) );
	SetTeamBackground();

	m_pServerTimeLabel = dynamic_cast< CExLabel* >( FindChildByName( "ServerTimeLimitLabel" ) );
	m_pServerTimeLabelBG = FindChildByName("ServerTimeLimitLabelBG" );

	m_flNextThink = 0.0f;
	m_iTimerIndex = 0;

	SetExtraTimePanels();

	BaseClass::ApplySchemeSettings( pScheme );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudTimeStatus::OnThink()
{
	if ( m_flNextThink < gpGlobals->curtime )
	{
		CTeamRoundTimer *pTimer = dynamic_cast< CTeamRoundTimer* >( ClientEntityList().GetEnt( m_iTimerIndex ) );

		if ( TFGameRules() )
		{
			if ( m_bKothMode != TFGameRules()->IsInKothMode() )
			{
				m_bKothMode = TFGameRules()->IsInKothMode();
				InvalidateLayout( false, true );
			}
		}
		
		// get the time remaining (in seconds)
		if ( pTimer )
		{
			int nTotalTime = pTimer->GetTimerMaxLength();
			int nTimeRemaining = pTimer->GetTimeRemaining();
			int nTimeToDisplay = nTimeRemaining;

			if ( !pTimer->ShowTimeRemaining() )
			{
				nTimeToDisplay = nTotalTime - nTimeToDisplay;
			}

			if ( m_pTimeValue && m_pTimeValue->IsVisible() )
			{
				// set our label
				int nMinutes = 0;
				int nSeconds = 0;
				char temp[256];

				if ( nTimeToDisplay <= 0 )
				{
					nMinutes = 0;
					nSeconds = 0;

					if ( TFGameRules() && TFGameRules()->IsInKothMode() )
					{
						SetExtraTimePanels();
					}
				}
				else
				{
					nMinutes = nTimeToDisplay / 60;
					nSeconds = nTimeToDisplay % 60;
				}				

				Q_snprintf( temp, sizeof( temp ), "%d:%02d", nMinutes, nSeconds );
				m_pTimeValue->SetText( temp );
			}
	
			// let the progress bar know the percentage of time that's passed ( 0.0 -> 1.0 )
			if ( m_pProgressBar && m_pProgressBar->IsVisible() )
			{
				if ( nTotalTime == 0 )
				{
					m_pProgressBar->SetPercentage( 0 );
				}
				else 
				{
					m_pProgressBar->SetPercentage( ( (float)nTotalTime - nTimeRemaining ) / (float)nTotalTime );
				}
			}

			// Optional display of mp_timelimit on HUD
			if ( m_pServerTimeLabel && m_pServerTimeLabelBG )
			{
				int nServerTimeLimit = mp_timelimit.GetInt() * 60;

				bool bDisplayServerTimerEnabled = tf_hud_show_servertimelimit.GetInt() && 
												  TFGameRules() && 
												  !TFGameRules()->InSetup() &&
												  !TFGameRules()->IsInWaitingForPlayers() &&
												  pTimer->IsRoundMaxTimerSet() &&
												  nServerTimeLimit;
				
				if ( m_pServerTimeLabel->IsVisible() != bDisplayServerTimerEnabled )
				{
					m_pServerTimeLabel->SetVisible( bDisplayServerTimerEnabled );
				}
				if ( m_pServerTimeLabelBG->IsVisible() != bDisplayServerTimerEnabled )
				{
					m_pServerTimeLabelBG->SetVisible( bDisplayServerTimerEnabled );
				}

				// Only display server timelimit when the round timer isn't showing mp_timelimit (e.g. ctf_2fort)
				if ( bDisplayServerTimerEnabled )
				{
					wchar_t wzServerTimeHrsLeft[128];
					wchar_t wzServerTimeMinLeft[128];
					wchar_t wzServerTimeSecLeft[128];
					wchar_t wzServerTimeLeft[128];

					int iTimeLeft = 0;
					int iHours = 0;
					int iMinutes = 0;
					int iSeconds = 0;

					if ( !nServerTimeLimit )
					{
						g_pVGuiLocalize->ConstructString_safe( wzServerTimeLeft, g_pVGuiLocalize->Find( "#TF_HUD_ServerNoTimeLimit" ), 0);
						SetDialogVariable( "servertimeleft", wzServerTimeLeft );
						return;
					}

					iTimeLeft = ( TFGameRules() && TFGameRules()->GetTimeLeft() > 0 ) ? TFGameRules()->GetTimeLeft() : 0;

					if ( iTimeLeft == 0 )
					{
						g_pVGuiLocalize->ConstructString_safe( wzServerTimeLeft, g_pVGuiLocalize->Find( "#TF_HUD_ServerChangeOnRoundEnd" ), 0);
						SetDialogVariable( "servertimeleft", wzServerTimeLeft );
						return;
					}

					iHours = iTimeLeft / 3600;
					iMinutes = (iTimeLeft % 3600) / 60;
					iSeconds = (iTimeLeft % 60);

					_snwprintf( wzServerTimeHrsLeft, ARRAYSIZE( wzServerTimeHrsLeft ), L"%i", iHours );
					_snwprintf( wzServerTimeMinLeft, ARRAYSIZE( wzServerTimeMinLeft ), L"%02i", iMinutes );
					_snwprintf( wzServerTimeSecLeft, ARRAYSIZE( wzServerTimeSecLeft ), L"%02i", iSeconds );

					if (iHours == 0)
					{
						g_pVGuiLocalize->ConstructString_safe( wzServerTimeLeft, g_pVGuiLocalize->Find( "#TF_HUD_ServerTimeLeftNoHours" ), 2, wzServerTimeMinLeft, wzServerTimeSecLeft);
						SetDialogVariable( "servertimeleft", wzServerTimeLeft );
						return;
					}

					g_pVGuiLocalize->ConstructString_safe( wzServerTimeLeft, g_pVGuiLocalize->Find( "#TF_HUD_ServerTimeLeft" ), 3, wzServerTimeHrsLeft, wzServerTimeMinLeft, wzServerTimeSecLeft);
					SetDialogVariable( "servertimeleft", wzServerTimeLeft );
				}
			}
		}

		m_flNextThink = gpGlobals->curtime + 0.1f;
	}

	if ( TFGameRules() && TFGameRules()->IsInArenaMode() == true && tf_arena_round_time.GetInt() > 0 )
	{
		CHudArenaPlayerCount *pPlayerCount = ( CHudArenaPlayerCount * )GET_HUDELEMENT( CHudArenaPlayerCount );

		int iSelfX, iSelfY;
		GetPos( iSelfX, iSelfY );

		if ( pPlayerCount && pPlayerCount->IsVisible() )
		{
			int iX, iY, iTall, iWide;
			pPlayerCount->GetBounds( iX, iY, iWide, iTall );
			
			SetPos( iSelfX, iY + iTall - YRES( 12 ) );
		}
		else
		{
			SetPos( iSelfX, 0 );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Paint the deltas
//-----------------------------------------------------------------------------
void CTFHudTimeStatus::Paint( void )
{
	BaseClass::Paint();

	for ( int i = 0 ;  i < NUM_TIMER_DELTA_ITEMS ; i++ )
	{
		// update all the valid delta items
		if ( m_TimerDeltaItems[i].m_flDieTime > gpGlobals->curtime )
		{
			// position and alpha are determined from the lifetime
			// color is determined by the delta - green for positive, red for negative

			Color c = ( m_TimerDeltaItems[i].m_nAmount > 0 ) ? m_DeltaPositiveColor : m_DeltaNegativeColor;

			float flLifetimePercent = ( m_TimerDeltaItems[i].m_flDieTime - gpGlobals->curtime ) / m_flDeltaLifetime;

			// fade out after half our lifetime
			if ( flLifetimePercent < 0.5 )
			{
				c[3] = (int)( 255.0f * ( flLifetimePercent / 0.5 ) );
			}

			float flHeight = ( m_flDeltaItemStartPos - m_flDeltaItemEndPos );
			float flYPos = m_flDeltaItemEndPos + flLifetimePercent * flHeight;

			surface()->DrawSetTextFont( m_hDeltaItemFont );
			surface()->DrawSetTextColor( c );
			surface()->DrawSetTextPos( m_flDeltaItemX, (int)flYPos );

			wchar_t wBuf[20];
			int nMinutes, nSeconds;
			int nClockTime = ( m_TimerDeltaItems[i].m_nAmount > 0 ) ? m_TimerDeltaItems[i].m_nAmount : ( m_TimerDeltaItems[i].m_nAmount * -1 );
			nMinutes = nClockTime / 60;
			nSeconds = nClockTime % 60;

			if ( m_TimerDeltaItems[i].m_nAmount > 0 )
			{
				V_swprintf_safe( wBuf, L"+%d:%02d", nMinutes, nSeconds );
			}
			else
			{
				V_swprintf_safe( wBuf, L"-%d:%02d", nMinutes, nSeconds );
			}

			surface()->DrawPrintText( wBuf, wcslen(wBuf), FONT_DRAW_NONADDITIVE );
		}
	}
}

DECLARE_HUDELEMENT( CTFHudKothTimeStatus );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFHudKothTimeStatus::CTFHudKothTimeStatus( const char *pElementName )
	: CHudElement( pElementName )
	, BaseClass( NULL, "HudKothTimeStatus" )
	, m_pBluePanel( NULL )
	, m_pRedPanel( NULL )
	, m_pActiveTimerBG( NULL )
	, m_nActiveTeam( TEAM_UNASSIGNED )
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	m_pBluePanel = new CTFHudTimeStatus( this, "BlueTimer" );
	m_pRedPanel = new CTFHudTimeStatus( this, "RedTimer" );

	if ( m_pRedPanel )
	{
		m_pRedPanel->SetTeam( TF_TEAM_RED );
	}

	SetHiddenBits( 0 );

	RegisterForRenderGroup( "mid" );
	RegisterForRenderGroup( "commentary" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFHudKothTimeStatus::~CTFHudKothTimeStatus()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFHudKothTimeStatus::ShouldDraw()
{
	if ( TFGameRules() )
	{
		if ( TFGameRules()->ShowMatchSummary() )
			return false;

		if ( TFGameRules()->IsInKothMode() && !TeamplayRoundBasedRules()->IsInWaitingForPlayers() )
		{
			C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
			if ( pPlayer )
			{
				if ( pPlayer->GetObserverMode() != OBS_MODE_FREEZECAM )
				{
					return CHudElement::ShouldDraw();
				}
			}
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudKothTimeStatus::SetVisible( bool bVisible )
{
	BaseClass::SetVisible( bVisible );

	if ( ShouldUseMatchHUD() )
	{
		UpdateActiveTeam();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudKothTimeStatus::Reset()
{
	if ( m_pBluePanel )
	{
		m_pBluePanel->Reset();
	}

	if ( m_pRedPanel )
	{
		m_pRedPanel->Reset();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudKothTimeStatus::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	KeyValues *pConditions = NULL;

	if ( TFGameRules() )
	{
		if ( ShouldUseMatchHUD() )
		{
			pConditions = new KeyValues( "conditions" );
			AddSubKeyNamed( pConditions, "if_match" );
		}
	}

	LoadControlSettings( "resource/UI/HudObjectiveKothTimePanel.res", NULL, NULL, pConditions );

	m_pActiveTimerBG = FindChildByName( "ActiveTimerBG" );

	UpdateActiveTeam();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudKothTimeStatus::UpdateActiveTeam( void )
{
	if ( ShouldUseMatchHUD() )
	{
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( m_pRedPanel, m_nActiveTeam == TF_TEAM_RED ? "ActiveTimerHighlight" : "ActiveTimerDim", false );
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( m_pBluePanel, m_nActiveTeam == TF_TEAM_BLUE ? "ActiveTimerHighlight" : "ActiveTimerDim", false );
	}
	else if ( m_pActiveTimerBG )
	{
		if ( m_nActiveTeam != TEAM_UNASSIGNED )
		{
			if ( !m_pActiveTimerBG->IsVisible() )
			{
				m_pActiveTimerBG->SetVisible( true );
			}

			m_pActiveTimerBG->SetAlpha( 255 );

			int xNewPos = m_nBlueActiveXPos;
			if ( m_nActiveTeam == TF_TEAM_RED )
			{
				xNewPos = m_nRedActiveXPos;
			}

			int xPos, yPos;
			m_pActiveTimerBG->GetPos( xPos, yPos );
			m_pActiveTimerBG->SetPos( xNewPos, yPos );

			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "ActiveTimerBGPulse" );
		}
		else
		{
			if ( m_pActiveTimerBG->IsVisible() )
			{
				m_pActiveTimerBG->SetVisible( false );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudKothTimeStatus::Think( void )
{
	if ( !TFGameRules() || !IsVisible() )
		return;
	
	int nActiveTeam = TEAM_UNASSIGNED;

	CTeamRoundTimer *pTimer = TFGameRules()->GetBlueKothRoundTimer();
	if ( pTimer )
	{
		if ( m_pBluePanel )
		{
			if ( pTimer->entindex() != m_pBluePanel->GetTimerIndex() )
			{
				m_pBluePanel->SetTimerIndex( pTimer->entindex() );
			}
		}

		if ( !pTimer->IsTimerPaused() )
		{
			nActiveTeam = TF_TEAM_BLUE;
		}
	}

	pTimer = TFGameRules()->GetRedKothRoundTimer();
	if ( pTimer )
	{
		if ( m_pRedPanel )
		{
			if ( pTimer->entindex() != m_pRedPanel->GetTimerIndex() )
			{
				m_pRedPanel->SetTimerIndex( pTimer->entindex() );
			}
		}

		if ( !pTimer->IsTimerPaused() )
		{
			nActiveTeam = TF_TEAM_RED;
		}
	}

	if ( nActiveTeam != m_nActiveTeam )
	{
		m_nActiveTeam = nActiveTeam;
		UpdateActiveTeam();
	}
}
