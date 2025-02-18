//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"

#include <vgui_controls/Label.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui_controls/RichText.h>
#include <vgui_controls/Frame.h>
#include <game/client/iviewport.h>
#include <KeyValues.h>
#include <filesystem.h>
#include "materialsystem/imaterialvar.h"
#include "IGameUIFuncs.h" // for key bindings

#include "tf_controls.h"
#include "tf_imagepanel.h"
#include "c_team_objectiveresource.h"
#include "c_tf_objective_resource.h"
#include "c_tf_player.h"

#include "tf_shareddefs.h"
#include "tf_roundinfo.h"


#include "vgui/ISurface.h"
#include <vgui/ILocalize.h>
#include <vgui/IVGui.h>
#include "engine/IEngineSound.h"

using namespace vgui;

const char *GetMapDisplayName( const char *mapName, bool bTitleCase = false );

class RoundInfoOverlay : public vgui::EditablePanel
{
public:
	DECLARE_CLASS_SIMPLE( RoundInfoOverlay, vgui::EditablePanel );

	RoundInfoOverlay( Panel *parent, const char *panelName ) : EditablePanel( parent, panelName )
	{
		m_iMode = 0;
		m_flModeChangeTime = -1;

		vgui::ivgui()->AddTickSignal( GetVPanel(), 100 );

		m_iBlueTeamTexture = vgui::surface()->CreateNewTextureID();
		vgui::surface()->DrawSetTextureFile(m_iBlueTeamTexture, "overviews/blueteam", true, false);

		m_iRedTeamTexture = vgui::surface()->CreateNewTextureID();
		vgui::surface()->DrawSetTextureFile(m_iRedTeamTexture, "overviews/redteam", true, false);

		m_iCapArrowTexture = vgui::surface()->CreateNewTextureID();
		vgui::surface()->DrawSetTextureFile(m_iCapArrowTexture, "overviews/caparrows", true, false);

		m_iFoundPoints = 0;
		m_iNextRoundPoints[0] = -1;
		m_iNextRoundPoints[1] = -1;

		m_iLastCappedPoint = -1;
	}

	virtual ~RoundInfoOverlay( void )
	{
		if ( vgui::surface() )
		{
			if ( m_iBlueTeamTexture != -1 )
			{
				vgui::surface()->DestroyTextureID( m_iBlueTeamTexture );
				m_iBlueTeamTexture = -1;
			}

			if ( m_iRedTeamTexture != -1 )
			{
				vgui::surface()->DestroyTextureID( m_iRedTeamTexture );
				m_iRedTeamTexture = -1;
			}

			if ( m_iCapArrowTexture != -1 )
			{
				vgui::surface()->DestroyTextureID( m_iCapArrowTexture );
				m_iCapArrowTexture = -1;
			}
		}
	}

	void Update( const char *szMapName );

	virtual void Paint();

	void SetState( int iPrevState, int iCurrentState, int iNextBattles );

	virtual void OnTick( void );

	void DrawTeamIcon( int x, int y, bool bBlueTeam, float flBloat = 1.0f );
	void DrawCapArrows( int x0, int y0, int x1, int y1 );

private:

	// structure to hold a single control point
	typedef struct 
	{
		char m_szName[64];
		int m_iXPos;
		int m_iYPos;
		bool m_bHideIcon;
	} roundinfo_control_point_t;

	CUtlVector < roundinfo_control_point_t > m_ControlPoints;

	int m_iPrevState;
	int m_iCurrentState;
	int m_iMiniRoundMask;

	// Time when we should change the text to the attack directive
	int m_iMode;	// 0 - start, 1 - previous round victory anim, 2 - attack directive, new round 
	float				m_flModeChangeTime;

	int m_iBlueTeamTexture;
	int m_iRedTeamTexture;
	int m_iCapArrowTexture;

	int m_iLastCappedPoint;

	int m_iFoundPoints;
	int m_iNextRoundPoints[2];
};

DECLARE_BUILD_FACTORY( RoundInfoOverlay );

