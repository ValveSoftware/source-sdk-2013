//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef CURVEEDITORPANEL_H
#define CURVEEDITORPANEL_H

#ifdef _WIN32
#pragma once
#endif


#include "vgui_controls/Panel.h"


//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
struct BGRA8888_t;


//-----------------------------------------------------------------------------
//
// Curve editor image panel
//
//-----------------------------------------------------------------------------
class CCurveEditorPanel : public vgui::Panel
{
	DECLARE_CLASS_SIMPLE( CCurveEditorPanel, vgui::Panel );

public:
	// constructor
	CCurveEditorPanel( vgui::Panel *pParent, const char *pName );
	~CCurveEditorPanel();

	virtual void Paint( void );
	virtual void PaintBackground( void );

	virtual void OnCursorMoved( int x,int y );
	virtual void OnMousePressed( vgui::MouseCode code );
	virtual void OnMouseReleased( vgui::MouseCode code );
	virtual void OnKeyCodePressed( vgui::KeyCode code );

protected:
	// Control points + values...
	virtual int FindOrAddControlPoint( float flIn, float flTolerance, float flOut ) = 0;
	virtual int FindControlPoint( float flIn, float flTolerance ) = 0;
	virtual int ModifyControlPoint( int nPoint, float flIn, float flOut ) = 0;
	virtual void RemoveControlPoint( int nPoint ) = 0;
	virtual float GetValue( float flIn ) = 0;
	virtual int ControlPointCount() = 0;
	virtual void GetControlPoint( int nPoint, float *pIn, float *pOut ) = 0;

private:
	// Converts screen location to normalized values and back
	void ScreenToValue( int x, int y, float *pIn, float *pOut );
	void ValueToScreen( float flIn, float flOut, int *x, int *y );

	int m_nSelectedPoint;
	int m_nHighlightedPoint;	// Used when not selecting
};



#endif // CURVEEDITORPANEL_H
