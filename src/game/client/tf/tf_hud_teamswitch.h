//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef TF_HUD_TEAMSWITCH_H
#define TF_HUD_TEAMSWITCH_H
#ifdef _WIN32
#pragma once
#endif

#include "hudelement.h"
#include <vgui_controls/EditablePanel.h>

using namespace vgui;

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CHudTeamSwitch : public CHudElement, public EditablePanel
{
	DECLARE_CLASS_SIMPLE( CHudTeamSwitch, EditablePanel );

public:
	CHudTeamSwitch( const char *pElementName );

	virtual void	Init( void );
	virtual void	OnTick( void );
	virtual void	LevelInit( void );
	virtual void	ApplySchemeSettings( IScheme *scheme );
	virtual bool	ShouldDraw( void );

	virtual void	FireGameEvent( IGameEvent * event );
	void			SetupSwitchPanel( int iNewTeam );

private:
	Label			*m_pBalanceLabel;
	float			m_flHideAt;
};

#endif // TF_HUD_TEAMSWITCH_H