void RoundInfoOverlay::Paint( void )
{
	BaseClass::Paint();

	if ( m_ControlPoints.Count() <= 0 )
	{
		return;
	}

	// Draw the Cap Icons

	for ( int i=0; i<m_ControlPoints.Count(); i++ )
	{
		if ( m_ControlPoints[i].m_bHideIcon )
			continue;

		int x = m_ControlPoints[i].m_iXPos;
		int y = m_ControlPoints[i].m_iYPos;

		switch( m_iMode )
		{
		case 0:	// Show previous state
			{
				if ( i != m_iLastCappedPoint )
				{
					bool bBlueTeam = ( m_iPrevState & (1<<i) );
					DrawTeamIcon( x, y, bBlueTeam );
				}
			}
			break;

		case 1:	// Animate the point being capped
			{
				bool bWasBlueTeam = ( m_iPrevState & (1<<i) );

				if ( i == m_iLastCappedPoint )
				{
					float flTimeUntilChange = m_flModeChangeTime - gpGlobals->curtime;

					if ( flTimeUntilChange < 0.4f )
					{
						float flBloat = RemapVal( flTimeUntilChange, 0.0f, 0.4f, 1.0f, 2.5f );

						DrawTeamIcon( x, y, !bWasBlueTeam, flBloat );
					}
				}
				else
				{
					DrawTeamIcon( x, y, bWasBlueTeam );
				}
			}
			break;

		case 2:	// Draw the current state and the next battle arrows
			{
				bool bPointInContention = (m_iNextRoundPoints[0] == i || m_iNextRoundPoints[1] == i );

				bool bBlueTeam = ( m_iCurrentState & (1<<i) );
				DrawTeamIcon( x, y, bBlueTeam, bPointInContention ? 1.4 : 1.0 );	// rescale? pop looks weird
			}
			break;
		}
	}

	if ( m_iMode == 2 )
	{
		if ( m_iFoundPoints == 2 )
		{
			if ( ( m_flModeChangeTime - gpGlobals->curtime ) < 3.5f )
			{
				DrawCapArrows( m_ControlPoints[m_iNextRoundPoints[0]].m_iXPos,
					m_ControlPoints[m_iNextRoundPoints[0]].m_iYPos,
					m_ControlPoints[m_iNextRoundPoints[1]].m_iXPos,
					m_ControlPoints[m_iNextRoundPoints[1]].m_iYPos );
			}
		}
	}
}

void RoundInfoOverlay::DrawCapArrows( int x0, int y0, int x1, int y1 )
{
	vgui::surface()->DrawSetColor( Color(255,255,255,255) );

	vgui::surface()->DrawSetTexture( m_iCapArrowTexture );

	Vector2D a( x0, y0 );
	Vector2D b( x1, y1 );

	Vector2D dir = b - a;

	Vector2D perp( -dir.y, dir.x );
	perp.NormalizeInPlace();
	perp *= YRES(50);
 
	float bloat = sin(4*gpGlobals->curtime) * 0.1f;

	Vector2D edgepoint = a + dir * 0.25f;
	Vector2D edgepoint2 = b - dir * 0.25f;

	edgepoint -= 0.25f * dir * bloat;
	edgepoint2 += 0.25f * dir * bloat;
	
	float uv1 = 0.0f, uv2 = 1.0f;
	Vector2D uv12( uv1, uv2 );
	Vector2D uv11( uv1, uv1 );
	Vector2D uv21( uv2, uv1 );
	Vector2D uv22( uv2, uv2 );

	vgui::Vertex_t verts[4];	
	verts[0].Init( edgepoint - perp * 0.5f, uv12 );
	verts[1].Init( edgepoint2 - perp * 0.5f, uv11 );
	verts[2].Init( edgepoint2 + perp * 0.5f, uv21 );
	verts[3].Init( edgepoint + perp * 0.5f, uv22  );

	vgui::surface()->DrawTexturedPolygon( 4, verts );	
}

void RoundInfoOverlay::DrawTeamIcon( int x, int y, bool bBlueTeam, float flBloat /* = 1.0f */ )
{
	float flWide = YRES(45) * flBloat;

	int xpos = x - flWide * 0.5f;
	int ypos = y - flWide * 0.5f;

	vgui::surface()->DrawSetColor( Color(255,255,255,255) );
	vgui::surface()->DrawSetTexture( bBlueTeam ? m_iBlueTeamTexture : m_iRedTeamTexture );
	vgui::surface()->DrawTexturedRect( xpos, ypos, xpos + flWide, ypos + flWide );
}

