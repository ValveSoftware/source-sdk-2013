//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef NAVPROGRESS_H
#define NAVPROGRESS_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/Frame.h>
#include <vgui_controls/ProgressBar.h>

#include <game/client/iviewport.h>

class CNavProgress : public vgui::Frame, public IViewPortPanel
{
private:
	DECLARE_CLASS_SIMPLE( CNavProgress, vgui::Frame );

public:
	CNavProgress(IViewPort *pViewPort);
	virtual ~CNavProgress();

	virtual const char *GetName( void ) { return PANEL_NAV_PROGRESS; }
	virtual void SetData(KeyValues *data);
	virtual void Reset();
	virtual void Update();
	virtual bool NeedsUpdate( void ) { return false; }
	virtual bool HasInputElements( void ) { return true; }
	virtual void ShowPanel( bool bShow );

	// both vgui::Frame and IViewPortPanel define these, so explicitly define them here as passthroughs to vgui
	vgui::VPANEL GetVPanel( void ) { return BaseClass::GetVPanel(); }
  	virtual bool IsVisible() { return BaseClass::IsVisible(); }
  	virtual void SetParent( vgui::VPANEL parent ) { BaseClass::SetParent( parent ); }

	virtual GameActionSet_t GetPreferredActionSet() { return GAME_ACTION_SET_NONE; }

public:

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void PerformLayout();
	void Init( const char *title, int numTicks, int currentTick );

protected:
	IViewPort	*m_pViewPort;

	int		m_numTicks;
	int		m_currentTick;

	vgui::Label *			m_pTitle;
	vgui::Label *			m_pText;
	vgui::Panel *			m_pProgressBarBorder;
	vgui::Panel *			m_pProgressBar;
	vgui::Panel *			m_pProgressBarSizer;
};

#endif // NAVPROGRESS_H
