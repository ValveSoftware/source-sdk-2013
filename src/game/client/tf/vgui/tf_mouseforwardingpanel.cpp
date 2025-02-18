//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#include "cbase.h"

#include "tf_mouseforwardingpanel.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

//=============================================================================//

DECLARE_BUILD_FACTORY( CMouseMessageForwardingPanel );
 
CMouseMessageForwardingPanel::CMouseMessageForwardingPanel( Panel *parent, const char *name ) : BaseClass( parent, name )
{
	// don't draw an
	SetPaintEnabled(false);
	SetPaintBackgroundEnabled(false);
	SetPaintBorderEnabled(false);
}

void CMouseMessageForwardingPanel::PerformLayout()
{
	// fill out the whole area
	int w, t;
	GetParent()->GetSize(w, t);
	SetBounds(0, 0, w, t);
}

void CMouseMessageForwardingPanel::OnCursorEntered()
{
	if ( GetParent() )
	{
		GetParent()->OnCursorEntered();
	}
}

void CMouseMessageForwardingPanel::OnCursorExited()
{
	if ( GetParent() )
	{
		GetParent()->OnCursorExited();
	}
}

void CMouseMessageForwardingPanel::OnMousePressed( vgui::MouseCode code )
{
	if ( GetParent() )
	{
		GetParent()->OnMousePressed( code );
	}
}

void CMouseMessageForwardingPanel::OnMouseReleased( vgui::MouseCode code )
{
	if ( GetParent() )
	{
		GetParent()->OnMouseReleased( code );
	}
}

void CMouseMessageForwardingPanel::OnMouseDoublePressed( vgui::MouseCode code )
{
	if ( GetParent() )
	{
		GetParent()->OnMouseDoublePressed( code );
	}
}

void CMouseMessageForwardingPanel::OnMouseWheeled(int delta)
{
	if ( GetParent() )
	{
		GetParent()->OnMouseWheeled( delta );
	}
}

