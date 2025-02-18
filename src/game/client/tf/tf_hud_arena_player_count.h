//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef TF_HUD_ARENA_PLAYER_COUNT_H
#define TF_HUD_ARENA_PLAYER_COUNT_H
#ifdef _WIN32
#pragma once
#endif

#include "hudelement.h"
#include "tf_imagepanel.h"
#include "tf_controls.h"
#include <vgui_controls/EditablePanel.h>

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CHudArenaPlayerCount : public CHudElement, public EditablePanel
{
	DECLARE_CLASS_SIMPLE( CHudArenaPlayerCount, EditablePanel );

public:
	CHudArenaPlayerCount( const char *pElementName );

	virtual void ApplySchemeSettings( IScheme *scheme );
	virtual bool ShouldDraw( void );
//	virtual void FireGameEvent( IGameEvent * event );
	virtual void OnTick( void );

private:
	void UpdateCounts( void );

private:

	EditablePanel *m_pBlueTeam;
	EditablePanel *m_pRedTeam;

};

#endif // TF_HUD_ARENA_PLAYER_COUNT_H
