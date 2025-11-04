//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef ECON_SAMPLE_ROOTUI_H
#define ECON_SAMPLE_ROOTUI_H
#ifdef _WIN32
#pragma once
#endif

#include "econ_ui.h"
#include "vgui_controls/Frame.h"
#include "GameEventListener.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CEconSampleRootUI : public vgui::Frame, public IEconRootUI, public CGameEventListener
{
	DECLARE_CLASS_SIMPLE( CEconSampleRootUI, vgui::Frame );
public:
	CEconSampleRootUI( vgui::Panel *parent );
	virtual ~CEconSampleRootUI();

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void PerformLayout( void );
	virtual void OnCommand( const char *command );
	virtual void ShowPanel( bool bShow );
	virtual void OnKeyCodeTyped(vgui::KeyCode code);

	void		 SetCheckForRoomOnExit( bool bCheck ) { m_bCheckForRoomOnExit = bCheck; }

	void		 FireGameEvent( IGameEvent *event );
	
	//---------------------------------------
	// IEconRootUI
	virtual IEconRootUI	*OpenEconUI( int iDirectToPage = 0, bool bCheckForInventorySpaceOnExit = false );
	virtual void		CloseEconUI( void );
	virtual bool		IsUIPanelVisible( EconBaseUIPanels_t iPanel );
	virtual void		SetPreventClosure( bool bPrevent ) { m_bPreventClosure = bPrevent; }

	// Gamestats access
	virtual void		Gamestats_ItemTransaction( int eventID, CEconItemView *item, const char *pszReason = NULL, int iQuality = 0 ) { return; }
	virtual void		SetExperimentValue( uint64 experimentValue ) { return; }

	// When the root UI is closed, send an "EconUIClosed" message to pListener.
	virtual void		AddPanelCloseListener( vgui::Panel *pListener )	{ AssertMsg( 0, "Implement me!" ); }

	// The panel at which we want back to actually close the UI - defaults to the root panel - a negative value can be passed in for class loadout panels
	virtual void		SetClosePanel( int iPanel ) { AssertMsg( 0, "Implement me!" ); }

	// Call this to set which team the class loadout should display
	virtual void		SetDefaultTeam( int iTeam ) { AssertMsg( 0, "Implement me!" ); }

protected:
	void				OpenSubPanel( EconBaseUIPanels_t nPanel );
	void				UpdateSubPanelVisibility( void );

private:
	bool				m_bPreventClosure;
	bool				m_bCheckForRoomOnExit;
	EconBaseUIPanels_t	m_nVisiblePanel;
};

#endif // ECON_SAMPLE_ROOTUI_H
