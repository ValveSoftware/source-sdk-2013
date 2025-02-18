//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include <assert.h>
#include <math.h>
#include <stdio.h>

#include <vgui_controls/RotatingProgressBar.h>

#include <vgui/IVGui.h>
#include <vgui/ILocalize.h>
#include <vgui/IScheme.h>
#include <vgui/ISurface.h>
#include <KeyValues.h>

#include "mathlib/mathlib.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

using namespace vgui;

DECLARE_BUILD_FACTORY( RotatingProgressBar );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
RotatingProgressBar::RotatingProgressBar(Panel *parent, const char *panelName) : ProgressBar(parent, panelName)
{
	m_flStartRadians = 0;
	m_flEndRadians = 0;
	m_flLastAngle = 0;

	m_nTextureId = -1;
	m_pszImageName = NULL;

	m_flTickDelay = 30;

	ivgui()->AddTickSignal(GetVPanel(), m_flTickDelay );

	SetPaintBorderEnabled(false);
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
RotatingProgressBar::~RotatingProgressBar()
{
	if ( vgui::surface() && m_nTextureId != -1 )
	{
		vgui::surface()->DestroyTextureID( m_nTextureId );
		m_nTextureId = -1;
	}

	delete [] m_pszImageName;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void RotatingProgressBar::ApplySettings(KeyValues *inResourceData)
{
	const char *imageName = inResourceData->GetString("image", "");
	if (*imageName)
	{
		SetImage( imageName );
	}

	// Find min and max rotations in radians
	m_flStartRadians = DEG2RAD(inResourceData->GetFloat( "start_degrees", 0 ) );
	m_flEndRadians = DEG2RAD( inResourceData->GetFloat( "end_degrees", 0 ) );

	// Start at 0 progress
	m_flLastAngle = m_flStartRadians;

	// approach speed is specified in degrees per second.
	// convert to radians per 1/30th of a second
	float flDegressPerSecond = DEG2RAD( inResourceData->GetFloat( "approach_speed", 360.0 ) );	// default is super fast
	m_flApproachSpeed = flDegressPerSecond * ( m_flTickDelay / 1000.0f );	// divide by number of frames in a second

	m_flRotOriginX = inResourceData->GetFloat( "rot_origin_x_percent", 0.5f );
	m_flRotOriginY = inResourceData->GetFloat( "rot_origin_y_percent", 0.5f );

	m_flRotatingX = inResourceData->GetFloat( "rotating_x", 0 );
	m_flRotatingY = inResourceData->GetFloat( "rotating_y", 0 );
	m_flRotatingWide = inResourceData->GetFloat( "rotating_wide", 0 );
	m_flRotatingTall = inResourceData->GetFloat( "rotating_tall", 0 );
	
	BaseClass::ApplySettings( inResourceData );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void RotatingProgressBar::ApplySchemeSettings(IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	if ( m_pszImageName && strlen( m_pszImageName ) > 0 )
	{
		if ( m_nTextureId == -1 )
		{
			m_nTextureId = surface()->CreateNewTextureID();
		}

		surface()->DrawSetTextureFile( m_nTextureId, m_pszImageName, true, false);
	}
}

//-----------------------------------------------------------------------------
// Purpose: sets an image by file name
//-----------------------------------------------------------------------------
void RotatingProgressBar::SetImage(const char *imageName)
{
	if ( m_pszImageName )
	{
		delete [] m_pszImageName;
		m_pszImageName = NULL;
	}

	const char *pszDir = "vgui/";
	int len = V_strlen(imageName) + 1;
	len += V_strlen(pszDir);
	m_pszImageName = new char[ len ];
	Q_snprintf( m_pszImageName, len, "%s%s", pszDir, imageName );
	InvalidateLayout(false, true); // force applyschemesettings to run
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void RotatingProgressBar::PaintBackground()
{
	// No background
}

//-----------------------------------------------------------------------------
// Purpose: Update event when we aren't drawing so we don't get huge sweeps
// when we start drawing it
//-----------------------------------------------------------------------------
void RotatingProgressBar::OnTick( void )
{
	float flDesiredAngle = RemapVal( GetProgress(), 0.0, 1.0, m_flStartRadians, m_flEndRadians );
	m_flLastAngle = Approach( flDesiredAngle, m_flLastAngle, m_flApproachSpeed );

	BaseClass::OnTick();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void RotatingProgressBar::Paint()
{
	// we have an image that we rotate based on the progress,
	// where '0' is not rotated,'90' is rotated 90 degrees to the right.
	// Image is rotated around its center.

	// desired rotation is GetProgress() ( 0.0 -> 1.0 ) mapped into
	// ( m_flStartDegrees -> m_flEndDegrees )

	vgui::surface()->DrawSetTexture( m_nTextureId );
	vgui::surface()->DrawSetColor( Color(255,255,255,255) );

	int wide, tall;
	GetSize( wide, tall );

	float mid_x = m_flRotatingX + m_flRotOriginX * m_flRotatingWide;
	float mid_y = m_flRotatingY + m_flRotOriginY * m_flRotatingTall;

	Vertex_t vert[4];	

	vert[0].Init( Vector2D( m_flRotatingX, m_flRotatingY ), Vector2D(0,0) );
	vert[1].Init( Vector2D( m_flRotatingX+m_flRotatingWide, m_flRotatingY ), Vector2D(1,0) );
	vert[2].Init( Vector2D( m_flRotatingX+m_flRotatingWide, m_flRotatingY+m_flRotatingTall ), Vector2D(1,1) );
	vert[3].Init( Vector2D( m_flRotatingX, m_flRotatingY+m_flRotatingTall ), Vector2D(0,1) );

	float flCosA = cos(m_flLastAngle);
	float flSinA = sin(m_flLastAngle);

	// rotate each point around (mid_x, mid_y) by flAngle radians
	for ( int i=0;i<4;i++ )
	{
		Vector2D result;

		// subtract the (x,y) we're rotating around, we'll add it on at the end.
		vert[i].m_Position.x -= mid_x;
		vert[i].m_Position.y -= mid_y;

		result.x = ( vert[i].m_Position.x * flCosA - vert[i].m_Position.y * flSinA ) + mid_x;
		result.y = ( vert[i].m_Position.x * flSinA + vert[i].m_Position.y * flCosA ) + mid_y;

		vert[i].m_Position = result;
	}

	vgui::surface()->DrawTexturedPolygon( 4, vert );
}