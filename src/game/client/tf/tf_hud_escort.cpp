//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#include "cbase.h"
#include "tf_hud_escort.h"
#include <vgui/IVGui.h>
#include "tf_hud_freezepanel.h"
#include "hud_controlpointicons.h"
#include "teamplayroundbased_gamerules.h"
#include "c_team_train_watcher.h"
#include "iclientmode.h"
#include "tf_gamerules.h"

using namespace vgui;

#define TF_ESCORT_RECEDE_COUNTDOWN	20.0f

#define TF_ESCORT_HILL_MATERIAL			"hud/cart_track_arrow"
#define TF_ESCORT_HILL_ALPHA_CYCLE_TIME	750

ConVar hud_escort_interp( "hud_escort_interp", "0.2" );
ConVar hud_escort_test_progress( "hud_escort_test_progress", "-1" );
ConVar hud_escort_test_speed( "hud_escort_test_speed", "-1" );

void AddSubKeyNamed( KeyValues *pKeys, const char *pszName )
{
	KeyValues *pNewKey = new KeyValues( pszName );
	if ( pNewKey )
	{
		pKeys->AddSubKey( pNewKey );
	}	
}

CEscortHillPanel::CEscortHillPanel( vgui::Panel *parent, const char *name ) : vgui::Panel( parent, name )
{
	m_iTexture = vgui::surface()->DrawGetTextureId( TF_ESCORT_HILL_MATERIAL );
	if ( m_iTexture == -1 ) // we didn't find it, so create a new one
	{
		m_iTexture = vgui::surface()->CreateNewTextureID();	
		vgui::surface()->DrawSetTextureFile( m_iTexture, TF_ESCORT_HILL_MATERIAL, true, false );
	}

	m_flUVScroll = 0.0f;
	m_flUVW = 0.0f;
	m_bOnHill = false;
	m_bFadingOut = true;
	m_bDownhill = false;

	ListenForGameEvent( "teamplay_round_start" );

	vgui::ivgui()->AddTickSignal( GetVPanel(), TF_ESCORT_HILL_ALPHA_CYCLE_TIME );
}

void CEscortHillPanel::OnTick( void )
{
	if ( IsVisible() == false )
		return;

	if ( m_bOnHill )
	{
		if ( m_bFadingOut )
		{
			GetAnimationController()->RunAnimationCommand( this, "alpha", 32, 0.0, (float)TF_ESCORT_HILL_ALPHA_CYCLE_TIME / 1000, vgui::AnimationController::INTERPOLATOR_LINEAR );
			m_bFadingOut = false;
		}
		else
		{
			GetAnimationController()->RunAnimationCommand( this, "alpha", 96, 0.0, (float)TF_ESCORT_HILL_ALPHA_CYCLE_TIME / 1000, vgui::AnimationController::INTERPOLATOR_LINEAR );
			m_bFadingOut = true;
		}
	}
	else
	{
		SetAlpha( 64 );
		m_bFadingOut = true;
	}
}

void CEscortHillPanel::PerformLayout( void )
{
	BaseClass::PerformLayout();

	int nPanelX, nPanelY;
	GetBounds( nPanelX, nPanelY, m_nPanelWide, m_nPanelTall );

	int nTextureWide, nTextureTall;
	surface()->DrawGetTextureSize( m_iTexture, nTextureWide, nTextureTall );

	float flScale = (float)m_nPanelTall / (float)nTextureTall;
	float flNewTextureWide = nTextureWide * flScale;

	m_flUVW = m_nPanelWide / flNewTextureWide;

	SetAlpha( 64 );
}

void CEscortHillPanel::Paint( void )
{
	BaseClass::Paint();

	if ( ObjectiveResource() )
	{
		m_bOnHill = ObjectiveResource()->IsTrainOnHill( m_nTeam, m_nHill );
		m_bDownhill = ObjectiveResource()->IsHillDownhill( m_nTeam, m_nHill );
	}

	if ( m_bOnHill )
	{
		m_flUVScroll += 0.02;

		if ( m_flUVScroll > 1.0 )
		{
			m_flUVScroll -= 1.0;
		}
	}

	Vertex_t vert[4];
	surface()->DrawSetTexture( m_iTexture );

	Vector2D uv11( m_flUVScroll, 0 );
	Vector2D uv21( m_flUVW + m_flUVScroll, 0 );
	Vector2D uv22( m_flUVW + m_flUVScroll, 1 );
	Vector2D uv12( m_flUVScroll, 1 );

	if ( m_bDownhill )
	{
		vert[0].Init( Vector2D( 0, 0 ), uv21 );
		vert[1].Init( Vector2D( m_nPanelWide, 0 ), uv11 );
		vert[2].Init( Vector2D( m_nPanelWide, m_nPanelTall ), uv12 );				
		vert[3].Init( Vector2D( 0, m_nPanelTall ), uv22 );
	}
	else
	{
		vert[0].Init( Vector2D( 0, 0 ), uv11 );
		vert[1].Init( Vector2D( m_nPanelWide, 0 ), uv21 );
		vert[2].Init( Vector2D( m_nPanelWide, m_nPanelTall ), uv22 );				
		vert[3].Init( Vector2D( 0, m_nPanelTall ), uv12 );
	}

	surface()->DrawSetColor( Color(255,255,255,255) );
	surface()->DrawTexturedPolygon( 4, vert );
}

