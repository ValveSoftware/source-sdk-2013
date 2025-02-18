//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "hud_controlpointicons.h"
#include "teamplayroundbased_gamerules.h"
#include "iclientmode.h"
#include "c_team_objectiveresource.h"
#include "c_playerresource.h"
#include "c_baseplayer.h"
#include "VGuiMatSurface/IMatSystemSurface.h"
#include "hud_macros.h"
#include "spectatorgui.h"
#include "c_team.h"
#include "tf_hud_freezepanel.h"
#include "tf_hud_objectivestatus.h"

#ifdef TF_CLIENT_DLL
#include "tf_gamerules.h"
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CControlPointIconPulseable::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	if ( !m_pPulseImage )
	{
		m_pPulseImage = scheme()->GetImage( "../sprites/obj_icons/icon_obj_white", true );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CControlPointIconPulseable::OnSizeChanged(int newWide, int newTall)
{
	if ( m_pPulseImage )
	{
		// scaling, force the image size to be our size
		m_pPulseImage->SetSize(newWide, newTall);
	}
	BaseClass::OnSizeChanged(newWide, newTall);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CControlPointIconPulseable::PaintBackground( void )
{
	if ( IsInFreezeCam() == true )
		return;

	if ( GetImage() )
	{
		SetAlpha(255);
		BaseClass::PaintBackground();
	}

	if ( m_flStartCapAnimStart && gpGlobals->curtime > m_flStartCapAnimStart )
	{
		float flElapsedTime = (gpGlobals->curtime - m_flStartCapAnimStart);

		// Pulse the white over the underlying color
		float flPulseSpeed = 20;
		if ( m_bAccelerateOverCapture )
		{
			float flCapPercentage = ObjectiveResource()->GetCPCapPercentage( m_iCPIndex );
			flPulseSpeed = RemapValClamped( flCapPercentage, 0, 1, 2, 5 );
		}

		float flPulseMod = fabs(sin( flElapsedTime * flPulseSpeed ));
		SetAlpha( 255 * flPulseMod );

		int wide, tall;
		GetSize( wide, tall );

		// Have to reset these - we're only referencing a material so the
		// size can be changed by CControlPointIconCapturePulse on a successful cap
		m_pPulseImage->SetPos( 0, 0 );
		m_pPulseImage->SetSize( wide, tall );

		m_pPulseImage->Paint();

		// Stop if we're only supposed to do this for a short time
		if ( m_flPulseTime && flElapsedTime >= m_flPulseTime )
		{
			StopPulsing();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CControlPointIconPulseable::StartPulsing( float flDelay, float flPulseTime, bool bAccelerate )
{
	m_flStartCapAnimStart = gpGlobals->curtime + flDelay;
	m_bAccelerateOverCapture = bAccelerate;
	m_flPulseTime = flPulseTime;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CControlPointIconPulseable::StopPulsing( void )
{
	m_flStartCapAnimStart = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CControlPointIcon::CControlPointIcon( Panel *parent, const char *pName, int iIndex ) : vgui::EditablePanel( parent, "ControlPointIcon" ), CHudElement( pName )
{
	SetHiddenBits( HIDEHUD_MISCSTATUS );

	m_iCPIndex = iIndex;
	m_pBaseImage = NULL;
	m_pOverlayImage = NULL;
	m_pCapImage = NULL;
	m_pCapHighlightImage = NULL;
	m_pCapPulseImage = NULL;
	m_pCapPlayerImage = NULL;
	m_pCapNumPlayers = NULL;
	m_bSwipeUp = false;
	m_flStartCapAnimStart = 0;
	m_iCapProgressDir = CP_DIR_N;
	m_iPrevCappers = 0;
	m_pCountdown = NULL;
	m_pCPTimerLabel = NULL;
	m_pCPTimerBG = NULL;
	m_flCPTimerTime = -1.0;
	m_bRedText = false;

	ListenForGameEvent( "controlpoint_unlock_updated" );
	ListenForGameEvent( "controlpoint_timer_updated" );

	ivgui()->AddTickSignal( GetVPanel(), 150 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CControlPointIcon::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	m_cRegularColor = pScheme->GetColor( "TanLight", Color( 235, 226, 202 ) );
	m_cHighlightColor = pScheme->GetColor( "RedSolid", Color( 192, 28, 0 ) );

	if ( !m_pCapHighlightImage )
	{
		m_pCapHighlightImage = new CControlPointIconSwoop( this, "CapHighlightImage" );
		m_pCapHighlightImage->SetParent( g_pClientMode->GetViewport() );
		m_pCapHighlightImage->SetZPos( 10 );
		m_pCapHighlightImage->SetShouldScaleImage( true );
	}

	if ( !m_pCapPulseImage )
	{
		m_pCapPulseImage = new CControlPointIconCapturePulse( this, "CapPulse" );
		m_pCapPulseImage->SetParent( g_pClientMode->GetViewport() );
		m_pCapPulseImage->SetZPos( -1 );
		m_pCapPulseImage->SetVisible( false );
		m_pCapPulseImage->SetShouldScaleImage( true );
	}

	if ( !m_pBaseImage )
	{
		m_pBaseImage = new CControlPointIconPulseable( this, "BaseImage", m_iCPIndex );
		m_pBaseImage->SetShouldScaleImage( true );
	}

	if ( !m_pCapImage )
	{
		m_pCapImage = new CControlPointIconCapArrow( this, this, "CapImage" );
		m_pCapImage->SetZPos( 2 );
		m_pCapImage->SetVisible( false );
	}

	if ( !m_pCountdown )
	{
		m_pCountdown = new CControlPointCountdown( this, "Countdown" );
		m_pCountdown->SetZPos( 4 );
		m_pCountdown->SetVisible( true );
	}

	if ( !m_pCPTimerLabel )
	{
		m_pCPTimerLabel = new CExLabel( this, "CPTimerLabel", L"" );
		m_pCPTimerLabel->SetZPos( 0 );
	}

	if ( !m_pCPTimerBG )
	{
		m_pCPTimerBG = new vgui::ImagePanel( this, "CPTimerBG" );
		m_pCPTimerBG->SetZPos( -1 );
		m_pCPTimerBG->SetShouldScaleImage( true );
	}

	LoadControlSettings( "resource/UI/ControlPointIcon.res" );

	m_pCapPlayerImage = dynamic_cast<vgui::ImagePanel *>( FindChildByName("CapPlayerImage") );
	m_pCapNumPlayers = dynamic_cast<vgui::Label *>( FindChildByName("CapNumPlayers") );
	m_pOverlayImage = dynamic_cast<vgui::ImagePanel *>( FindChildByName("OverlayImage") );

	if ( m_pCPTimerLabel )
	{
		m_pCPTimerLabel->SetParent( GetParent() );
	}

	if ( m_pCPTimerBG )
	{
		m_pCPTimerBG->SetParent( GetParent() );
	}

	UpdateImage();
	UpdateCapImage();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CControlPointIcon::FireGameEvent( IGameEvent *event )
{
	const char *pszEventName = event->GetName();

	if ( FStrEq( pszEventName, "controlpoint_unlock_updated" ) )
	{
		int iIndex = event->GetInt( "index" );
		if ( iIndex == m_iCPIndex )
		{
			float flTime = event->GetFloat( "time" );
			SetUnlockTime( flTime );
		}
	}
	else if ( FStrEq( pszEventName, "controlpoint_timer_updated" ) )
	{
		int iIndex = event->GetInt( "index" );
		if ( iIndex == m_iCPIndex )
		{
			float flTime = event->GetFloat( "time" );
			SetTimerTime( flTime );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CControlPointIcon::~CControlPointIcon( void )
{
	if ( m_pCapHighlightImage )
	{
		m_pCapHighlightImage->MarkForDeletion();
		m_pCapHighlightImage = NULL;
	}

	if ( m_pCapPulseImage )
	{
		m_pCapPulseImage->MarkForDeletion();
		m_pCapPulseImage = NULL;
	}

	if ( m_pCPTimerLabel )
	{
		m_pCPTimerLabel->MarkForDeletion();
		m_pCPTimerLabel = NULL;
	}

	if ( m_pCPTimerBG )
	{
		m_pCPTimerBG->MarkForDeletion();
		m_pCPTimerBG = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CControlPointIcon::UpdateImage( void )
{
	if ( !ObjectiveResource() )
		return;

	int iOwner = ObjectiveResource()->GetOwningTeam( m_iCPIndex );

	if ( m_pBaseImage )
	{
		int iOwnerIcon = ObjectiveResource()->GetCPCurrentOwnerIcon( m_iCPIndex, iOwner );
		const char *szMatName = GetMaterialNameFromIndex( iOwnerIcon );

		if ( IsPointLocked() && !IsPointUnlockCountdownRunning() )
		{
			m_pBaseImage->SetImage( VarArgs("..\\%s_locked", szMatName ) );
		}
		else
		{
			m_pBaseImage->SetImage( VarArgs("..\\%s", szMatName ) );
		}
	}

	if ( m_pOverlayImage )
	{
		int iOverlayIcon = ObjectiveResource()->GetOverlayForTeam( m_iCPIndex, iOwner );
		if ( iOverlayIcon )
		{
			const char *szMatName = GetMaterialNameFromIndex( iOverlayIcon );
			m_pOverlayImage->SetImage( VarArgs("..\\%s", szMatName ) );
			m_pOverlayImage->SetVisible( true );
		}
		else
		{
			m_pOverlayImage->SetVisible( false );
		}
	}

	// Whenever a successful cap occurs, flash the cap point
	if ( m_pCapPulseImage )
	{
		if ( m_iPrevCappers != 0 && iOwner == m_iPrevCappers )
		{
			m_iPrevCappers = 0;

			if ( ShouldDraw() )
			{
				m_pCapPulseImage->SetVisible( true );
				m_pCapPulseImage->StartPulse( gpGlobals->curtime, GetWide() );
			}
			m_pBaseImage->StartPulsing( FINISHCAPANIM_SWOOP_LENGTH, 0.5, false );
		}
		
	}
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CControlPointIcon::UpdateCapImage( void )
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return;

	if ( m_pCapImage )
	{
		int iCappingTeam = ObjectiveResource()->GetCappingTeam( m_iCPIndex );
		int iOwningTeam = ObjectiveResource()->GetOwningTeam( m_iCPIndex );

		if ( iCappingTeam != TEAM_UNASSIGNED && iCappingTeam != iOwningTeam )
		{
			const char *pszCapSwipe = ObjectiveResource()->GetGameSpecificCPCappingSwipe( m_iCPIndex, iCappingTeam );
			if ( m_bSwipeUp )
			{
				m_pCapImage->SetImage( VarArgs("%s_up",pszCapSwipe)  );
			}
			else
			{
				m_pCapImage->SetImage( pszCapSwipe );
			}
			m_pCapImage->SetVisible( true );

			// Tell the cap highlight image to fire up if it's our point being capped
			if ( m_pCapHighlightImage && pPlayer->GetTeamNumber() != iCappingTeam && pPlayer->GetTeamNumber() > LAST_SHARED_TEAM )
			{
				if ( ShouldDraw() && GetParent() && GetParent()->IsVisible() )
				{
					m_pCapHighlightImage->SetVisible( true );
					m_pCapHighlightImage->StartSwoop();
				}
				m_pBaseImage->StartPulsing( STARTCAPANIM_ICON_SWITCH, 0, true );
			}
			else
			{
				m_pBaseImage->StartPulsing( 0, 0, true );
			}

			if ( m_pCapPlayerImage )
			{
				m_pCapPlayerImage->SetVisible( true );
			}

			m_iPrevCappers = iCappingTeam;
			InvalidateLayout( true );
		}
		else
		{
			m_pBaseImage->StopPulsing();
			m_pCapImage->SetVisible( false );
			if ( m_pCapHighlightImage )
			{
				m_pCapHighlightImage->SetVisible( false );
			}
			if ( m_pCapPlayerImage )
			{
				m_pCapPlayerImage->SetVisible( false );
			}
			if ( m_pCapNumPlayers )
			{
				m_pCapNumPlayers->SetVisible( false );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Lock cap points when neither team can cap them for map-specific reasons
//-----------------------------------------------------------------------------
bool CControlPointIcon::IsPointLocked( void )
{
	bool bAnyTeamCanCap = false;
	for ( int gameteam = FIRST_GAME_TEAM; gameteam < GetNumberOfTeams(); gameteam++ )
	{
		// Ignore teams that already own the point
		if ( ObjectiveResource()->GetOwningTeam(m_iCPIndex) != gameteam )
		{
			if ( (ObjectiveResource()->TeamCanCapPoint( m_iCPIndex, gameteam)) )
			{
				if ( TeamplayGameRules()->TeamMayCapturePoint( gameteam, m_iCPIndex ) )
				{
					bAnyTeamCanCap = true;
				}
			}
		}
	}

	return ( !bAnyTeamCanCap );
}

//-----------------------------------------------------------------------------
// Purpose: Lock cap points when neither team can cap them for map-specific reasons
//-----------------------------------------------------------------------------
bool CControlPointIcon::IsPointUnlockCountdownRunning( void )
{
	if ( m_pCountdown && ( TeamplayRoundBasedRules() && !TeamplayRoundBasedRules()->IsInWaitingForPlayers() ) )
	{
		if ( m_pCountdown->GetUnlockTime() > 0 )
		{
			int nTimeToUnlock = m_pCountdown->GetUnlockTime() - gpGlobals->curtime;
			if ( nTimeToUnlock < 6 )
			{
				return true;
			}
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Used by the intro to fake the pulsing of this icon
//-----------------------------------------------------------------------------
void CControlPointIcon::FakePulse( float flTime )
{
	if ( m_pCapPulseImage )
	{
		m_pCapPulseImage->SetVisible( true );
		m_pCapPulseImage->StartPulse( flTime, GetWide() );
		m_pBaseImage->StartPulsing( flTime + FINISHCAPANIM_SWOOP_LENGTH - gpGlobals->curtime, 0.8, false );

		InvalidateLayout();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CControlPointIcon::IsVisible( void )
{
	if ( IsInFreezeCam() == true )
		return false;

	return BaseClass::IsVisible();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CControlPointIcon::Paint( void )
{
	if ( m_bCachedLockedState != IsPointLocked() ||
		 m_bCachedCountdownState != IsPointUnlockCountdownRunning() )
	{
		UpdateImage();
	}

	m_bCachedCountdownState = IsPointUnlockCountdownRunning();
	m_bCachedLockedState = IsPointLocked();

	BaseClass::Paint();

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CControlPointIcon::PerformLayout( void )
{
	BaseClass::PerformLayout();

	int iBaseXPos, iBaseYPos;
	ipanel()->GetAbsPos(GetVPanel(), iBaseXPos, iBaseYPos );

	m_pBaseImage->SetBounds( 0, 0, GetWide(), GetTall() );
	m_pCountdown->SetBounds( 0, 0, GetWide(), GetTall() );
	
	if ( m_pCapImage->IsVisible() )
	{
		m_pCapImage->SetBounds( 0, 0, GetWide(), GetTall() );
	}

	if ( m_pCapHighlightImage->IsVisible() )
	{
		int iHeight = ScreenHeight() * 0.75;
		m_pCapHighlightImage->SetBounds( iBaseXPos + CAP_BOX_INDENT_X, iBaseYPos - iHeight, GetWide() - (CAP_BOX_INDENT_X*2), iHeight + GetTall() -CAP_BOX_INDENT_Y );
	}

	int iCappingTeam = ObjectiveResource()->GetCappingTeam( m_iCPIndex );
	int iPlayers = ObjectiveResource()->GetNumPlayersInArea( m_iCPIndex, iCappingTeam );
	if ( m_pCapPlayerImage && !m_pCapPlayerImage->IsVisible() && iPlayers )
	{
		m_pCapPlayerImage->SetVisible(true);
	}
	if ( m_pCapPlayerImage && m_pCapPlayerImage->IsVisible() )
	{
		if ( !iPlayers )
		{
			// We're a deteriorating point
			m_pCapPlayerImage->SetVisible( false );
			if ( m_pCapNumPlayers )
			{
				m_pCapNumPlayers->SetVisible( false );
			}
		}
		else
		{
			int iXPos, iYPos;
			if ( ( iPlayers < 2 ) || !m_pCapNumPlayers )
			{
				iXPos = (GetWide() - m_pCapPlayerImage->GetWide()) * 0.5;
			}
			else
			{
				iXPos = (GetWide() - m_pCapPlayerImage->GetWide()) * 0.5 - XRES(4);
			}
			iYPos = (GetTall() - m_pCapPlayerImage->GetTall()) * 0.5;

			m_pCapPlayerImage->SetPos( iXPos, iYPos );

			if ( m_pCapNumPlayers )
			{
				m_pCapNumPlayers->SetVisible( (iPlayers>1) );
				SetDialogVariable( "numcappers", iPlayers );

				m_pCapNumPlayers->SetFgColor( Color(0,0,0,255) );
			}
		}
	}

	if ( m_pCapPulseImage )
	{
		int iSize = GetWide() * 3;
		int iXpos = iBaseXPos - ((iSize-GetWide()) * 0.5);
		int iYpos = iBaseYPos - ((iSize-GetTall()) * 0.5);
		m_pCapPulseImage->SetBounds( iXpos, iYpos, iSize, iSize );
	}

	SetTimerTime( m_flCPTimerTime );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CControlPointIcon::OnTick( void )
{
	if ( m_flCPTimerTime < 0 )
		return;

	if ( !m_pCPTimerLabel || !m_pCPTimerLabel->IsVisible() )
		return;

	int nTime = 0;
	if ( m_flCPTimerTime - gpGlobals->curtime > 0 )
	{
		nTime = ceil( m_flCPTimerTime - gpGlobals->curtime );
	}

	if ( nTime <= 10 ) // start flashing with 10 seconds left
	{
		if ( m_bRedText )
		{
			m_bRedText = false;
			m_pCPTimerLabel->SetFgColor( m_cRegularColor );
		}
		else
		{
			m_bRedText = true;
			m_pCPTimerLabel->SetFgColor( m_cHighlightColor );
		}
	}

	char szTime[4];
	Q_snprintf( szTime, sizeof( szTime ), "%d", nTime );
	m_pCPTimerLabel->SetText( szTime );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CControlPointIcon::SetTimerTime( float flTime )
{
	m_flCPTimerTime = flTime;

	if ( m_pCPTimerBG && m_pCPTimerLabel )
	{
		if ( flTime < 0 )
		{
			m_pCPTimerBG->SetVisible( false );
			m_pCPTimerLabel->SetVisible( false );
		}
		else
		{
			int xPos, yPos;
			GetPos( xPos, yPos );

			m_pCPTimerBG->SetPos( xPos, yPos );
			m_pCPTimerBG->SetVisible( true );
	
			m_bRedText = false;
			m_pCPTimerLabel->SetFgColor( m_cRegularColor ); // reset our color
			m_pCPTimerLabel->SetPos( xPos + GetWide() - XRES(1), yPos + ( GetTall() / 2 ) - ( m_pCPTimerLabel->GetTall() / 2 ) );
			m_pCPTimerLabel->SetVisible( true );
			OnTick(); // call this now so our time gets initialized
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudControlPointIcons::CHudControlPointIcons( const char *pName ) : vgui::Panel( NULL, "HudControlPointIcons" ), CHudElement( pName )
{
	SetParent( g_pClientMode->GetViewport() );
	SetHiddenBits( HIDEHUD_MISCSTATUS );

	m_iBackgroundTexture = vgui::surface()->DrawGetTextureId( "vgui/white" );
	if ( m_iBackgroundTexture == -1 )
	{
		m_iBackgroundTexture = vgui::surface()->CreateNewTextureID();
		vgui::surface()->DrawSetTextureFile( m_iBackgroundTexture, "vgui/white", true, true );
	}

	Reset();

	// Initialize textures to invalid
	for( int i = 0; i < ARRAYSIZE( m_iCPTextures ); i++ )
	{
		m_iCPTextures[i] = -1;
		m_iCPCappingTextures[i] = -1;
	}

	for( int i = FIRST_GAME_TEAM; i < MAX_TEAMS; i++ )
	{
		m_iTeamBaseTextures[i] = -1;
	}
}

DECLARE_HUDELEMENT( CHudControlPointIcons );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudControlPointIcons::~CHudControlPointIcons( void )
{
	ShutdownIcons();

	if ( vgui::surface() )
	{
		// Clear out all the texture IDs
		for( int i = 0; i < ARRAYSIZE( m_iCPTextures ); i++ )
		{
			if ( m_iCPTextures[i] != -1 )
			{
				vgui::surface()->DestroyTextureID( m_iCPTextures[i] );
				m_iCPTextures[i] = -1;
			}

			if ( m_iCPCappingTextures[i] != -1 )
			{
				vgui::surface()->DestroyTextureID( m_iCPCappingTextures[i] );
				m_iCPCappingTextures[i] = -1;
			}
		}

		for( int i = FIRST_GAME_TEAM; i < MAX_TEAMS; i++ )
		{
			if ( m_iTeamBaseTextures[i] != -1 )
			{
				vgui::surface()->DestroyTextureID( m_iTeamBaseTextures[i] );
				m_iTeamBaseTextures[i] = -1;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudControlPointIcons::Init( void )
{
	for( int i = 0; i < ARRAYSIZE( m_iCPTextures ); i++ )
	{
		if ( m_iCPTextures[i] == -1 )
		{
			m_iCPTextures[i] = vgui::surface()->CreateNewTextureID();
		}
		
		if ( m_iCPCappingTextures[i] == -1 )
		{
			m_iCPCappingTextures[i] = vgui::surface()->CreateNewTextureID();
		}
	}

	for( int i = FIRST_GAME_TEAM; i < MAX_TEAMS; i++ )
	{
		if ( m_iTeamBaseTextures[i] == -1 )
		{
			m_iTeamBaseTextures[i] = vgui::surface()->CreateNewTextureID();
		}
	}

	ListenForGameEvent( "controlpoint_initialized" );
	ListenForGameEvent( "controlpoint_updateimages" );
	ListenForGameEvent( "controlpoint_updatelayout" );
	ListenForGameEvent( "controlpoint_updatecapping" );
	ListenForGameEvent( "controlpoint_starttouch" );
	ListenForGameEvent( "controlpoint_endtouch" );
	ListenForGameEvent( "controlpoint_pulse_element" );
	ListenForGameEvent( "controlpoint_fake_capture" );
	ListenForGameEvent( "controlpoint_fake_capture_mult" );
	ListenForGameEvent( "intro_nextcamera" );
	ListenForGameEvent( "intro_finish" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudControlPointIcons::Reset( void )
{
	m_iCurrentCP = -1;
	m_iLastCP = -1;
	m_flIconExpand = 0;
	m_flPulseTime = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHudControlPointIcons::IsVisible( void )
{
	if ( IsInFreezeCam() == true )
		return false;

	if ( CHudElement::ShouldDraw() == false )
		return false;

	return BaseClass::IsVisible();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudControlPointIcons::LevelShutdown( void )
{
	ShutdownIcons();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudControlPointIcons::FireGameEvent( IGameEvent *event )
{
	const char *eventname = event->GetName();
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();

	if ( FStrEq( "controlpoint_initialized", eventname ) )
	{
		// Create our control points
		InitIcons();
		return;
	}

	if ( FStrEq( "controlpoint_updateimages", eventname ) )
	{
		// Update the images of our control point icons
		int iIndex = event->GetInt( "index" );
		if ( iIndex == -1 )
		{
			for (int i = 0; i < m_Icons.Count(); i++)
			{
				m_Icons[i]->UpdateImage();
			}
		}
		else
		{
			// Only invalidate the specified cap point
			for (int i = 0; i < m_Icons.Count(); i++)
			{
				if ( m_Icons[i]->GetCapIndex() == iIndex )
				{
					m_Icons[i]->UpdateImage();
				}
			}
		}
		UpdateProgressBarFor( iIndex );
		return;
	}

	if ( FStrEq( "controlpoint_updatelayout", eventname ) )
	{
		// Update the layout of our control point icons
		int iIndex = event->GetInt( "index" );
		if ( iIndex == -1 )
		{
			InvalidateLayout();
		}
		else
		{
			// Only invalidate the specified cap point
			for (int i = 0; i < m_Icons.Count(); i++)
			{
				if ( m_Icons[i]->GetCapIndex() == iIndex )
				{
					m_Icons[i]->InvalidateLayout();
				}
			}
		}
		UpdateProgressBarFor( iIndex );
		return;
	}

	if ( FStrEq( "controlpoint_updatecapping", eventname ) )
	{
		// Update the capping status of our control point icons
		int iIndex = event->GetInt( "index" );
		if ( iIndex == -1 )
		{
			for (int i = 0; i < m_Icons.Count(); i++)
			{
				m_Icons[i]->UpdateCapImage();
			}
		}
		else
		{
			// Only invalidate the specified cap point
			for (int i = 0; i < m_Icons.Count(); i++)
			{
				if ( m_Icons[i]->GetCapIndex() == iIndex )
				{
					m_Icons[i]->UpdateCapImage();
				}
			}
		}

		UpdateProgressBarFor( iIndex );
		return;
	}

	if ( FStrEq( "controlpoint_starttouch", eventname ) )
	{
		int iPlayer = event->GetInt( "player" );
		if ( pPlayer && iPlayer == pPlayer->entindex() )
		{
			m_iCurrentCP = event->GetInt( "area" );
			UpdateProgressBarFor( m_iCurrentCP );
		}
	}
	else if ( FStrEq( "controlpoint_endtouch", eventname ) )
	{
		int iPlayer = event->GetInt( "player" );
		if ( pPlayer && iPlayer == pPlayer->entindex() )
		{
			m_iCurrentCP = -1;
			UpdateProgressBarFor( m_iCurrentCP );
		}
	}
	else if ( FStrEq( "controlpoint_pulse_element", eventname ) )
	{
		int iPlayer = event->GetInt( "player" );
		if ( pPlayer && iPlayer == pPlayer->entindex() )
		{
			for (int i = 0; i < m_Icons.Count(); i++)
			{
				m_Icons[i]->FakePulse( gpGlobals->curtime + (i * PULSE_TIME_PER_ICON) );
			}
		}
	}
	else if ( FStrEq( "controlpoint_fake_capture", eventname ) )
	{
		int iPlayer = event->GetInt( "player" );
		if ( pPlayer && iPlayer == pPlayer->entindex() )
		{
			m_iCurrentCP = event->GetInt( "int_data" );
			m_bFakingCapture = true;
			m_bFakingCaptureMult = false;
			m_flFakeCaptureTime = gpGlobals->curtime + FAKE_CAPTURE_TIME + FAKE_CAPTURE_POST_PAUSE;
			UpdateProgressBarFor( -1 );
		}
	}
	else if ( FStrEq( "controlpoint_fake_capture_mult", eventname ) )
	{
		int iPlayer = event->GetInt( "player" );
		if ( pPlayer && iPlayer == pPlayer->entindex() )
		{
			m_iCurrentCP = event->GetInt( "int_data" );
			m_bFakingCapture = true;
			m_bFakingCaptureMult = true;
			m_flFakeCaptureTime = gpGlobals->curtime + FAKE_CAPTURE_TIME + FAKE_CAPTURE_POST_PAUSE;
			UpdateProgressBarFor( -1 );
		}
	}
	else if ( FStrEq( "intro_nextcamera", eventname ) )
	{
		int iPlayer = event->GetInt( "player" );
		if ( pPlayer && iPlayer == pPlayer->entindex() )
		{
			m_iCurrentCP = -1;
			m_bFakingCapture = false;
			m_bFakingCaptureMult = false;
			UpdateProgressBarFor( -1 );
		}
	}
	else if ( FStrEq( "intro_finish", eventname ) )
	{
		int iPlayer = event->GetInt( "player" );
		if ( pPlayer && iPlayer == pPlayer->entindex() )
		{
			m_iCurrentCP = -1;
			m_flPulseTime = 0;
			m_bFakingCapture = false;
			m_bFakingCaptureMult = false;

			InitIcons();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudControlPointIcons::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	m_hTextFont = pScheme->GetFont( "ChatFont" );

	m_clrBackground = pScheme->GetColor( "HudPanelBackground", GetFgColor() );
	m_clrBorder = pScheme->GetColor( "HudPanelBorder", GetBgColor() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudControlPointIcons::PerformLayout( void )
{
	BaseClass::PerformLayout();

	int iCapPointLines[MAX_CONTROL_POINTS][MAX_CONTROL_POINTS];
	memset( iCapPointLines, 0, sizeof(int) * MAX_CONTROL_POINTS * MAX_CONTROL_POINTS );
	bool bUseDefaultLines = true;
	if ( ObjectiveResource() )
	{
		// Allow the objective resource to override it
		const char *pszLayout = ObjectiveResource()->GetCapLayoutInHUD();
		if ( pszLayout && pszLayout[0] )
		{
			bUseDefaultLines = false;

			// Cap layout is a string with indexes of cap points seperated by ',' to denote
			// a new line. So "3,1 2" would create a pyramid, with cap point 3 on the 
			// first line, and caps 1 & 2 on the second line.
			int iLine = 0;
			int iCapIndex = 0;
			char szBuffer[MAX_CAPLAYOUT_LENGTH];
			Q_strncpy( szBuffer, pszLayout, MAX_CAPLAYOUT_LENGTH );
			char *pszChar = szBuffer;
			char *pszLastNumber = pszChar;
			while ( *pszChar )
			{
				pszChar++;

				if ( *pszChar == ' ' || *pszChar == ',' )
				{
					// Get the number
					char cOrg = *pszChar;
					*pszChar = '\0';

					int iCPIndex = atoi( pszLastNumber );
					int iIconIndex = -1;
					for (int i = 0; i < m_Icons.Count(); i++)
					{
						if ( m_Icons[i]->GetCapIndex() == iCPIndex )
						{
							iIconIndex = i;
							break;
						}
					}

					if ( iIconIndex != -1 )
					{
						iCapPointLines[iLine][iCapIndex] = iIconIndex+1;
						*pszChar = cOrg;
						if ( *pszChar == ',' )
						{
							iLine++;
							iCapIndex = 0;
						}
						else
						{
							iCapIndex++;
						}
					}

					// Walk past the ,/space
					pszChar++;
					pszLastNumber = pszChar;
				}
			}

			// Now get the trailing number
			int iCPIndex = atoi( pszLastNumber );
			for (int i = 0; i < m_Icons.Count(); i++)
			{
				if ( m_Icons[i]->GetCapIndex() == iCPIndex )
				{
					iCapPointLines[iLine][iCapIndex] = i+1;
					break;
				}
			}
		}
	}
	
	if ( bUseDefaultLines )
	{
		// By default, put all the caps on a single line
		int iCPIndex = 0;
		for (int iIcon = 0; iIcon < m_Icons.Count(); iIcon++)
		{
			iCapPointLines[0][iCPIndex] = iIcon+1;
			iCPIndex++;
		}
	}

	int iTall = m_iIconGapHeight;
	int iTallest = m_iIconGapHeight;
	int iWidest = m_iIconGapWidth;
	int iTotalIconsPerLine[MAX_CONTROL_POINTS];
	int iLineWidth[MAX_CONTROL_POINTS];
	memset( iTotalIconsPerLine, 0, sizeof(int) * MAX_CONTROL_POINTS );
	memset( iLineWidth, 0, sizeof(int) * MAX_CONTROL_POINTS );
	int iTotalLines = 0;

	// Search through the lines and figure out our overall width & height
	for ( int iLine = 0; iLine < MAX_CONTROL_POINTS; iLine++ )
	{
		// If we've hit a line with nothing in it, we're done
		if ( !iCapPointLines[iLine][0] )
			break;

		iTotalLines++;

		iLineWidth[iLine] = m_iIconGapWidth;
		int iLineTall = 0;
		for ( int iPosition = 0; iPosition < MAX_CONTROL_POINTS; iPosition++ )
		{
			int iIconIndex = iCapPointLines[iLine][iPosition];
			if ( !iIconIndex )
				break;

			iIconIndex--;

			// Add the icon dimensions to our counts.
			if ( iIconIndex >= 0 && iIconIndex < m_Icons.Count() )
			{
				m_Icons[iIconIndex]->PerformLayout();
				iTotalIconsPerLine[iLine]++;

				iLineWidth[iLine] += m_Icons[iIconIndex]->GetWide() + m_iIconGapWidth;
				int iHeight = m_Icons[iIconIndex]->GetTall();
				if ( iHeight > iLineTall )
				{
					iLineTall = iHeight;
				}
			}
		}

		if ( iLineWidth[iLine] > iWidest )
		{
			iWidest = iLineWidth[iLine];
		}
		if ( iLineTall > iTallest )
		{
			iTallest = iLineTall;
		}
		iTall += iLineTall + m_iIconGapHeight;
	}

	// Setup the main panel
	float flPositionX = (ScreenWidth() - iWidest) * 0.5;
	float flPositionY = ScreenHeight() - iTall - m_nHeightOffset;
	if ( ObjectiveResource() )
	{
		float flCustomPositionX = -1.f;
		float flCustomPositionY = -1.f;
		ObjectiveResource()->GetCapLayoutCustomPosition( flCustomPositionX, flCustomPositionY );
		if ( flCustomPositionX != -1.f )
		{
			flPositionX = flCustomPositionX * ScreenWidth();
		}
		if ( flCustomPositionY != -1.f )
		{
			flPositionY = flCustomPositionY * ScreenHeight();
		}
	}
	SetBounds( flPositionX, flPositionY, iWidest, iTall );

	// Now that we know how wide we are, and how many icons are in each line, 
	// we can lay the icons out, centered in the lines.
	for ( int iLine = 0; iLine < MAX_CONTROL_POINTS; iLine++ )
	{
		if ( !iTotalIconsPerLine[iLine] )
			break;

		int iLineXPos = ((iWidest - iLineWidth[iLine]) * 0.5) + m_iIconGapWidth;
		int iLineYPos = (iLine * m_iIconGapHeight) + ( iLine * iTallest ) + m_iIconGapHeight;
		for ( int iPosition = 0; iPosition < MAX_CONTROL_POINTS; iPosition++ )
		{
			int iIconIndex = iCapPointLines[iLine][iPosition];
			if ( !iIconIndex )
				break;

			iIconIndex--;

			if ( iIconIndex >= 0 && iIconIndex < m_Icons.Count() )
			{
				m_Icons[iIconIndex]->SetPos( iLineXPos, iLineYPos );
				iLineXPos += m_Icons[iIconIndex]->GetWide() + m_iIconGapWidth;

				// If we have multiple lines, swipe up when capping
				m_Icons[iIconIndex]->SetSwipeUp( iTotalLines > 1 );

				// Set the progress extrusion dir:
				//		N if we're on the top line. Otherwise:
				//			NW if we're left of the center.
				//			NE if we're at or right of the center.
				int iDir = CP_DIR_N;
				if ( iLine > 0 )
				{
					if ( ((float)(iPosition+1) / (float)iTotalIconsPerLine[iLine]) > 0.5 )
					{
						iDir = CP_DIR_NE;
					}
					else
					{
						iDir = CP_DIR_NW;
					}
				}

				m_Icons[iIconIndex]->SetCapProgressDir( iDir );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudControlPointIcons::UpdateProgressBarFor( int iIndex )
{
	// If they tell us to update all progress bars, update only the one we're standing on
	if ( iIndex == -1 )
	{
		iIndex = m_iCurrentCP;
	}

	// Ignore requests to display progress bars for points we're not standing on
	if ( ( m_iCurrentCP != iIndex ) )
		return;

	// This can happen at level load
	CTFHudObjectiveStatus *pStatus = GET_HUDELEMENT( CTFHudObjectiveStatus );
	if ( pStatus && pStatus->GetControlPointProgressBar() )
	{
		CControlPointProgressBar *pProgressBar = pStatus->GetControlPointProgressBar();
		if ( !IsVisible() || iIndex < 0 || iIndex >= ObjectiveResource()->GetNumControlPoints() )
		{
			pProgressBar->SetupForPoint( NULL );
		}
		else
		{
			for (int i = 0; i < m_Icons.Count(); i++)
			{
				if ( m_Icons[i]->GetCapIndex() == iIndex )
				{
					pProgressBar->SetupForPoint( m_Icons[i] );
					break;
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Create the icons we need to display the state of this map's control points
//-----------------------------------------------------------------------------
void CHudControlPointIcons::InitIcons( void )
{
	ShutdownIcons();

	CTFHudObjectiveStatus *pStatus = GET_HUDELEMENT( CTFHudObjectiveStatus );
	if ( pStatus )
	{
		CControlPointProgressBar *pProgressBar = pStatus->GetControlPointProgressBar();

		if ( pProgressBar )
		{
			m_iCurrentCP = -1;
			pProgressBar->SetupForPoint( NULL );
		}
	}

	// Create an icon for each visible control point in this miniround
	int iPoints = ObjectiveResource()->GetNumControlPoints();
	for ( int i = 0; i < iPoints; i++ )
	{
		if ( ObjectiveResource()->IsInMiniRound(i) && ObjectiveResource()->IsCPVisible(i) )
		{
			CControlPointIcon *pIcon = new CControlPointIcon( this, VarArgs( "ControlPointIcon%d", i ), i );
			m_Icons.AddToTail( vgui::SETUP_PANEL(pIcon) );
		}
	}

	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudControlPointIcons::ShutdownIcons( void )
{
	for ( int i = 0; i < m_Icons.Count(); i++ )
	{
		m_Icons[i]->MarkForDeletion();
	}
	m_Icons.RemoveAll();

	// if we remove all the icons, we need to make sure the progress bar isn't holding onto one
	CTFHudObjectiveStatus *pStatus = GET_HUDELEMENT( CTFHudObjectiveStatus );
	if ( pStatus )
	{
		CControlPointProgressBar *pProgressBar = pStatus->GetControlPointProgressBar();
		if ( pProgressBar )
		{
			m_iCurrentCP = -1;
			pProgressBar->SetupForPoint( NULL );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudControlPointIcons::DrawBackgroundBox( int xpos, int ypos, int nBoxWidth, int nBoxHeight, bool bCutCorner )
{
	int nCornerCutSize = bCutCorner ? m_nCornerCutSize : 0;
	vgui::Vertex_t verts[5];

	verts[0].Init( Vector2D( xpos, ypos ) );
	verts[1].Init( Vector2D( xpos + nBoxWidth, ypos ) );
	verts[2].Init( Vector2D( xpos + nBoxWidth + 1, ypos + nBoxHeight - nCornerCutSize + 1 ) );
	verts[3].Init( Vector2D( xpos + nBoxWidth - nCornerCutSize + 1, ypos + nBoxHeight + 1 ) );
	verts[4].Init( Vector2D( xpos, ypos + nBoxHeight ) );

	vgui::surface()->DrawSetTexture( m_iBackgroundTexture );
	vgui::surface()->DrawSetColor( Color( m_clrBackground ) );
	vgui::surface()->DrawTexturedPolygon( 5, verts );

	vgui::Vertex_t borderverts[5];

	borderverts[0].Init( Vector2D( xpos, ypos ) );
	borderverts[1].Init( Vector2D( xpos + nBoxWidth, ypos ) );
	borderverts[2].Init( Vector2D( xpos + nBoxWidth, ypos + nBoxHeight - nCornerCutSize ) );
	borderverts[3].Init( Vector2D( xpos + nBoxWidth - nCornerCutSize, ypos + nBoxHeight ) );
	borderverts[4].Init( Vector2D( xpos, ypos + nBoxHeight ) );

	vgui::surface()->DrawSetColor( Color( m_clrBorder ) );
	vgui::surface()->DrawTexturedPolyLine( borderverts, 5 );
}

//-----------------------------------------------------------------------------
// Purpose: Draw the team's base icon at either end of the icon panel
//-----------------------------------------------------------------------------
bool CHudControlPointIcons::PaintTeamBaseIcon( int index, float flXPos, float flYPos, float flIconSize )
{
	float uv1 = 0.0f;
	float uv2 = 1.0f;

	// Find out which team owns the far left
	for ( int i = 0; i < MAX_TEAMS; i++ )
	{
		if ( ObjectiveResource()->GetBaseControlPointForTeam(i) == index )
		{
			int iTeamBaseIcon = ObjectiveResource()->GetBaseIconForTeam(i);
			if ( iTeamBaseIcon )
			{
				// Draw the Team's Base texture
				const char *szMatName = GetMaterialNameFromIndex( iTeamBaseIcon );

				vgui::surface()->DrawSetTextureFile( m_iTeamBaseTextures[i], szMatName, true, false );

				Vector2D uv11( uv1, uv1 );
				Vector2D uv21( uv2, uv1 );
				Vector2D uv22( uv2, uv2 );
				Vector2D uv12( uv1, uv2 );

				vgui::Vertex_t vert[4];	
				vert[0].Init( Vector2D( flXPos,					flYPos              ), uv11 );
				vert[1].Init( Vector2D( flXPos + flIconSize,	flYPos              ), uv21 );
				vert[2].Init( Vector2D( flXPos + flIconSize,	flYPos + flIconSize ), uv22 );				
				vert[3].Init( Vector2D( flXPos,					flYPos + flIconSize ), uv12 );

				vgui::surface()->DrawSetColor( Color(255,255,255,255) );	
				vgui::surface()->DrawTexturedPolygon( 4, vert );

				return true;
			}
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudControlPointIcons::Paint()
{
	if ( IsInFreezeCam() == true )
		return;

	if( !ObjectiveResource() )
		return;

	int num = ObjectiveResource()->GetNumControlPoints();
	if ( num <= 0 )
		return; // nothing to draw yet

	//DrawBackgroundBox( 0, 0, GetWide(), GetTall(), false );
	BaseClass::Paint();
}


//========================================================================================================================
// CONTROL POINT PROGRESS BAR
//========================================================================================================================
CControlPointProgressBar::CControlPointProgressBar(Panel *parent) : vgui::EditablePanel( parent, "ControlPointProgressBar" )
{
	m_pAttachedToIcon = NULL;
	m_pBar = NULL;
	m_pBarText = NULL;
	m_pTeardrop = NULL;
	m_pTeardropSide = NULL;
	m_pBlocked = NULL;
	m_iOrgHeight = 0;
	m_iMidGroupIndex = -1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CControlPointProgressBar::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "resource/UI/ControlPointProgressBar.res" );

	m_pBar = dynamic_cast<vgui::CircularProgressBar *>( FindChildByName("ProgressBar") );
	m_pBarText = dynamic_cast<vgui::Label *>( FindChildByName("ProgressText") );
	m_pTeardrop = dynamic_cast<CIconPanel *>( FindChildByName("Teardrop") );
	m_pTeardropSide = dynamic_cast<CIconPanel *>( FindChildByName("TeardropSide") );
	m_pBlocked = dynamic_cast<CIconPanel *>( FindChildByName("Blocked") );
	m_iOrgHeight = GetTall();

	m_pBar->SetProgressDirection( vgui::CircularProgressBar::PROGRESS_CW );
	m_pBar->SetReverseProgress( true );

	m_iMidGroupIndex = gHUD.LookupRenderGroupIndexByName( "mid" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CControlPointProgressBar::PerformLayout( void )
{
	BaseClass::PerformLayout();

	if ( m_pAttachedToIcon && m_pTeardrop && m_pTeardropSide && m_pAttachedToIcon->GetVPanel() )
	{
		int iIconX, iIconY;
		ipanel()->GetAbsPos(m_pAttachedToIcon->GetVPanel(), iIconX, iIconY );
		int iDir = m_pAttachedToIcon->GetCapProgressDir();
		int iXPos = 0;
		int iYPos = 0;

		int iEdgeSpace = (GetWide() - m_pTeardrop->GetWide()) * 0.5;

		// Line up our middle with the middle of the icon
		switch ( iDir )
		{
		default:
		case CP_DIR_N:
			SetSize( GetWide(), m_iOrgHeight );
			m_pTeardrop->SetVisible( true );
			m_pTeardropSide->SetVisible( false );
			iXPos = iIconX - ((GetWide() - m_pAttachedToIcon->GetWide()) * 0.5);
			iYPos = iIconY - GetTall();
			break;

		case CP_DIR_NE:
			SetSize( GetWide(), m_iOrgHeight );
			m_pTeardropSide->SetIcon( "cappoint_progressbar_teardrop_right" );
			m_pTeardrop->SetVisible( false );
			m_pTeardropSide->SetVisible( true );
			iXPos = iIconX + m_pAttachedToIcon->GetWide() - iEdgeSpace;
			iYPos = iIconY - GetTall();
			break;

		case CP_DIR_NW:
			SetSize( GetWide(), m_iOrgHeight );
			m_pTeardropSide->SetIcon( "cappoint_progressbar_teardrop_left" );
			m_pTeardrop->SetVisible( false );
			m_pTeardropSide->SetVisible( true );
			iXPos = iIconX - GetWide() + iEdgeSpace;
			iYPos = iIconY - GetTall();
			break;
		}
		
		SetPos( iXPos, iYPos );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CControlPointProgressBar::Reset( void )
{
	m_pAttachedToIcon = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CControlPointProgressBar::IsVisible( void )
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
void CControlPointProgressBar::Paint( void )
{
	if ( m_pAttachedToIcon )
	{
		int iCP = m_pAttachedToIcon->GetCapIndex();
		if ( m_pBar && m_pBar->IsVisible() )
		{						
			m_pBar->SetProgress( ObjectiveResource()->GetCPCapPercentage( iCP ) );
		}
	}

	BaseClass::Paint();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CControlPointProgressBar::SetupForPoint( CControlPointIcon *pIcon )
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return;

	m_pAttachedToIcon = pIcon;

	bool bInWinState = TeamplayRoundBasedRules() ? TeamplayRoundBasedRules()->RoundHasBeenWon() : false;

	if ( m_pAttachedToIcon && !bInWinState )
	{
		SetVisible( true );

		int iCP = m_pAttachedToIcon->GetCapIndex();
		int iCappingTeam = ObjectiveResource()->GetCappingTeam( iCP );
		int iOwnerTeam = ObjectiveResource()->GetOwningTeam( iCP );
		int iPlayerTeam = pPlayer->GetTeamNumber();
		bool bCapBlocked = ObjectiveResource()->CapIsBlocked( iCP );

		if ( !bCapBlocked && iCappingTeam != TEAM_UNASSIGNED && iCappingTeam != iOwnerTeam && iCappingTeam == iPlayerTeam )
		{
			m_pBar->SetBgImage( ObjectiveResource()->GetGameSpecificCPBarBG( iCP, iCappingTeam ) );
			m_pBar->SetFgImage( ObjectiveResource()->GetGameSpecificCPBarFG( iCP, iOwnerTeam ) );
			m_pBar->SetVisible( true );
			m_pBlocked->SetVisible( false );
			m_pBarText->SetVisible( false );
		}
		else
		{
			m_pBar->SetVisible( false );
			m_pBlocked->SetVisible( true );
			
			UpdateBarText();
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
void CControlPointProgressBar::UpdateBarText( void )
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer || !m_pBarText || !m_pAttachedToIcon )
		return;

	m_pBarText->SetVisible( true );

	int iCP = m_pAttachedToIcon->GetCapIndex();
	int iCappingTeam = ObjectiveResource()->GetCappingTeam( iCP );
	int iPlayerTeam = pPlayer->GetTeamNumber();
	int iOwnerTeam = ObjectiveResource()->GetOwningTeam( iCP );

	if ( !TeamplayGameRules()->PointsMayBeCaptured() )
	{
		m_pBarText->SetText( "#Team_Capture_NotNow" );
		return;
	}

	if ( ObjectiveResource()->GetCPLocked( iCP ) )
	{
		m_pBarText->SetText( "#Team_Capture_NotNow" );
		return;
	}

	if ( mp_blockstyle.GetInt() == 1 && iCappingTeam != TEAM_UNASSIGNED && iCappingTeam != iPlayerTeam )
	{
		if ( ObjectiveResource()->IsCPBlocked(iCP) )
		{
			m_pBarText->SetText( "#Team_Blocking_Capture" );
			return;
		}
		else if ( iOwnerTeam == TEAM_UNASSIGNED )
		{
			m_pBarText->SetText( "#Team_Reverting_Capture" );
			return;
		}
	}

	if ( ObjectiveResource()->GetOwningTeam(iCP) == iPlayerTeam )
	{
		// If the opponents can never recapture this point back, we use a different string
		if ( iPlayerTeam != TEAM_UNASSIGNED )
		{
			int iEnemyTeam = ( iPlayerTeam == TF_TEAM_RED ) ? TF_TEAM_BLUE : TF_TEAM_RED;
			if ( !ObjectiveResource()->TeamCanCapPoint( iCP, iEnemyTeam ) )
			{
				m_pBarText->SetText( "#Team_Capture_Owned" );
				return;
			}
		}

		m_pBarText->SetText( "#Team_Capture_OwnPoint" );
		return;
	}

	if ( !TeamplayGameRules()->TeamMayCapturePoint( iPlayerTeam, iCP ) )
	{
		if ( TeamplayRoundBasedRules() && TeamplayRoundBasedRules()->IsInArenaMode() == true )
		{
			m_pBarText->SetText( "#Team_Capture_NotNow" );
		}
		else
		{
			m_pBarText->SetText( "#Team_Capture_Linear" );
		}

		return;
	}

	char szReason[256];
	if ( !TeamplayGameRules()->PlayerMayCapturePoint( pPlayer, iCP, szReason, sizeof(szReason) ) )
	{
		m_pBarText->SetText( szReason );
		return;
	}

	bool bHaveRequiredPlayers = true;

	// In Capstyle 1, more players simply cap faster, no required amounts.
	if ( mp_capstyle.GetInt() != 1 )
	{
		int nNumTeammates = ObjectiveResource()->GetNumPlayersInArea( iCP, iPlayerTeam );
		int nRequiredTeammates = ObjectiveResource()->GetRequiredCappers( iCP, iPlayerTeam );
		bHaveRequiredPlayers = (nNumTeammates >= nRequiredTeammates);
	}

	if ( iCappingTeam == iPlayerTeam && bHaveRequiredPlayers )
	{
		m_pBarText->SetText( "#Team_Capture_Blocked" );
		return;
	}

	if ( !ObjectiveResource()->TeamCanCapPoint( iCP, iPlayerTeam ) )
	{
		m_pBarText->SetText( "#Team_Cannot_Capture" );
		return;
	}

	m_pBarText->SetText( "#Team_Waiting_for_teammate" );
}

//========================================================================================================================
// CONTROL POINT CAPTURE ARROW SWIPE
//========================================================================================================================
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CControlPointIconCapArrow::CControlPointIconCapArrow( CControlPointIcon *pIcon, Panel *parent, const char *name) 
	: vgui::Panel( parent, name )
{
	m_pArrowMaterial = NULL;
	m_pAttachedToIcon = pIcon;
	Assert( m_pAttachedToIcon );
}

bool CControlPointIconCapArrow::IsVisible( void )
{
	if ( IsInFreezeCam() == true )
		return false;

	return BaseClass::IsVisible();
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CControlPointIconCapArrow::Paint( void )
{
	if ( !m_pArrowMaterial || !m_pAttachedToIcon )
		return;

	int x = 0;
	int y = 0;
	ipanel()->GetAbsPos(GetVPanel(), x,y );
	int iWidth = GetWide();
	int iHeight = GetTall();

	// Position the arrow based on the cap percentage
	float flXa = 0;
	float flXb = 1.0;
	float flYa = 0;
	float flYb = 1.0;

	int iCappingTeam = ObjectiveResource()->GetCappingTeam( m_pAttachedToIcon->GetCapIndex() );
	float flCapPercentage = ObjectiveResource()->GetCPCapPercentage( m_pAttachedToIcon->GetCapIndex() );

	// The image needs to be remapped to the width of the underlying box,
	// because we want the arrow head to move off the box at the exact time the cap ends.
	float flArrowHeadPixelWidth = 15.0;
	float flArrowBodyPixelWidth = 54.0;
	float flBoxSize = 33.0;

	// HACK: The arrow has an arrowhead portion that looks like this: >
	// We want to start with the arrow entering the image, but we want to keep
	// going until the arrow is fully off the edge. So we include extra width for it.
	float flImageSize = (flArrowHeadPixelWidth+flArrowBodyPixelWidth);
	float flMovementInTextureSpace = ( (flBoxSize+flArrowHeadPixelWidth) / flImageSize );
	float flArrowSizeInTextureSpace = ( flArrowHeadPixelWidth / flImageSize );

	// To help players spot the start & end of a cap, we indent a little on either side.
	float flIndent = 0.07;

	if ( m_pAttachedToIcon->ShouldSwipeUp() )
	{
		flYa = RemapVal( flCapPercentage, 0.0, 1.0, -flMovementInTextureSpace - flIndent, flArrowSizeInTextureSpace - flIndent );
		flYb = RemapVal( flCapPercentage, 0.0, 1.0, flIndent, flMovementInTextureSpace - flIndent );
	}
	else
	{
		flIndent = 0.1;

		float flStart = 1.0 - flIndent;
		float flEnd = 1.0 + flIndent;

		bool bSwipeLeftToRight = ( iCappingTeam % 2 ) ? false : true;
		if ( bSwipeLeftToRight )
		{
			flXa = RemapVal( flCapPercentage, 0.0, 1.0, flStart + flMovementInTextureSpace, flEnd - flArrowSizeInTextureSpace );
			flXb = RemapVal( flCapPercentage, 0.0, 1.0, flStart, 0.0 );
		}
		else
		{
			flXa = RemapVal( flCapPercentage, 0.0, 1.0, flStart, 0.0 );
			flXb = RemapVal( flCapPercentage, 0.0, 1.0, flStart + flMovementInTextureSpace, flEnd - flArrowSizeInTextureSpace );
		}
	}

	CMatRenderContextPtr pRenderContext( materials );
	pRenderContext->Bind( m_pArrowMaterial );
	IMesh* pMesh = pRenderContext->GetDynamicMesh( true );

	CMeshBuilder meshBuilder;
	meshBuilder.Begin( pMesh, MATERIAL_QUADS, 1 );

	meshBuilder.Position3f( x, y, 0.0f );
	meshBuilder.TexCoord2f( 0, flXa, flYa );
	meshBuilder.TexCoord2f( 1, 0.0f, 0.0f );
	meshBuilder.Color4ub( 255, 255, 255, 255 );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3f( x + iWidth, y, 0.0f );
	meshBuilder.TexCoord2f( 0, flXb, flYa );
	meshBuilder.TexCoord2f( 1, 1.0f, 0.0f );
	meshBuilder.Color4ub( 255, 255, 255, 255 );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3f( x + iWidth, y + iHeight, 0.0f );
	meshBuilder.TexCoord2f( 0, flXb, flYb );
	meshBuilder.TexCoord2f( 1, 1.0f, 1.0f );
	meshBuilder.Color4ub( 255, 255, 255, 255 );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3f( x, y + iHeight, 0.0f );
	meshBuilder.TexCoord2f( 0, flXa, flYb );
	meshBuilder.TexCoord2f( 1, 0.0f, 1.0f );
	meshBuilder.Color4ub( 255, 255, 255, 255 );
	meshBuilder.AdvanceVertex();

	meshBuilder.End();
	pMesh->Draw();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CControlPointCountdown::CControlPointCountdown(Panel *parent, const char *name) : vgui::EditablePanel( parent, name )
{
	m_bFire5SecRemain = true;
	m_bFire4SecRemain = true;
	m_bFire3SecRemain = true;
	m_bFire2SecRemain = true;
	m_bFire1SecRemain = true;
	m_bFire0SecRemain = true;

	m_flUnlockTime = 0.0f;

	vgui::ivgui()->AddTickSignal( GetVPanel(), 100 );
	SetVisible( true );

	SetDialogVariable( "capturetime", "" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CControlPointCountdown::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	// load control settings...
	LoadControlSettings( "resource/UI/ControlPointCountdown.res" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CControlPointCountdown::PerformLayout()
{
	CExLabel *pLabel = dynamic_cast<CExLabel *>( FindChildByName( "CapCountdownLabel" ) );
	if ( pLabel )
	{
		pLabel->SetBounds( 0, 0, GetWide(), GetTall() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CControlPointCountdown::SetUnlockTime( float flTime )
{ 
	m_flUnlockTime = flTime; 

	float flTimeDiff = m_flUnlockTime - gpGlobals->curtime;
	m_bFire5SecRemain = ( flTimeDiff >= 5.0f );
	m_bFire4SecRemain = ( flTimeDiff >= 4.0f );
	m_bFire3SecRemain = ( flTimeDiff >= 3.0f );
	m_bFire2SecRemain = ( flTimeDiff >= 2.0f );
	m_bFire1SecRemain = ( flTimeDiff >= 1.0f );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CControlPointCountdown::OnTick( void )
{
	BaseClass::OnTick();

	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pLocalPlayer || ( m_flUnlockTime <= 0.0f ) )
	{
		if ( IsVisible() )
		{
			SetVisible( false );
		}
		return;
	}

	if ( TeamplayRoundBasedRules() ) 
	{
		if ( TeamplayRoundBasedRules()->IsInWaitingForPlayers() || TeamplayRoundBasedRules()->State_Get() != GR_STATE_RND_RUNNING )
		{
			return;
		}
	}

	int iTimeLeft = m_flUnlockTime - gpGlobals->curtime;
	if ( iTimeLeft > 5 || iTimeLeft <= 0 )
	{
		if ( iTimeLeft <= 0 && m_bFire0SecRemain )
		{
			m_bFire0SecRemain = false;
			pLocalPlayer->EmitSound( "Announcer.AM_CapEnabledRandom" );
			m_flUnlockTime = 0.0f;
		}

		if ( IsVisible() )
		{
			SetVisible( false );
		}
		return;
	}

	if ( !IsVisible() )
	{
		SetVisible( true );
	}

	wchar_t wzTimeLeft[128];
	_snwprintf( wzTimeLeft, ARRAYSIZE( wzTimeLeft ), L"%i", iTimeLeft );

	SetDialogVariable( "capturetime", wzTimeLeft );

	if ( iTimeLeft <= 5 && m_bFire5SecRemain )
	{
		m_bFire5SecRemain = false;
		pLocalPlayer->EmitSound( "Announcer.RoundBegins5Seconds" );
	}
	else if ( iTimeLeft <= 4 && m_bFire4SecRemain )
	{
		m_bFire4SecRemain = false;
		pLocalPlayer->EmitSound( "Announcer.RoundBegins4Seconds" );
	}
	else if ( iTimeLeft <= 3 && m_bFire3SecRemain )
	{
		m_bFire3SecRemain = false;
		pLocalPlayer->EmitSound( "Announcer.RoundBegins3Seconds" );
	}
	else if ( iTimeLeft <= 2 && m_bFire2SecRemain )
	{
		m_bFire2SecRemain = false;
		pLocalPlayer->EmitSound( "Announcer.RoundBegins2Seconds" );
	}
	else if ( iTimeLeft <= 1 && m_bFire1SecRemain )
	{
		m_bFire1SecRemain = false;
		m_bFire0SecRemain = true;
		pLocalPlayer->EmitSound( "Announcer.RoundBegins1Seconds" );
	}
}
