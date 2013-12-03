//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "hudelement.h"
#include <vgui_controls/Panel.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui/ISurface.h>
#include "c_baseplayer.h"
#include "iclientmode.h"
#include "c_team_objectiveresource.h"
#include "c_team.h"
#include "view.h"
#include "teamplay_gamerules.h"

#define INTRO_NUM_FAKE_PLAYERS		3

extern ConVar mp_capstyle;
extern ConVar mp_blockstyle;

//-----------------------------------------------------------------------------
// Purpose: Draws the progress bar
//-----------------------------------------------------------------------------
class CHudCapturePanelProgressBar : public vgui::ImagePanel
{
public:
	DECLARE_CLASS_SIMPLE( CHudCapturePanelProgressBar, vgui::ImagePanel );

	CHudCapturePanelProgressBar( vgui::Panel *parent, const char *name );

	virtual void Paint();

	void SetPercentage( float flPercentage ){ m_flPercent = flPercentage; }

private:

	float	m_flPercent;
	int		m_iTexture;

	CPanelAnimationVar( Color, m_clrActive, "color_active", "HudCaptureProgressBar.Active" );
	CPanelAnimationVar( Color, m_clrInActive, "color_inactive", "HudCaptureProgressBar.InActive" );
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CHudCapturePanelIcon : public vgui::ImagePanel
{
public:
	DECLARE_CLASS_SIMPLE( CHudCapturePanelIcon, vgui::ImagePanel );

	CHudCapturePanelIcon( vgui::Panel *parent, const char *name );

	virtual void Paint();

	void SetActive( bool state ){ m_bActive = state; }

private:

	bool	m_bActive;
	int		m_iTexture;

	CPanelAnimationVar( Color, m_clrActive, "color_active", "HudCaptureIcon.Active" );
	CPanelAnimationVar( Color, m_clrInActive, "color_inactive", "HudCaptureIcon.InActive" );
};

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
class CHudCapturePanel : public CHudElement, public vgui::EditablePanel
{
public:
	DECLARE_CLASS_SIMPLE( CHudCapturePanel, vgui::EditablePanel );

	CHudCapturePanel( const char *pElementName );

	virtual void Init( void );
	virtual void LevelInit( void );
	virtual void OnThink();
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void OnScreenSizeChanged( int iOldWide, int iOldTall );
	virtual void FireGameEvent( IGameEvent *event );

private:

	int m_iCurrentCP;	// the index of the control point the local is currently in

	int									m_iOriginalYPos;

	CHudCapturePanelProgressBar			*m_pProgressBar;

	CUtlVector<CHudCapturePanelIcon *>	m_PlayerIcons;

	bool								m_bInitializedFlags;
	vgui::ImagePanel					*m_pTeamFlags[ MAX_TEAMS ];

	vgui::Label							*m_pMessage;

	vgui::Panel							*m_pBackground;

	CPanelAnimationVarAliasType( float, m_nSpaceBetweenIcons, "icon_space", "2", "proportional_float" );

