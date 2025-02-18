//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef TF_HUD_MINIGAME_H
#define TF_HUD_MINIGAME_H
#ifdef _WIN32
#pragma once
#endif

#include "hud.h"
#include "hudelement.h"
#include <vgui_controls/EditablePanel.h>
#include "tf_logic_halloween_2014.h"


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CHudMiniGame : public CHudElement, public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CHudMiniGame, vgui::EditablePanel );
public:
	CHudMiniGame( const char *pElementName );

	virtual void	ApplySchemeSettings( vgui::IScheme *scheme ) OVERRIDE;
	virtual bool	ShouldDraw( void ) OVERRIDE;
	virtual void	OnTick() OVERRIDE;

private:

	CTFMiniGame				*m_pActiveMinigame;
	char					m_szResFilename[MAX_PATH];
};

#endif // TF_HUD_MINIGAME_H
