//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_IMAGEPANEL_H
#define TF_IMAGEPANEL_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_shareddefs.h"
#include <vgui/IScheme.h>
#include <vgui_controls/ScalableImagePanel.h>
#include "GameEventListener.h"

#define MAX_BG_LENGTH		128

class CTFImagePanel : public vgui::ScalableImagePanel, public CGameEventListener
{
public:
	DECLARE_CLASS_SIMPLE( CTFImagePanel, vgui::ScalableImagePanel );

	CTFImagePanel( vgui::Panel *parent, const char *name );

	virtual void ApplySettings( KeyValues *inResourceData );
	void UpdateBGImage( void );
	void SetBGTeam( int iTeam ) { m_iBGTeam = iTeam; }

public: // IGameEventListener Interface
	virtual void FireGameEvent( IGameEvent * event );

public:
	char	m_szTeamBG[TF_TEAM_COUNT][MAX_BG_LENGTH];
	int		m_iBGTeam;
};


#endif // TF_IMAGEPANEL_H