	// For demonstrations of the element in the intro
	bool m_bFakingCapture;	
	bool m_bFakingMultCapture;
	float m_flFakeCaptureTime;
	C_BaseAnimating	*m_pFakePlayers[INTRO_NUM_FAKE_PLAYERS];
};

DECLARE_HUDELEMENT( CHudCapturePanel );

ConVar hud_capturepanel( "hud_capturepanel", "1", FCVAR_CLIENTDLL | FCVAR_ARCHIVE, "Set to 0 to not draw the HUD capture panel" );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudCapturePanelProgressBar::CHudCapturePanelProgressBar( vgui::Panel *parent, const char *name ) : vgui::ImagePanel( parent, name )
{
	m_flPercent = 0.0f;

	m_iTexture = vgui::surface()->DrawGetTextureId( "vgui/progress_bar" );
	if ( m_iTexture == -1 ) // we didn't find it, so create a new one
	{
		m_iTexture = vgui::surface()->CreateNewTextureID();	
		vgui::surface()->DrawSetTextureFile( m_iTexture, "vgui/progress_bar", true, false );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudCapturePanelProgressBar::Paint()
{
	int wide, tall;
	GetSize( wide, tall );

	float uv1 = 0.0f, uv2 = 1.0f;
	Vector2D uv11( uv1, uv1 );
	Vector2D uv21( uv2, uv1 );
	Vector2D uv22( uv2, uv2 );
	Vector2D uv12( uv1, uv2 );

	vgui::Vertex_t verts[4];	
	verts[0].Init( Vector2D( 0, 0 ), uv11 );
	verts[1].Init( Vector2D( wide, 0 ), uv21 );
	verts[2].Init( Vector2D( wide, tall ), uv22 );
	verts[3].Init( Vector2D( 0, tall ), uv12  );

	// first, just draw the whole thing inactive.
	vgui::surface()->DrawSetTexture( m_iTexture );
	vgui::surface()->DrawSetColor( m_clrInActive );
	vgui::surface()->DrawTexturedPolygon( 4, verts );

	// now, let's calculate the "active" part of the progress bar
	vgui::surface()->DrawSetColor( m_clrActive );

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
	//	float flEndAngle = flCompleteCircle * m_flPercent; // count UP (clockwise)
	
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

		vgui::surface()->DrawTexturedPolygon( 4, verts );

		// draw the third quadrant
		uv11.Init( 0.0f, 0.5f );
		uv21.Init( 0.5f, 0.5f );
		uv22.Init( 0.5f, 1.0f );
		uv12.Init( 0.0f, 1.0f );

		verts[0].Init( Vector2D( 0.0f, flHalfTall ), uv11 );
		verts[1].Init( Vector2D( flHalfWide, flHalfTall ), uv21 );
		verts[2].Init( Vector2D( flHalfWide, tall ), uv22 );
		verts[3].Init( Vector2D( 0.0f, tall ), uv12  );

		vgui::surface()->DrawTexturedPolygon( 4, verts );

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

			vgui::surface()->DrawTexturedPolygon( 4, verts );
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

			vgui::surface()->DrawTexturedPolygon( 4, verts );
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

		vgui::surface()->DrawTexturedPolygon( 4, verts );

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

			vgui::surface()->DrawTexturedPolygon( 4, verts );
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

			vgui::surface()->DrawTexturedPolygon( 4, verts );
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

		vgui::surface()->DrawTexturedPolygon( 4, verts );

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

			vgui::surface()->DrawTexturedPolygon( 4, verts );
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

			vgui::surface()->DrawTexturedPolygon( 4, verts );
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

			vgui::surface()->DrawTexturedPolygon( 4, verts );
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

			vgui::surface()->DrawTexturedPolygon( 4, verts );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudCapturePanelIcon::CHudCapturePanelIcon( vgui::Panel *parent, const char *name ) : vgui::ImagePanel( parent, name )
{
	m_bActive = false;

	m_iTexture = vgui::surface()->DrawGetTextureId( "vgui/capture_icon" );
	if ( m_iTexture == -1 ) // we didn't find it, so create a new one
	{
		m_iTexture = vgui::surface()->CreateNewTextureID();	
		vgui::surface()->DrawSetTextureFile( m_iTexture, "vgui/capture_icon", true, false );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudCapturePanelIcon::Paint()
{
	int wide, tall;
	GetSize( wide, tall );

	float uv1 = 0.0f, uv2 = 1.0f;
	Vector2D uv11( uv1, uv1 );
	Vector2D uv12( uv1, uv2 );
	Vector2D uv21( uv2, uv1 );
	Vector2D uv22( uv2, uv2 );

	vgui::Vertex_t verts[4];	
	verts[0].Init( Vector2D( 0, 0 ), uv11 );
	verts[1].Init( Vector2D( wide, 0 ), uv21 );
	verts[2].Init( Vector2D( wide, tall ), uv22 );
	verts[3].Init( Vector2D( 0, tall ), uv12  );

	// just draw the whole thing
	vgui::surface()->DrawSetTexture( m_iTexture );
	vgui::surface()->DrawSetColor( m_bActive ? m_clrActive : m_clrInActive );
	vgui::surface()->DrawTexturedPolygon( 4, verts );
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CHudCapturePanel::CHudCapturePanel( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudCapturePanel" ) 
{
	SetParent( g_pClientMode->GetViewport() );

	m_iCurrentCP = -1;
	m_bFakingCapture = false;

	m_pBackground = new vgui::Panel( this, "CapturePanelBackground" );
	m_pProgressBar = new CHudCapturePanelProgressBar( this, "CapturePanelProgressBar" );

	for ( int i = 0 ; i < 5 ; i++ )
	{
		CHudCapturePanelIcon *pPanel;
		char szName[64];

		Q_snprintf( szName, sizeof( szName ), "CapturePanelPlayerIcon%d", i + 1 );
		pPanel = new CHudCapturePanelIcon( this, szName );

		m_PlayerIcons.AddToTail( pPanel );
	}

	m_bInitializedFlags = false;
	for ( int i = 0; i < MAX_TEAMS; i++ )
	{
		m_pTeamFlags[i] = NULL;
	}

	m_pMessage = new vgui::Label( this, "CapturePanelMessage", " " );

	// load control settings...
	LoadControlSettings( "resource/UI/HudCapturePanel.res" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudCapturePanel::Init( void )
{
	ListenForGameEvent( "controlpoint_starttouch" );
	ListenForGameEvent( "controlpoint_endtouch" );
	ListenForGameEvent( "teamplay_round_start" );
	ListenForGameEvent( "controlpoint_fake_capture" );
	ListenForGameEvent( "controlpoint_fake_capture_mult" );
	ListenForGameEvent( "intro_finish" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudCapturePanel::LevelInit( void )
{
	m_iCurrentCP = -1;
	m_bFakingCapture = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudCapturePanel::OnScreenSizeChanged( int iOldWide, int iOldTall )
{
	LoadControlSettings( "resource/UI/HudCapturePanel.res" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudCapturePanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	if ( m_pBackground )
	{
		m_pBackground->SetBgColor( GetSchemeColor( "HintMessageBg", pScheme ) );
		m_pBackground->SetPaintBackgroundType( 2 );
	}

	SetFgColor( GetSchemeColor( "HudProgressBar.Active", pScheme ) );

	int iX;
	GetPos( iX, m_iOriginalYPos );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudCapturePanel::OnThink()
{
	BaseClass::OnThink();

	if ( !GetNumberOfTeams() )
		return;

	if ( !m_bInitializedFlags )
	{
		m_bInitializedFlags = true;
		for ( int i = 0; i < GetNumberOfTeams(); i++ )
		{
			if ( i == TEAM_SPECTATOR )
				continue;

			m_pTeamFlags[i] = dynamic_cast< vgui::ImagePanel * >(FindChildByName( VarArgs("CapturePanelTeamFlag_%d", i) ));
		}
		InvalidateLayout();
	}

	if ( m_bFakingCapture && gpGlobals->curtime > m_flFakeCaptureTime )
	{
		m_bFakingCapture = false;
		if ( m_bFakingMultCapture )
		{
			for ( int i = 0; i < INTRO_NUM_FAKE_PLAYERS; i++ )
			{
				m_pFakePlayers[i]->Release();
				m_pFakePlayers[i] = NULL;
			}
		}
	}

	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();

	if ( pPlayer )
	{
		bool bInCapZone = ( m_iCurrentCP >= 0 );

		// Turn off the panel and children if the player is dead or not in a cap zone
		if ( !m_bFakingCapture && (!bInCapZone || !hud_capturepanel.GetBool() || !pPlayer->IsAlive()) )
		{
			if ( IsVisible() )
			{
				SetVisible( false );
			}
			return;
		}

		int nOwningTeam = ObjectiveResource()->GetOwningTeam( m_iCurrentCP );
		int nPlayerTeam = pPlayer->GetTeamNumber();

		int nNumTeammates = ObjectiveResource()->GetNumPlayersInArea( m_iCurrentCP, nPlayerTeam );
		int nRequiredTeammates = ObjectiveResource()->GetRequiredCappers( m_iCurrentCP, nPlayerTeam );

		int nNumEnemies = 0;
		bool bEnemyTeamReadyToCap = false;
		for ( int i = LAST_SHARED_TEAM+1; i < GetNumberOfTeams(); i++ )
		{
			if ( i == nPlayerTeam )
				continue;

			int iTeamInArea = ObjectiveResource()->GetNumPlayersInArea( m_iCurrentCP, i );
			nNumEnemies += iTeamInArea;

			if ( iTeamInArea >= ObjectiveResource()->GetRequiredCappers( m_iCurrentCP, i ) )
			{
				// There's an enemy team here that has enough players to cap
				bEnemyTeamReadyToCap = true;
			}
		}

		int iCappingTeam = ObjectiveResource()->GetCappingTeam( m_iCurrentCP );
		
		// If we're faking it, stomp all the data
		if ( m_bFakingCapture )
		{
			nOwningTeam = TEAM_UNASSIGNED;
			iCappingTeam = nPlayerTeam;

			if ( m_bFakingMultCapture )
			{
				nNumTeammates = nRequiredTeammates = 3;
			}
			else
			{
				nNumTeammates = nRequiredTeammates = 1;
			}
			nNumEnemies = 0;
			bEnemyTeamReadyToCap = false;
		}

		// If we're in more-players-cap-faster mode, we have no required amount. 
		// Just show the number of players in the zone.
		if ( mp_capstyle.GetInt() == 1 )
		{
			// Clip to max number of players we can show
			if ( nNumTeammates > 5 )
			{
				nNumTeammates = 5;
			}

			nRequiredTeammates = nNumTeammates;
		}

		// if we already own this capture point and there are no enemies in the area
		// or we're playing minirounds and the current cap zone is not in the current round
		if ( ( nOwningTeam == nPlayerTeam && !bEnemyTeamReadyToCap ) ||
			 ( ObjectiveResource()->PlayingMiniRounds() && !ObjectiveResource()->IsInMiniRound( m_iCurrentCP ) ) )
		{
			// don't need to do anything
			if ( IsVisible() )
			{
				SetVisible( false );
			}
			return;		
		}

		// okay, turn on the capture point panel
		if ( !IsVisible() )
		{
			SetVisible( true );
		}

		// If there's a hint onscreen, move ourselves off it
		int iX,iY;
		GetPos( iX, iY );
		if ( pPlayer->Hints() && pPlayer->Hints()->HintIsCurrentlyVisible() )
		{
			int iMovedY = (m_iOriginalYPos - YRES(50));
			if ( iY != iMovedY )
			{
				SetPos( iX, iMovedY );
			}
		}
		else if ( iY != m_iOriginalYPos )
		{
			SetPos( iX, m_iOriginalYPos );
		}

		// set the correct flag image
		for ( int i = 0; i < GetNumberOfTeams(); i++ )
		{
			if ( !m_pTeamFlags[i] )
				continue;

			m_pTeamFlags[i]->SetVisible( nOwningTeam == i );
		}

		// arrange the player icons
		for ( int i = 0 ; i < m_PlayerIcons.Count() ; i++ )
		{
			CHudCapturePanelIcon *pPanel = m_PlayerIcons[i];

			if ( !pPanel )
			{
				continue;
			}

			if ( i < nRequiredTeammates )
			{
				if ( i < nNumTeammates )
				{
					pPanel->SetActive( true );

					if ( !pPanel->IsVisible() )
						pPanel->SetVisible( true );
				}
				else
				{
					pPanel->SetActive( false );

					if ( !pPanel->IsVisible() )
						pPanel->SetVisible( true );
				}
			}
			else
			{
				if ( pPanel->IsVisible() )
					pPanel->SetVisible( false );
			}
		}

		int wide = 0, tall = 0, iconWide = 0, iconTall = 0;
		GetSize( wide, tall );

		vgui::ImagePanel *pPanel = m_PlayerIcons[0];
		if ( pPanel )
			pPanel->GetSize( iconWide, iconTall );
		
        int width = ( nRequiredTeammates * iconWide ) + ( ( nRequiredTeammates - 1 ) * m_nSpaceBetweenIcons );
		int xpos = wide / 2.0 - width / 2.0;

		// rearrange the player icon panels
		for ( int i = 0 ; i < nRequiredTeammates ; i++ )
		{
			CHudCapturePanelIcon *pPanel = m_PlayerIcons[i];
 
			if ( pPanel )
			{
				int x, y, w, t;
				pPanel->GetBounds( x, y, w, t );
				pPanel->SetBounds( xpos, y, w, t );
			}

			xpos += iconWide + m_nSpaceBetweenIcons;
		}

		// are we capping an area?
		if ( iCappingTeam == TEAM_UNASSIGNED || iCappingTeam != nPlayerTeam )
		{
			// turn off the progress bar, we're not capping
			if ( m_pProgressBar && m_pProgressBar->IsVisible() )
			{
				m_pProgressBar->SetVisible( false );
			}

			// turn on the message
			if ( m_pMessage )
			{
				m_pMessage->SetFgColor( GetFgColor() );

				if ( !m_pMessage->IsVisible() )
				{
					m_pMessage->SetVisible( true );
				}

				char szReason[256];

				// If a team's not allowed to cap a point, don't count players in it at all
				if ( !TeamplayGameRules()->TeamMayCapturePoint( nPlayerTeam, m_iCurrentCP ) )
				{
					m_pMessage->SetText( "#Team_Capture_Linear" );

					if ( m_pTeamFlags[ nOwningTeam ] )
					{
						m_pTeamFlags[ nOwningTeam ]->SetVisible( false );
					}
				}
				else if ( !TeamplayGameRules()->PlayerMayCapturePoint( pPlayer, m_iCurrentCP, szReason, sizeof(szReason) ) )
				{
					m_pMessage->SetText( szReason );

					if ( m_pTeamFlags[ nOwningTeam ] )
					{
						m_pTeamFlags[ nOwningTeam ]->SetVisible( false );
					}
				}
				else if ( nNumTeammates >= nRequiredTeammates && nNumEnemies > 0 )
				{
					m_pMessage->SetText( "#Team_Capture_Blocked" );
				}
				else if ( bEnemyTeamReadyToCap )
				{
					m_pMessage->SetText( "#Team_Blocking_Capture" );
				}
				else if ( mp_blockstyle.GetInt() == 1 && iCappingTeam != TEAM_UNASSIGNED )
				{
					m_pMessage->SetText( "#Team_Blocking_Capture" );

					for ( int i = 0; i < GetNumberOfTeams(); i++ )
					{
						if ( m_pTeamFlags[i] )
						{
							m_pTeamFlags[i]->SetVisible( false );
						}
					}
				}
				else if ( !ObjectiveResource()->TeamCanCapPoint( m_iCurrentCP, nPlayerTeam ) )
				{
					m_pMessage->SetText( "#Team_Cannot_Capture" );

					if ( m_pTeamFlags[ nOwningTeam ] )
					{
						m_pTeamFlags[ nOwningTeam ]->SetVisible( false );
					}
				}
				else
				{
					m_pMessage->SetText( "#Team_Waiting_for_teammate" );
				}

				if ( m_pBackground )
				{
					// do we need to resize our background?
					int textW, textH, bgX, bgY, bgW, bgH;
					m_pMessage->GetContentSize( textW, textH );
					m_pBackground->GetBounds( bgX, bgY, bgW, bgH );

					if ( bgW < textW )
					{
						m_pBackground->SetBounds( bgX + ( bgW / 2.0 ) - ( ( textW + XRES(3) ) / 2.0 ), bgY, textW + XRES(3), bgH );
					}
				}
			}
		}
		else
		{
			// turn on the progress bar, we're capping
			if ( m_pProgressBar )
			{
				if ( !m_pProgressBar->IsVisible() )
				{
					m_pProgressBar->SetVisible( true );
				}

				if ( m_bFakingCapture )
				{
					float flProgress = RemapVal( m_flFakeCaptureTime - gpGlobals->curtime, 0, 5.0, 0, 1 );
					m_pProgressBar->SetPercentage( flProgress );
				}
				else
				{
					m_pProgressBar->SetPercentage( ObjectiveResource()->GetCPCapPercentage( m_iCurrentCP ) );
				}
			}

			// If our cap is being paused by blocking enemies, show that
			if ( mp_blockstyle.GetInt() == 1 && nNumTeammates == 0 )
			{
				m_pMessage->SetText( "#Team_Capture_Blocked" );

				if ( !m_pMessage->IsVisible() )
				{
					m_pMessage->SetVisible( true );
				}

				for ( int i = 0; i < GetNumberOfTeams(); i++ )
				{
					if ( m_pTeamFlags[i] )
					{
						m_pTeamFlags[i]->SetVisible( false );
					}
				}
			}
			else if ( m_pMessage && m_pMessage->IsVisible() )
			{
				// turn off the message
				m_pMessage->SetVisible( false );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudCapturePanel::FireGameEvent( IGameEvent *event )
{
	m_iCurrentCP = -1;
	return;

	const char *eventname = event->GetName();
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();

	if ( FStrEq( "controlpoint_starttouch", eventname ) )
	{
		int iPlayer = event->GetInt( "player" );
		if ( pPlayer && iPlayer == pPlayer->entindex() )
		{
			m_iCurrentCP = event->GetInt( "area" );
		}
	}
	else if ( FStrEq( "controlpoint_endtouch", eventname ) )
	{
		int iPlayer = event->GetInt( "player" );
		if ( pPlayer && iPlayer == pPlayer->entindex() )
		{
			Assert( m_iCurrentCP == event->GetInt( "area" ) );
			m_iCurrentCP = -1;
		}
	}
	else if ( FStrEq( "teamplay_round_start", eventname ) )
	{
		m_iCurrentCP = -1;
	}
	else if ( FStrEq( "controlpoint_fake_capture", eventname ) )
	{
		int iPlayer = event->GetInt( "player" );
		if ( pPlayer && iPlayer == pPlayer->entindex() )
		{
			m_iCurrentCP = event->GetInt( "int_data" );
			m_bFakingCapture = true;
			m_bFakingMultCapture = false;
			m_flFakeCaptureTime = gpGlobals->curtime + 5.0;
		}
	}
	else if ( FStrEq( "controlpoint_fake_capture_mult", eventname ) )
	{
		int iPlayer = event->GetInt( "player" );
		if ( pPlayer && iPlayer == pPlayer->entindex() )
		{
			m_iCurrentCP = event->GetInt( "int_data" );
			m_bFakingCapture = true;
			m_bFakingMultCapture = true;
			m_flFakeCaptureTime = gpGlobals->curtime + 5.0;

			// Trace forward & find the world
			trace_t tr;
			Vector vecEnd;
			VectorMA( MainViewOrigin(), MAX_TRACE_LENGTH, MainViewForward(), vecEnd );
			UTIL_TraceLine( MainViewOrigin(), vecEnd, MASK_SOLID_BRUSHONLY, pPlayer, COLLISION_GROUP_NONE, &tr );
			if ( !tr.startsolid && tr.fraction < 1.0 )
			{
				Vector vecPositions[INTRO_NUM_FAKE_PLAYERS] = 
				{
					Vector( 100, 100, 0 ),
					Vector( 0, -100, 0 ),
					Vector( -100, 0, 0 ),
				};
				const char *pszModels[INTRO_NUM_FAKE_PLAYERS] =
				{
					"models/player/engineer.mdl",
					"models/player/medic.mdl",
					"models/player/soldier.mdl",
				};
				for ( int i = 0; i < INTRO_NUM_FAKE_PLAYERS; i++ )
				{
					m_pFakePlayers[i] = new C_BaseAnimating;
					if ( m_pFakePlayers[i]->InitializeAsClientEntity( pszModels[i], RENDER_GROUP_OPAQUE_ENTITY ) )
					{
						Vector vecOrigin = tr.endpos + vecPositions[i];
						m_pFakePlayers[i]->SetAbsOrigin( vecOrigin );
						m_pFakePlayers[i]->SetAbsAngles( QAngle(0,RandomInt(0,360),0) );
					}
				}
			}
		}
	}
	else if ( FStrEq( "intro_finish", eventname ) )
	{
		int iPlayer = event->GetInt( "player" );
		if ( pPlayer && iPlayer == pPlayer->entindex() )
		{
			m_iCurrentCP = -1;
			m_bFakingCapture = false;
			m_bFakingMultCapture = false;
			m_flFakeCaptureTime = 0;

			for ( int i = 0; i < INTRO_NUM_FAKE_PLAYERS; i++ )
			{
				if ( m_pFakePlayers[i] )
				{
					m_pFakePlayers[i]->Release();
					m_pFakePlayers[i] = NULL;
				}
			}
		}
	}
}

