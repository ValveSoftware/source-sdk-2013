//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//===========================================================================//

#include "matsys_controls/curveeditorpanel.h"
#include "matsys_controls/matsyscontrols.h"
#include "tier1/KeyValues.h"
#include "vgui/ISurface.h"
#include "vgui/IInput.h"
#include "vgui/MouseCode.h"

using namespace vgui;



//-----------------------------------------------------------------------------
// constructor, destructor
//-----------------------------------------------------------------------------
CCurveEditorPanel::CCurveEditorPanel( vgui::Panel *pParent, const char *pName ) : BaseClass( pParent, pName )
{
	m_nSelectedPoint = -1;
	SetMouseInputEnabled( true );
	SetKeyBoardInputEnabled( true );
	m_nHighlightedPoint = -1;
}

CCurveEditorPanel::~CCurveEditorPanel()
{
}


//-----------------------------------------------------------------------------
// Converts screen location to normalized values
//-----------------------------------------------------------------------------
void CCurveEditorPanel::ScreenToValue( int x, int y, float *pIn, float *pOut )
{
	int w, h;
	GetSize( w, h );

	*pIn = (float)x / (w-1);
	*pOut = 1.0f - ((float)y / (h-1));
}

void CCurveEditorPanel::ValueToScreen( float flIn, float flOut, int *x, int *y )
{
	int w, h;
	GetSize( w, h );

	*x = (int)(flIn * (w-1) + 0.5f);
	*y = (h-1) - (int)(flOut * (h-1) + 0.5f);
}


//-----------------------------------------------------------------------------
// Handle input
//-----------------------------------------------------------------------------
void CCurveEditorPanel::OnMousePressed( vgui::MouseCode code )
{
	BaseClass::OnMousePressed( code );

	int x, y;
	input()->GetCursorPos( x, y );
	ScreenToLocal( x, y );

	if ( code == MOUSE_LEFT )
	{
		int w, h;
		GetSize( w, h );

		float flIn, flOut;
		ScreenToValue( x, y, &flIn, &flOut );
		float flTolerance = 5.0f / (w-1);	// +/- 3 pixels to select the point
		m_nSelectedPoint = FindOrAddControlPoint( flIn, flTolerance, flOut );
	}
}

void CCurveEditorPanel::OnMouseReleased( vgui::MouseCode code )
{
	BaseClass::OnMouseReleased( code );

	if ( code == MOUSE_LEFT )
	{
		m_nSelectedPoint = -1;
	}
}

void CCurveEditorPanel::OnCursorMoved( int x, int y )
{
	BaseClass::OnCursorMoved( x, y );

    float flIn, flOut;
	ScreenToValue( x, y, &flIn, &flOut );

	int w, h;
	GetSize( w, h );
	float flTolerance = 5.0f / (w-1);	// +/- 3 pixels to select the point
	m_nHighlightedPoint = FindControlPoint( flIn, flTolerance );

	if ( m_nSelectedPoint < 0 )
		return;
	m_nSelectedPoint = ModifyControlPoint( m_nSelectedPoint, flIn, flOut );
}


//-----------------------------------------------------------------------------
// Handles keypresses
//-----------------------------------------------------------------------------
void CCurveEditorPanel::OnKeyCodePressed( vgui::KeyCode code )
{
	BaseClass::OnKeyCodePressed( code );

	if ( m_nSelectedPoint >= 0 )
	{
		RemoveControlPoint( m_nSelectedPoint );
		m_nSelectedPoint = -1;
	}
}

	
//-----------------------------------------------------------------------------
// This paints the grid behind the curves
//-----------------------------------------------------------------------------
void CCurveEditorPanel::PaintBackground( void )
{
	int w, h;
	GetSize( w, h );

	vgui::surface()->DrawSetColor( 255, 255, 255, 255 );
	vgui::surface()->DrawFilledRect( 0, 0, w, h );

	vgui::surface()->DrawSetColor( 128, 128, 128, 255 );
	vgui::surface()->DrawLine( 0, h/4, w, h/4 );
	vgui::surface()->DrawLine( 0, h/2, w, h/2 );
	vgui::surface()->DrawLine( 0, 3*h/4, w, 3*h/4 );

	vgui::surface()->DrawLine( w/4, 0, w/4, h );
	vgui::surface()->DrawLine( w/2, 0, w/2, h );
	vgui::surface()->DrawLine( 3*w/4, 0, 3*w/4, h );

	vgui::surface()->DrawSetColor( 0, 0, 0, 255 );
	vgui::surface()->DrawLine( 0, 0, w, 0 );
	vgui::surface()->DrawLine( w, 0, w, h );
	vgui::surface()->DrawLine( w, h, 0, h );
	vgui::surface()->DrawLine( 0, h, 0, 0 );
}


//-----------------------------------------------------------------------------
// Sets the color curves operation to edit
//-----------------------------------------------------------------------------
void CCurveEditorPanel::Paint( void )
{
	int w, h;
	GetSize( w, h );

	int x0 = 0, y0 = 0;

	// FIXME: Add method to draw multiple lines DrawPolyLine connects the 1st and last points... bleah
	vgui::surface()->DrawSetColor( 0, 0, 0, 255 );
	for ( int i = 0; i < w; ++i )
	{
		float flIn = (float)i / (w-1);
		float flOut = GetValue( flIn );
		int x = i;
		int y = (h-1) - (int)(flOut * (h-1) + 0.5f);
		y = clamp( y, 0, h-1 );
		
		if ( i != 0 )
		{
			vgui::surface()->DrawLine( x0, y0, x, y );
		}

		x0 = x; y0 = y;
	}


	// This will paint the control points
	// The currently selected one will be painted red and filled.
	for ( int i = ControlPointCount(); --i >= 0; )
	{
		float flIn, flOut;
		GetControlPoint( i, &flIn, &flOut );

		int cx, cy;
		ValueToScreen( flIn, flOut, &cx, &cy );

		if ( (i == m_nSelectedPoint) || ((m_nSelectedPoint == -1) && (i == m_nHighlightedPoint)) )
		{
			vgui::surface()->DrawSetColor( 255, 0, 0, 255 );
			vgui::surface()->DrawFilledRect( cx-3, cy-3, cx+3, cy+3 );
		}
		else
		{
			vgui::surface()->DrawSetColor( 0, 0, 0, 255 );
			vgui::surface()->DrawOutlinedRect( cx-3, cy-3, cx+3, cy+3 );
		}
	}
}