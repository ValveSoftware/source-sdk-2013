//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef CSTEXTWINDOW_H
#define CSTEXTWINDOW_H
#ifdef _WIN32
#pragma once
#endif

#include "vguitextwindow.h"
#include <spectatorgui.h>

//-----------------------------------------------------------------------------
// Purpose: displays the MOTD
//-----------------------------------------------------------------------------

class CHL2MPTextWindow : public CTextWindow
{
private:
	DECLARE_CLASS_SIMPLE( CHL2MPTextWindow, CTextWindow );

public:
	CHL2MPTextWindow(IViewPort *pViewPort);
	virtual ~CHL2MPTextWindow();

	virtual void Update();
	virtual void SetVisible(bool state);
	virtual void ShowPanel( bool bShow );
	virtual void OnKeyCodePressed(vgui::KeyCode code);

protected:
	ButtonCode_t m_iScoreBoardKey;

	// Background panel -------------------------------------------------------

public:
	virtual void PaintBackground();
	virtual void PerformLayout();
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	bool m_backgroundLayoutFinished;

	// End background panel ---------------------------------------------------
};

class CHL2MPSpectatorGUI : public CSpectatorGUI
{
private:
	DECLARE_CLASS_SIMPLE( CHL2MPSpectatorGUI, CSpectatorGUI );

public:
	CHL2MPSpectatorGUI( IViewPort *pViewPort );

	virtual void Update( void );
	virtual bool NeedsUpdate( void );

protected:
	int		m_nLastSpecMode;
	CBaseEntity	*m_nLastSpecTarget;
};



#endif // CSTEXTWINDOW_H
