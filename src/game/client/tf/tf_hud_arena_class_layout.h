//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef TF_HUD_ARENA_CLASS_LAYOUT_H
#define TF_HUD_ARENA_CLASS_LAYOUT_H
#ifdef _WIN32
#pragma once
#endif

#include "hudelement.h"
#include "tf_imagepanel.h"
#include "tf_controls.h"
#include <vgui_controls/EditablePanel.h>

using namespace vgui;

#define MAX_CLASS_IMAGES	12

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CHudArenaClassLayout : public CHudElement, public EditablePanel
{
	DECLARE_CLASS_SIMPLE( CHudArenaClassLayout, EditablePanel );

public:
	CHudArenaClassLayout( const char *pElementName );

	virtual void	Init( void );
	virtual void	ApplySchemeSettings( IScheme *scheme );
	virtual bool	ShouldDraw( void );
	virtual void	OnTick( void );
	virtual void	PerformLayout( void );
	bool HandleKeyCodePressed( vgui::KeyCode code );

	virtual int GetRenderGroupPriority() { return 40; }

	virtual void	SetVisible( bool state );

private:
	CTFImagePanel *m_pBackground;
	CTFImagePanel *m_pLocalPlayerBG;
	CExLabel *m_pTitle;
	CExLabel *m_pChangeLabel;
	CExLabel *m_pChangeLabelShadow;
	CTFImagePanel *m_ClassImages[MAX_CLASS_IMAGES];
};

#endif // TF_HUD_ARENA_CLASS_LAYOUT_H
