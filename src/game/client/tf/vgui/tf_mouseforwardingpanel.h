//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================

#ifndef TF_MOUSEFORWARDINGPANEL_H
#define TF_MOUSEFORWARDINGPANEL_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/EditablePanel.h>

//-----------------------------------------------------------------------------
// Purpose: Invisible panel that forwards up mouse movement
//-----------------------------------------------------------------------------
class CMouseMessageForwardingPanel : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CMouseMessageForwardingPanel, vgui::EditablePanel );
public:
	CMouseMessageForwardingPanel( Panel *parent, const char *name );

	virtual void PerformLayout( void );
	virtual void OnCursorEntered();
	virtual void OnCursorExited();
	virtual void OnMousePressed( vgui::MouseCode code );
	virtual void OnMouseReleased( vgui::MouseCode code );
	virtual void OnMouseDoublePressed( vgui::MouseCode code );
	virtual void OnMouseWheeled(int delta);
};

#endif // TF_MOUSEFORWARDINGPANEL_H
