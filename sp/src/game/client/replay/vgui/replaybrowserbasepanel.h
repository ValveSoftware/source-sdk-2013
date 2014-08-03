//========= Copyright Valve Corporation, All rights reserved. ============//
//
//=======================================================================================//

#ifndef REPLAYBASEPANEL_H
#define REPLAYBASEPANEL_H
#ifdef _WIN32
#pragma once
#endif

#include "vgui_controls/EditablePanel.h"

//-----------------------------------------------------------------------------
// Purpose: Base panel for replay panels
//-----------------------------------------------------------------------------
class CReplayBasePanel : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CReplayBasePanel, vgui::EditablePanel );
public:
	CReplayBasePanel( Panel *pParent, const char *pName );

	void GetPosRelativeToAncestor( Panel *pAncestor, int &nXOut, int &nYOut );
};

#endif // REPLAYBASEPANEL_H
