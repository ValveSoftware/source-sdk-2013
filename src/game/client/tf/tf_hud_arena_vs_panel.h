//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef TF_HUD_ARENA_VS_PANEL_H
#define TF_HUD_ARENA_VS_PANEL_H
#ifdef _WIN32
#pragma once
#endif

#include "hudelement.h"
#include <vgui_controls/EditablePanel.h>

using namespace vgui;

class CAvatarImagePanel;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CHudArenaVsPanel : public CHudElement, public EditablePanel
{
	DECLARE_CLASS_SIMPLE( CHudArenaVsPanel, EditablePanel );

public:
	CHudArenaVsPanel( const char *pElementName );

	virtual void	Init( void );
	virtual void	ApplySchemeSettings( IScheme *scheme );
	virtual bool	ShouldDraw( void );
	virtual void	OnTick( void );
	virtual void	FireGameEvent( IGameEvent * event );

private:
	bool m_bVisible;
	float m_flHideTime;

	EditablePanel *m_pBluePanel;
	EditablePanel *m_pRedPanel;

	CAvatarImagePanel *m_pBlueAvatar;
	CAvatarImagePanel *m_pRedAvatar;
};

#endif // TF_HUD_ARENA_VS_PANEL_H