void CEscortHillPanel::FireGameEvent( IGameEvent * event )
{
	if ( FStrEq( "teamplay_round_start", event->GetName() ) )
	{
		m_flUVScroll = 0.0f;
	}
}

//========================================================================================================================
// Escort status indicator that floats above the train when you are interacting with it
//========================================================================================================================
CEscortStatusTeardrop::CEscortStatusTeardrop(Panel *parent) : vgui::EditablePanel( parent, "EscortTeardrop" )
{
	m_pBarText = new vgui::Label( this, "ProgressText", "" );
	m_pTeardrop = new CIconPanel( this, "Teardrop" );
	m_pBlocked = new CIconPanel( this, "Blocked" );
	m_pCapping = new ImagePanel( this, "Capping" );
	m_iOrgHeight = 0;
	m_iMidGroupIndex = -1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEscortStatusTeardrop::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	//LoadControlSettings( "resource/UI/EscortStatusTeardrop.res" );		/*//ControlPointProgressBar.res" );*/

	m_iOrgHeight = GetTall();

	m_iMidGroupIndex = gHUD.LookupRenderGroupIndexByName( "mid" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CEscortStatusTeardrop::IsVisible( void )
{
	if ( IsInFreezeCam() == true )
		return false;

	if ( m_iMidGroupIndex != -1 && gHUD.IsRenderGroupLockedFor( NULL, m_iMidGroupIndex ) )
		return false;

	return BaseClass::IsVisible();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEscortStatusTeardrop::SetupForPoint( int iCPIndex )
{
	if ( !ObjectiveResource() )
		return;

	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return;

	if ( iCPIndex == -1 )
	{
		SetVisible( false );
		return;
	}

	bool bInWinState = TeamplayRoundBasedRules() ? TeamplayRoundBasedRules()->RoundHasBeenWon() : false;

	if ( !bInWinState )	
	{
		SetVisible( true );

		int iCappingTeam = ObjectiveResource()->GetCappingTeam( iCPIndex );
		int iOwnerTeam = ObjectiveResource()->GetOwningTeam( iCPIndex );
		int iPlayerTeam = pPlayer->GetTeamNumber();
		bool bCapBlocked = ObjectiveResource()->CapIsBlocked( iCPIndex );

		if ( !bCapBlocked && iCappingTeam != TEAM_UNASSIGNED && iCappingTeam != iOwnerTeam && iCappingTeam == iPlayerTeam )
		{
			// it's not blocked and we're capping

			m_pBlocked->SetVisible( false );

			// replace this with a capping icon
			m_pCapping->SetVisible( true );

			m_pBarText->SetVisible( false );
		}
		else
		{
			// not capping 

			m_pCapping->SetVisible( false );

			m_pBlocked->SetVisible( true );
			UpdateBarText( iCPIndex );
		}

		InvalidateLayout();
	}
	else
	{
		SetVisible( false );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEscortStatusTeardrop::UpdateBarText( int iCPIndex )
{
	if ( !ObjectiveResource() )
		return;

	// If we're not escorting the train we don't show text
	if ( iCPIndex == -1 )
	{
		m_pBarText->SetVisible( false );
		return;
	}

	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer || !m_pBarText )
		return;

	m_pBarText->SetVisible( true );
	
	int iCappingTeam = ObjectiveResource()->GetCappingTeam( iCPIndex );
	int iPlayerTeam = pPlayer->GetTeamNumber();

	if ( !TeamplayGameRules()->PointsMayBeCaptured() )
	{
		m_pBarText->SetText( "#Team_Capture_NotNow" );
		return;
	}

	// Player is blocking an enemy capture
	if ( iCappingTeam != TEAM_UNASSIGNED && iCappingTeam != iPlayerTeam )
	{
		if ( ObjectiveResource()->IsCPBlocked( iCPIndex ) )
		{
			m_pBarText->SetText( "#Team_Blocking_Capture" );
			return;
		}
	}

	// player is standing on an already owned cap
	if ( ObjectiveResource()->GetOwningTeam( iCPIndex ) == iPlayerTeam )
	{
		m_pBarText->SetText( "#Team_Capture_OwnPoint" );
		return;
	}

	// disguised spies etc can't cap or block
	char szReason[256];
	if ( !TeamplayGameRules()->PlayerMayCapturePoint( pPlayer, iCPIndex, szReason, sizeof(szReason) ) )
	{
		m_pBarText->SetText( szReason );
		return;
	}

	// we can't capture because it's blocked
	if ( iCappingTeam == iPlayerTeam )
	{
		m_pBarText->SetText( "#Team_Progress_Blocked" );
		return;
	}

	m_pBarText->SetText( "" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFHudEscortProgressBar::CTFHudEscortProgressBar( vgui::Panel *parent, const char *name ) : vgui::ImagePanel( parent, name )
{
	m_flPercent = 0.0f;

	SetTeam( TF_TEAM_BLUE ); // default to blue
}

//-----------------------------------------------------------------------------
// Purpose: Update the escort image position
//-----------------------------------------------------------------------------
void CTFHudEscortProgressBar::SetTeam( int nTeam )
{
	m_nTeam = nTeam;

	const char *pszMaterial = "hud/cart_track_neutral_opaque";

	if ( m_nTeam == TF_TEAM_RED )
	{
		pszMaterial = "hud/cart_track_red_opaque";
	}
	else if ( m_nTeam == TF_TEAM_BLUE )
	{
		pszMaterial = "hud/cart_track_blue_opaque";
	}

	m_iTexture = vgui::surface()->DrawGetTextureId( pszMaterial );
	if ( m_iTexture == -1 ) // we didn't find it, so create a new one
	{
		m_iTexture = vgui::surface()->CreateNewTextureID();	
	}

	vgui::surface()->DrawSetTextureFile( m_iTexture, pszMaterial, true, false );
}

//-----------------------------------------------------------------------------
// Purpose: Update the escort image position
//-----------------------------------------------------------------------------
void CTFHudEscortProgressBar::Paint()
{
 	if ( m_flPercent > 0.0f )
 	{
		Vertex_t vert[4];
		float uv1 = 0.0f;
		float uv2 = 1.0f;

		surface()->DrawSetTexture( m_iTexture );

		Vector2D uv11( uv1, uv1 );
		Vector2D uv21( uv2, uv1 );
		Vector2D uv22( uv2, uv2 );
		Vector2D uv12( uv1, uv2 );

		int nBarX, nBarY, nBarW, nBarH;
		GetBounds( nBarX, nBarY, nBarW, nBarH );

		int nMiddle = (int)( (float)(nBarW) * m_flPercent );

		vert[0].Init( Vector2D( 0, 0 ), uv11 );
		vert[1].Init( Vector2D( nMiddle, 0 ), uv21 );
		vert[2].Init( Vector2D( nMiddle, nBarH ), uv22 );				
		vert[3].Init( Vector2D( 0, nBarH ), uv12 );

		surface()->DrawSetColor( Color(255,255,255,210) );
		surface()->DrawTexturedPolygon( 4, vert );
	  
		surface()->DrawSetColor( Color(245,229,196,210) );
		surface()->DrawLine( nMiddle - 1, 0, nMiddle - 1, nBarH );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFHudEscort::CTFHudEscort( Panel *parent, const char *name ) : EditablePanel( parent, name ), m_flProgressHistory("CTFHudEscort::m_flProgressHistory")
{
	m_flProgress = 0.0f;
	m_flProgressHistory.Setup( &m_flProgress, 0 );

	ListenForGameEvent( "escort_speed" );
	ListenForGameEvent( "escort_progress" );
	ListenForGameEvent( "escort_recede" );
	ListenForGameEvent( "controlpoint_initialized" );
	ListenForGameEvent( "controlpoint_updateimages" );
	ListenForGameEvent( "controlpoint_updatecapping" );
	ListenForGameEvent( "controlpoint_starttouch" );
	ListenForGameEvent( "controlpoint_endtouch" );
	ListenForGameEvent( "teamplay_round_start" );
	ListenForGameEvent( "localplayer_respawn" );
	ListenForGameEvent( "localplayer_changeteam" );

	m_pEscortItemPanel = new EditablePanel( this, "EscortItemPanel" );

	Assert( m_pEscortItemPanel );

	m_pSpeedBackwards = new ImagePanel( m_pEscortItemPanel, "Speed_Backwards" );
	m_pCapPlayerImage = new ImagePanel( m_pEscortItemPanel, "CapPlayerImage" );
	m_pCapNumPlayers = new CExLabel( m_pEscortItemPanel, "CapNumPlayers", "" );
	m_pEscortItemImage = new ImagePanel( m_pEscortItemPanel, "EscortItemImage" );
	m_pEscortItemImageBottom = new ImagePanel( m_pEscortItemPanel, "EscortItemImageBottom" );
	m_pBlocked = new ImagePanel( m_pEscortItemPanel, "Blocked" );

	m_pEscortItemImageAlert = new ImagePanel( m_pEscortItemPanel, "EscortItemImageAlert" );
	
	m_pHomeCPIcon = new ImagePanel( this, "HomeCPIcon" );

	m_pCPTemplate = new ImagePanel( this, "SimpleControlPointTemplate" );

	// store the background bar so we can use its dimensions for CP placement
	m_pLevelBar = new ImagePanel( this, "LevelBar" );

	m_pStatus = new CEscortStatusTeardrop( m_pEscortItemPanel );

	m_pHilightSwoop = new CControlPointIconSwoop( this, "EscortHilightSwoop" );
	m_pHilightSwoop->SetParent( g_pClientMode->GetViewport() );
	m_pHilightSwoop->SetZPos( 10 );
	m_pHilightSwoop->SetShouldScaleImage( true );

	m_pProgressBar = new CTFHudEscortProgressBar( this, "ProgressBar" );

	m_iCurrentCP = -1;

	m_flRecedeTime = 0;

	m_nTeam = TF_TEAM_BLUE; // blue team by default
	m_bTopPanel = true;

	m_bAlarm = false;

	vgui::ivgui()->AddTickSignal( GetVPanel(), 100 );

	m_bMultipleTrains = false;
	m_nNumHills = 0;

	// blue material for multiple tracks colored overlay
	m_iBlueMaterialIndex = surface()->DrawGetTextureId( "hud/cart_track_blue" );
	if ( m_iBlueMaterialIndex == -1 ) // we didn't find it, so create a new one
	{
		m_iBlueMaterialIndex = surface()->CreateNewTextureID();	
	}

	surface()->DrawSetTextureFile( m_iBlueMaterialIndex, "hud/cart_track_blue", true, false );

	// red material for multiple tracks colored overlay
	m_iRedMaterialIndex = surface()->DrawGetTextureId( "hud/cart_track_red" );
	if ( m_iRedMaterialIndex == -1 ) // we didn't find it, so create a new one
	{
		m_iRedMaterialIndex = surface()->CreateNewTextureID();	
	}

	surface()->DrawSetTextureFile( m_iRedMaterialIndex, "hud/cart_track_red", true, false );

	for ( int i = 0 ; i < TEAM_TRAIN_MAX_HILLS ; i++ )
	{
		char szPanelName[32];
		Q_snprintf( szPanelName, sizeof(szPanelName), "hill_panel%d", i );
		m_pHillPanels[i] = new CEscortHillPanel( this, szPanelName );

		if ( m_pHillPanels[i] )
		{
			m_pHillPanels[i]->SetHillNumber( i );
			m_pHillPanels[i]->SetTeamNumber( m_nTeam );
		}
	}
}

CTFHudEscort::~CTFHudEscort()
{
	if ( m_pHilightSwoop )
	{
		m_pHilightSwoop->MarkForDeletion();
		m_pHilightSwoop = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Hide when we take a freeze cam shot
//-----------------------------------------------------------------------------
bool CTFHudEscort::IsVisible( void )
{
	if( IsTakingAFreezecamScreenshot() )
		return false;

	if ( IsInFreezeCam() )
		return false;

	if ( !m_bHaveValidPointPositions )
		return false;

	return BaseClass::IsVisible();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudEscort::PerformLayout( void )
{
	BaseClass::PerformLayout();

	for ( int i = 0 ; i < TEAM_TRAIN_MAX_HILLS ; i++ )
	{
		if ( m_pHillPanels[i] )
		{
			m_pHillPanels[i]->SetTeamNumber( m_nTeam );
			m_pHillPanels[i]->SetVisible( false );
		}
	}

	if ( ObjectiveResource() )
	{
		if ( m_nNumHills > 0 )
		{
			if ( m_pLevelBar )
			{
				int xpos, ypos, wide, tall, zpos;
				m_pLevelBar->GetBounds( xpos, ypos, wide, tall );
				zpos = m_pLevelBar->GetZPos() + 1;

				for ( int i = 0 ; i < m_nNumHills ; i++ )
				{
					CEscortHillPanel *pPanel = m_pHillPanels[i];

					if ( pPanel )
					{
						float flStart = 0, flEnd = 0;
						ObjectiveResource()->GetHillData( m_nTeam, i, flStart, flEnd );

						pPanel->SetVisible( true );

						int xOffset1 = wide * flStart;
						int xOffset2 = wide * flEnd;

						pPanel->SetBounds( xpos + xOffset1, ypos, xOffset2 - xOffset1, tall );
						pPanel->SetZPos( zpos );
					}
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudEscort::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	KeyValues *pConditions = NULL;
	if ( m_nTeam > LAST_SHARED_TEAM )
	{
		pConditions = new KeyValues( "conditions" );
		if ( pConditions )
		{
			if ( m_nTeam == TF_TEAM_RED )
			{
				AddSubKeyNamed( pConditions, "if_team_red" );
			}
			else
			{
				AddSubKeyNamed( pConditions, "if_team_blue" );
			}

			if ( m_bMultipleTrains )
			{
				AddSubKeyNamed( pConditions, "if_multiple_trains" );

				if ( m_nTeam == TF_TEAM_RED )
				{
					AddSubKeyNamed( pConditions, "if_multiple_trains_red" );
				}
				else
				{
					AddSubKeyNamed( pConditions, "if_multiple_trains_blue" );
				}

				if ( m_bTopPanel )
				{
					AddSubKeyNamed( pConditions, "if_multiple_trains_top" );
				}
				else
				{
					AddSubKeyNamed( pConditions, "if_multiple_trains_bottom" );
				}
			}
			else
			{
				if ( m_nNumHills > 0 )
				{
					// we have a single track map with hills
					AddSubKeyNamed( pConditions, "if_single_with_hills" );

					if ( m_nTeam == TF_TEAM_RED )
					{
						AddSubKeyNamed( pConditions, "if_single_with_hills_red" );
					}
					else
					{
						AddSubKeyNamed( pConditions, "if_single_with_hills_blue" );
					}
				}
			}
		}
	}

	// load control settings...
	LoadControlSettings( "resource/UI/ObjectiveStatusEscort.res", NULL, NULL, pConditions );

	UpdateCPImages();

	if ( m_pEscortItemImage && m_pEscortItemImageBottom )
	{
		if ( m_bMultipleTrains == false )
		{
			m_pEscortItemImageBottom->SetVisible( false );
			m_pEscortItemImage->SetVisible( true );
		}
		else
		{
			m_pEscortItemImageBottom->SetVisible( !m_bTopPanel );
			m_pEscortItemImage->SetVisible( m_bTopPanel );
		}
	}

	if ( pConditions )
	{
		pConditions->deleteThis();
	}

	m_bAlarm = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudEscort::UpdateAlarmAnimations( void )
{
	if ( m_bMultipleTrains )
	{
		if ( m_pEscortItemImageAlert )
		{
			if ( m_bAlarm )
			{
				if ( m_pEscortItemImageAlert->IsVisible() == false )
				{
					m_pEscortItemImageAlert->SetVisible( true );
				}

				g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( m_pEscortItemPanel, "HudCartAlarmPulse" );
			}
			else
			{
				if ( m_pEscortItemImageAlert->IsVisible() == true )
				{
					m_pEscortItemImageAlert->SetVisible( false );
				}

				g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( m_pEscortItemPanel, "eventHudCartAlarmPulseStop" );
			}
		}
	}
	else
	{
		if ( m_pEscortItemImageAlert && m_pEscortItemImageAlert->IsVisible() == true )
		{
			m_pEscortItemImageAlert->SetVisible( false );
		}

		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( m_pEscortItemPanel, "eventHudCartAlarmPulseStop" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Update the escort image position
//-----------------------------------------------------------------------------
void CTFHudEscort::OnTick()
{
	// don't need to do this on non-escort maps (unless we're trying to override the HUD type)
	if ( TFGameRules() && ( TFGameRules()->GetGameType() != TF_GAMETYPE_ESCORT ) && ( TFGameRules()->GetHUDType() != TF_HUDTYPE_ESCORT ) )
		return;

	if ( !BaseClass::IsVisible() ) // intentionally skipping our version of IsVisible() to bypass the !m_bHaveValidPointPositions check
		return;

	bool bInvalidateLayout = false;

	if ( TFGameRules() )
	{
		if ( m_bMultipleTrains != TFGameRules()->HasMultipleTrains() )
		{
			// if we don't match, reload our settings
			m_bMultipleTrains = TFGameRules()->HasMultipleTrains();
			bInvalidateLayout = true;
		}
	}

	if ( ObjectiveResource() )
	{
		if ( m_nNumHills != ObjectiveResource()->GetNumNodeHillData( m_nTeam ) )
		{
			m_nNumHills = ObjectiveResource()->GetNumNodeHillData( m_nTeam );
			bInvalidateLayout = true;
		}
	}

	if ( bInvalidateLayout )
	{
		InvalidateLayout( false, true );
	}

	// if we haven't found any valid points yet, keep trying until we do...
	if ( !m_bHaveValidPointPositions )
	{
		UpdateCPImages();
	}

	bool bAlarm = ObjectiveResource() && ObjectiveResource()->GetTrackAlarm( m_nTeam );
	if ( bAlarm != m_bAlarm )
	{
		m_bAlarm = bAlarm;
		UpdateAlarmAnimations();
	}

	// The escort item position approaches its actual pos
	int x, y, w, h;
	m_pEscortItemPanel->GetBounds( x, y, w, h );

	m_flProgressHistory.Interpolate( gpGlobals->curtime, hud_escort_interp.GetFloat() );

	float flProgress = m_flProgress;

	if ( hud_escort_test_progress.GetFloat() >= 0 )
	{
		flProgress = hud_escort_test_progress.GetFloat();
	}

	int iBarX, iBarY, iBarW, iBarH;
	m_pLevelBar->GetBounds( iBarX, iBarY, iBarW, iBarH );	

	int newX = iBarX + (int)( (float)(iBarW) * m_flProgress );

	int iSwoopX = newX - XRES(12);

	newX -= ( w / 2 );

	m_pEscortItemPanel->SetPos( newX, y );

	if ( m_bShowSwoop )
	{
		C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();

		// Tell the cap highlight image to fire up if it's our point being capped
		if ( m_pHilightSwoop && pPlayer && pPlayer->GetTeamNumber() > LAST_SHARED_TEAM )
		{
			if ( IsVisible() )
			{
				ConVarRef cl_hud_minmode( "cl_hud_minmode", true );

				int parentX, parentY;
				GetPos( parentX, parentY );

				int nYOffset = YRES(138);
				int nXOffset = 0;
				int nWide = XRES(24);
				int nTall = YRES(100);

				if ( m_bMultipleTrains )
				{
					nWide = XRES(20);
					nTall = YRES(70);

					if ( pPlayer->GetTeamNumber() == m_nTeam )
					{
						// top bar
 						nYOffset = YRES(97);
						nXOffset = XRES(2);
					}
					else
					{
						// bottom bar
						nYOffset = YRES(61);
						nXOffset = XRES(2);
					}
				}
				else
				{
					if ( cl_hud_minmode.IsValid() && cl_hud_minmode.GetBool() )
					{
						nYOffset = YRES(96);
						nXOffset = XRES(4);
						nWide = XRES(16);
						nTall = YRES(70);
					}
				}

				int nMidBarY = iBarY + ( iBarH / 2 );
				m_pHilightSwoop->SetPos( parentX + iSwoopX + nXOffset, parentY + nMidBarY - nYOffset );
				m_pHilightSwoop->SetSize( nWide, nTall );
				m_pHilightSwoop->SetVisible( true );
				m_pHilightSwoop->StartSwoop();
			}
		}

		m_bShowSwoop = false;
	}

//	char value[64];

	// 5 second countdown to receding
	float flSecondsToRecede = MAX( 0, ( m_flRecedeTime <= 0 ) ? 0 : m_flRecedeTime - gpGlobals->curtime );
	if ( flSecondsToRecede > 0.0f && flSecondsToRecede <= TF_ESCORT_RECEDE_COUNTDOWN )
	{
		int iDisplaySeconds = (int)( flSecondsToRecede ) + 1;
		m_pEscortItemPanel->SetDialogVariable( "recede", VarArgs( "%d", iDisplaySeconds ) );

		// we should not be showing the blocked image if we're showing the countdown
		m_pBlocked->SetVisible( false );
	}
	else
	{
		m_pEscortItemPanel->SetDialogVariable( "recede", "" );
	}

	// Debug string
// 	Q_snprintf( value, sizeof(value), "speed: %d  progress: %.3f recede %.1f\n", m_iSpeedLevel, flProgress, flSecondsToRecede );
// 	SetDialogVariable( "progress", value );

	if ( m_bMultipleTrains )
	{
		if ( m_pProgressBar && !m_pProgressBar->IsVisible() )
		{
			m_pProgressBar->SetVisible( true );
		}

		m_pProgressBar->SetPercentage( m_flProgress );
	}
	else
	{
		if ( m_pProgressBar && m_pProgressBar->IsVisible() )
		{
			m_pProgressBar->SetVisible( false );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Receive messages about changes in state
//-----------------------------------------------------------------------------
void CTFHudEscort::FireGameEvent( IGameEvent *event )
{	
	const char *eventName = event->GetName();
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();

	if ( !ObjectiveResource() )
		return;

	// make sure we care about this message
	int iTeam = event->GetInt( "team", TEAM_UNASSIGNED );
	if ( iTeam != TEAM_UNASSIGNED && ( iTeam != m_nTeam ) )
		return;

	if ( FStrEq( eventName, "escort_progress" ) )
	{
		m_flProgress = event->GetFloat( "progress" );

		// so we don't interp back to the start position on reset
		if ( event->GetBool( "reset" ) )
		{
			m_flProgressHistory.ClearHistory();
		}

		m_flProgressHistory.NoteChanged( gpGlobals->curtime, false );
	}
	else if ( FStrEq( eventName, "escort_speed" ) )
	{
		int iSpeedLevel = event->GetInt( "speed" );
		int nPlayers = event->GetInt( "players" );

		m_pSpeedBackwards->SetVisible( iSpeedLevel < 0 );

		bool bShowPlayers = ( iSpeedLevel > 0 ) && ( nPlayers > 0 );
		m_pCapPlayerImage->SetVisible( bShowPlayers );
		m_pCapNumPlayers->SetVisible( bShowPlayers );
		m_pEscortItemPanel->SetDialogVariable( "numcappers", nPlayers );

		// make sure the point isn't being marked as blocked when we're showing players or moving backwards
		if ( bShowPlayers || ( iSpeedLevel < 0 ) )
		{
			m_pBlocked->SetVisible( false );
		}

		m_pEscortItemPanel->InvalidateLayout();

		if ( iSpeedLevel > 0 && m_iSpeedLevel == 0 )
		{
			m_bShowSwoop = true;
		}

		m_iSpeedLevel = iSpeedLevel;
	}
	if ( FStrEq( eventName, "escort_recede" ) )
	{
		m_flRecedeTime = event->GetFloat( "recedetime" );
	}
	else if ( FStrEq( eventName, "controlpoint_initialized" ) )
	{
		UpdateCPImages();
	}
	else if ( FStrEq( eventName, "controlpoint_updateimages" ) )
	{
		// Update the images of our control point icons
		int iIndex = event->GetInt( "index" );

		bool bIsAnyCapBlocked = false;

		// Only invalidate the specified cap point
		// unless iIndex is -1, then do them all
		for (int i = 0; i < m_Icons.Count(); i++)
		{
			if ( iIndex != -1 && m_Icons[i]->GetCapIndex() != iIndex )
				continue;

			m_Icons[i]->SetForceOpaqueImages( !m_bMultipleTrains && ( m_nNumHills > 0 ) );
			m_Icons[i]->UpdateImage();

			if ( ObjectiveResource()->IsCPBlocked( m_Icons[i]->GetCapIndex() ) )
			{
				bIsAnyCapBlocked = true;
			}
		}

		// update our teardrop status
		UpdateStatusTeardropFor( m_iCurrentCP );

		// Assume that there is only one cap area linked to the escort item
		m_pBlocked->SetVisible( bIsAnyCapBlocked );
	}
	else if ( FStrEq( "controlpoint_updatecapping", eventName ) )
	{
		// Update the capping status of our control point icons
		int iIndex = event->GetInt( "index" );	
		UpdateStatusTeardropFor( iIndex );
		return;
	}
	else if ( FStrEq( "controlpoint_starttouch", eventName ) )
	{
		int iPlayer = event->GetInt( "player" );
		if ( pPlayer && iPlayer == pPlayer->entindex() )
		{
			m_iCurrentCP = event->GetInt( "area" );
			UpdateStatusTeardropFor( m_iCurrentCP );
		}
	}
	else if ( FStrEq( "controlpoint_endtouch", eventName ) )
	{
		int iPlayer = event->GetInt( "player" );
		if ( pPlayer && iPlayer == pPlayer->entindex() )
		{
			m_iCurrentCP = -1;
			UpdateStatusTeardropFor( m_iCurrentCP );
		}
	}
	else if ( FStrEq( "teamplay_round_start", eventName ) )
	{
		InvalidateLayout();
	}
	else if ( FStrEq( "localplayer_respawn", eventName ) )
	{
		// reset the alarm (will be restarted in OnThink() if needed)
		m_bAlarm = false;
		UpdateAlarmAnimations();
	}
	else if ( FStrEq( "localplayer_changeteam", eventName ) )
	{
		// reset the alarm (will be restarted in OnThink() if needed)
		m_bAlarm = false;
		UpdateAlarmAnimations();
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudEscort::UpdateStatusTeardropFor( int iIndex )
{
	if ( !ObjectiveResource() )
		return;

	if ( m_bMultipleTrains )
	{
		// don't draw teardrops for points that aren't in the local group the player cares about (group numbers map to the teams)
		if ( iIndex != -1 && ( ObjectiveResource()->GetCPGroup( iIndex ) != ( m_nTeam - 2 ) ) ) // -2 to offset the team numbers to 0
			return;

		// don't draw teardrops if you're not the same team as the local player (top bar is the only one to use the teamdrop)
		C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
		if ( pPlayer )
		{
			if ( m_nTeam != pPlayer->GetTeamNumber() )
				return;
		}
	}

	if ( iIndex == -1 )
	{
		iIndex = m_iCurrentCP;
	}

	// Ignore requests to display progress bars for points we're not standing on
	if ( ( m_iCurrentCP != iIndex ) )
		return;

	if ( iIndex >= ObjectiveResource()->GetNumControlPoints() )
	{
		iIndex = -1;
	}

	if ( m_pStatus )
	{
		m_pStatus->SetupForPoint( iIndex );
	}
}

void CTFHudEscort::UpdateCPImages( void )
{
	if ( !ObjectiveResource() )
		return;

	// delete any existing control points
	for ( int i = 0; i < m_Icons.Count(); i++)
	{
		m_Icons[i]->MarkForDeletion();
	}
	m_Icons.RemoveAll();

	Assert( m_pCPTemplate );

	// dummy pos
	int x, y, w, h;
	m_pCPTemplate->GetBounds( x, y, w, h );
	int nZPos = m_pCPTemplate->GetZPos();

	//bool bSetHome = false;

	int iValidPointPos = 0;		// hax, how many points are not at 0.0f

	// Create an icon for each visible control point in this miniround
	int iPoints = ObjectiveResource()->GetNumControlPoints();
	for ( int i = 0; i < iPoints; i++ )
	{
		bool bValid = true;

		if ( !ObjectiveResource()->IsInMiniRound(i) || 
			 !ObjectiveResource()->IsCPVisible(i) ||
			 ( m_bMultipleTrains && ( ObjectiveResource()->GetCPGroup( i ) != ( m_nTeam - 2 ) ) ) ) // -2 to offset the team numbers to 0
		{
			bValid = false;
		}

		if ( bValid )
		{
			CSimpleControlPoint *pIcon = new CSimpleControlPoint( this, "SimpleControlPointTemplate", i );
			pIcon->SetName( VarArgs("cp_%d", i ) );

			m_Icons.AddToTail( vgui::SETUP_PANEL(pIcon) );

			float flDist = ObjectiveResource()->GetPathDistance(i);

			int iBarX, iBarY, iBarW, iBarH;
			m_pLevelBar->GetBounds( iBarX, iBarY, iBarW, iBarH );	

			int newX = iBarX + (int)( (float)iBarW * flDist );

			newX -= ( w / 2 );

			pIcon->SetBounds( newX, y, w, h );
			pIcon->SetZPos( nZPos );
			pIcon->InvalidateLayout();

			if ( flDist > 0 )
			{
				iValidPointPos++;
			}

			pIcon->SetForceOpaqueImages( !m_bMultipleTrains && ( m_nNumHills > 0 ) );
			pIcon->UpdateImage();
		}
		
	}

	// we don't want to draw if we haven't got good point positions yet
	m_bHaveValidPointPositions = ( iValidPointPos > 0 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFHudMultipleEscort::CTFHudMultipleEscort( Panel *parent, const char *name ) : EditablePanel( parent, name )
{
	m_pBluePanel = new CTFHudEscort( this, "BlueEscortPanel" );
	m_pRedPanel = new CTFHudEscort( this, "RedEscortPanel" );

	if ( m_pRedPanel )
	{
		m_pRedPanel->SetTeam( TF_TEAM_RED );
	}

	ListenForGameEvent( "localplayer_changeteam" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFHudMultipleEscort::~CTFHudMultipleEscort()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudMultipleEscort::SetVisible( bool state )
{
	if ( m_pBluePanel )
	{
		m_pBluePanel->SetVisible( state );
	}

	if ( m_pRedPanel )
	{
		m_pRedPanel->SetVisible( state );
	}

	BaseClass::SetVisible( state );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudMultipleEscort::Reset()
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
void CTFHudMultipleEscort::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	KeyValues *pConditions = NULL;
	if ( TFGameRules() && TFGameRules()->HasMultipleTrains() )
	{
		pConditions = new KeyValues( "conditions" );
		if ( pConditions )
		{
			C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
			int iPlayerTeam = pPlayer ? pPlayer->GetTeamNumber() : TEAM_UNASSIGNED;

			if ( iPlayerTeam == TF_TEAM_BLUE )
			{
				AddSubKeyNamed( pConditions, "if_blue_is_top" );
			}
			else
			{
				AddSubKeyNamed( pConditions, "if_red_is_top" );
			}
		}
	}

	// load control settings...
	LoadControlSettings( "resource/UI/ObjectiveStatusMultipleEscort.res", NULL, NULL, pConditions );

	if ( pConditions )
	{
		pConditions->deleteThis();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudMultipleEscort::FireGameEvent( IGameEvent * event )
{
	if ( FStrEq( "localplayer_changeteam", event->GetName() ) )
	{
		if ( m_pRedPanel && m_pBluePanel )
		{
			C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
			int iPlayerTeam = pPlayer ? pPlayer->GetTeamNumber() : TEAM_UNASSIGNED;

			if ( iPlayerTeam == TF_TEAM_BLUE )
			{
				m_pBluePanel->SetTopPanel( true );
				m_pRedPanel->SetTopPanel( false );
			}
			else
			{
				m_pBluePanel->SetTopPanel( false );
				m_pRedPanel->SetTopPanel( true );
			}
		}

		InvalidateLayout( false, true );

		if ( m_pRedPanel && m_pBluePanel )
		{
			m_pRedPanel->InvalidateLayout( false, true );
			m_pBluePanel->InvalidateLayout( false, true );
		}
	}
}
