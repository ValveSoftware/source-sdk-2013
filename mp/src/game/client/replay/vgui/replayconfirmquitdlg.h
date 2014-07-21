//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef REPLAYBROWSER_CONFIRMQUITDLG_H
#define REPLAYBROWSER_CONFIRMQUITDLG_H
#ifdef _WIN32
#pragma once
#endif

#include "replaybrowserbasepanel.h"
#include "vgui/IScheme.h"
#include "vgui_controls/CheckButton.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CExButton;

class CReplayConfirmQuitDialog : public CReplayBasePanel
{
	DECLARE_CLASS_SIMPLE( CReplayConfirmQuitDialog, CReplayBasePanel );
public:
	CReplayConfirmQuitDialog( Panel *pParent );

	virtual void	ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void	OnCommand( const char *pCommand );
	virtual void	OnKeyCodeTyped( vgui::KeyCode code );
	virtual void	OnKeyCodePressed( vgui::KeyCode code );

private:
	vgui::CheckButton			*m_pDontShowAgain;
	CExButton					*m_pQuitButton;
};

//-----------------------------------------------------------------------------

bool ReplayUI_ShowConfirmQuitDlg();

//-----------------------------------------------------------------------------

#endif // REPLAYBROWSER_CONFIRMQUITDLG_H