void RoundInfoOverlay::Update( const char *szMapName )
{
	KeyValues *kvCapPoints = NULL;

	char strFullpath[MAX_PATH];
	Q_strncpy( strFullpath, "resource/roundinfo/", MAX_PATH );	// Assume we must play out of the media directory
	Q_strncat( strFullpath, szMapName, MAX_PATH );

#ifdef _X360
	char *pExt = Q_stristr( strFullpath, ".360" );
	if ( pExt )
	{
		*pExt = '\0';
	}
#endif

	Q_strncat( strFullpath, ".res", MAX_PATH );		// Assume we're a .res extension type

	if ( g_pFullFileSystem->FileExists( strFullpath ), "MOD" )
	{
		kvCapPoints = new KeyValues( strFullpath );

		if ( kvCapPoints )
		{
			if ( kvCapPoints->LoadFromFile( g_pFullFileSystem, strFullpath ) )
			{
				m_ControlPoints.RemoveAll();

				for ( KeyValues *pData = kvCapPoints->GetFirstSubKey(); pData != NULL; pData = pData->GetNextKey() )
				{
					roundinfo_control_point_t point;

					Q_snprintf( point.m_szName, sizeof(point.m_szName), "%s", pData->GetName() );

					// These x,y coords are relative to a 640x480 parent panel.
					int wide, tall;
					GetSize( wide, tall );

					// can't use XRES, YRES because of widescreen
					point.m_iXPos =  (int)( (float)pData->GetInt( "x", 0 ) * ( ( float )wide / 560.0f ) );
					point.m_iYPos =  (int)( (float)pData->GetInt( "y", 0 ) * ( ( float )tall / 280.0f ) );

					point.m_bHideIcon = ( pData->GetInt( "hideicon", 0 ) > 0 );

					m_ControlPoints.AddToTail( point );
				}
			}

			kvCapPoints->deleteThis();
		}
	}
}

void RoundInfoOverlay::SetState( int iPrevState, int iCurrentState, int iNextBattles )
{	
	m_iPrevState = iPrevState;
	m_iCurrentState = iCurrentState;
	m_iMiniRoundMask = iNextBattles;

	m_iMode = 0;
	m_flModeChangeTime = gpGlobals->curtime + 0.5f;

	// Find the two points that are being fought over
	
	m_iFoundPoints = 0;

	for ( int i=0;i<8 && m_iFoundPoints<2;i++ )
	{
		if ( m_iMiniRoundMask & (1<<i) )
		{
			m_iNextRoundPoints[m_iFoundPoints] = i;
			m_iFoundPoints++;
		}
	}

	// Make sure the blue point is in m_iNextRoundPoints[0]
	if ( m_iFoundPoints >= 2 )
	{
		if ( !( m_iCurrentState & (1<<m_iNextRoundPoints[0]) ) )
		{
			// The first point is red! swap them
			int temp = m_iNextRoundPoints[0];
			m_iNextRoundPoints[0] = m_iNextRoundPoints[1];
			m_iNextRoundPoints[1] = temp;
		}
	}

	m_iLastCappedPoint = -1;

	// Find the index of the point that was just capped
	int iMaskedCappedPoint = m_iCurrentState ^ m_iPrevState;

	if ( iMaskedCappedPoint != 0 )
	{
		int iIndex = 0;

		// Find the index of the point that changed
		while ( !( iMaskedCappedPoint & 0x1 ) )
		{
			iMaskedCappedPoint = iMaskedCappedPoint>>1;
			iIndex++;
		}
		m_iLastCappedPoint = iIndex;
	}
}


ConVar tf_roundinfo_pause( "tf_roundinfo_pause", "0", FCVAR_DEVELOPMENTONLY );

void RoundInfoOverlay::OnTick( void )
{
	// Stop ticking when our parent is invisible
	Panel *parent = GetParent();
	if ( m_iMode >= 0 && ( !parent || !parent->IsVisible() ) )
	{
		m_iMode = -1;
		return;
	}

	BaseClass::OnTick();

	if ( tf_roundinfo_pause.GetBool() == false && m_flModeChangeTime <= gpGlobals->curtime )
	{
		switch( m_iMode )
		{
		case 0:
			{
				// start showing previous round anim
				if ( m_iCurrentState != m_iPrevState )
				{
					m_iMode = 1;
					m_flModeChangeTime = gpGlobals->curtime + 1.5f;
				}
				else
				{
					m_iMode = 2;
					m_flModeChangeTime = gpGlobals->curtime + 4.0f;
				}
			}
			break;

		case 1:
			{
				// start showing next round plan
				m_iMode = 2;
				m_flModeChangeTime = gpGlobals->curtime + 4.0f;

				CLocalPlayerFilter filter;
				C_BaseEntity::EmitSound( filter, SOUND_FROM_LOCAL_PLAYER, "Hud.EndRoundScored" );
			}
			break;

		case 2:
			{
				// we're done, hide the panel
				//GetParent()->OnCommand( "continue" );
				//m_iMode = -1;
			}
			break;

		default:
			break;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFRoundInfo::CTFRoundInfo( IViewPort *pViewPort ) : Frame( NULL, PANEL_ROUNDINFO )
{
	m_pViewPort = pViewPort;

	// load the new scheme early!!
	SetScheme( "ClientScheme" );

	SetTitleBarVisible( false );
	SetMinimizeButtonVisible( false );
	SetMaximizeButtonVisible( false );
	SetCloseButtonVisible( false );
	SetSizeable( false );
	SetMoveable( false );
	SetProportional( true );
	SetVisible( false );
	SetKeyBoardInputEnabled( true );

	m_pTitle = new CExLabel( this, "RoundTitle", " " );
	m_pMapImage = new ImagePanel( this, "MapImage" );

#ifdef _X360
	m_pFooter = new CTFFooter( this, "Footer" );
#else
	m_pContinue = new CExButton( this, "RoundContinue", "#TF_Continue" );
#endif

	m_pOverlay = new RoundInfoOverlay( this, "Overlay" );

	ListenForGameEvent( "game_newmap" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFRoundInfo::PerformLayout()
{
	BaseClass::PerformLayout();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFRoundInfo::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	LoadControlSettings( "Resource/UI/RoundInfo.res" );

	BaseClass::ApplySchemeSettings( pScheme );

	Update();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFRoundInfo::ShowPanel( bool bShow )
{
	if ( IsVisible() == bShow )
		return;

	if ( bShow )
	{
		// look for the textures we want to use and don't show the roundinfo panel if any are missing
		char temp[255];
		Q_snprintf( temp, sizeof( temp ), "VGUI/%s", m_szMapImage );
		IMaterial *pMapMaterial = materials->FindMaterial( temp, TEXTURE_GROUP_VGUI, false );
	
		// are we missing any of the images we want to show?
		if ( pMapMaterial && !IsErrorMaterial( pMapMaterial ) )
		{
			Activate();
		}
		else
		{
			SetVisible( false );
		}
	}
	else
	{
		SetVisible( false );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFRoundInfo::OnCommand( const char *command )
{
	if ( !Q_strcmp( command, "continue" ) )
	{
		m_pViewPort->ShowPanel( this, false );
	}
	else
	{
		BaseClass::OnCommand( command );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFRoundInfo::UpdateImage( ImagePanel *pImagePanel, const char *pszImageName )
{
	if ( pImagePanel && ( Q_strlen( pszImageName ) > 0 ) )
	{
		char szTemp[255];
		Q_snprintf( szTemp, sizeof( szTemp ), "VGUI/%s", pszImageName );

		IMaterial *pTemp = materials->FindMaterial( szTemp, TEXTURE_GROUP_VGUI, false );
		if ( pTemp && !IsErrorMaterial( pTemp ) )
		{
			pImagePanel->SetImage( pszImageName );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFRoundInfo::Update()
{
	char szMapName[MAX_MAP_NAME];
	Q_FileBase( engine->GetLevelName(), szMapName, sizeof(szMapName) );
	Q_strlower( szMapName );

	SetDialogVariable( "mapname", GetMapDisplayName( szMapName ) );

	if ( m_pMapImage )
	{
		char temp[255];
		Q_snprintf( temp, sizeof(temp), "../overviews/%s", szMapName );
		Q_strncpy(  m_szMapImage, temp, sizeof( m_szMapImage ) );

		UpdateImage( m_pMapImage, m_szMapImage );
	}

	if ( m_pOverlay )
	{
		m_pOverlay->Update( szMapName );
	}

#ifndef _X360
	if ( m_pContinue )
	{
		m_pContinue->RequestFocus();
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFRoundInfo::OnKeyCodePressed( KeyCode code )
{
	if( code == KEY_SPACE ||
		code == KEY_ENTER ||
		code == KEY_XBUTTON_A ||
		code == KEY_XBUTTON_B ||
		code == STEAMCONTROLLER_A ||
		code == STEAMCONTROLLER_B )
	{
		OnCommand( "continue" );
	}
	else
	{
		BaseClass::OnKeyCodePressed( code );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFRoundInfo::SetData( KeyValues *data )
{
	if ( m_pOverlay )
	{
		m_pOverlay->SetState( data->GetInt( "prev" ), data->GetInt( "cur" ), data->GetInt( "round" ) );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFRoundInfo::FireGameEvent( IGameEvent *event )
{
	if ( Q_strcmp( event->GetName(), "game_newmap" ) == 0 )
	{
		Update();
	}
}
