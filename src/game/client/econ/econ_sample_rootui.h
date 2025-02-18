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
#include "backpack_panel.h"

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

	// Sub panel access.
	// These are panels that are parented to the root EconUI.
	virtual CBackpackPanel *GetBackpackPanel( void ) { return NULL; }
	virtual CCraftingPanel *GetCraftingPanel( void ) { return NULL; }

	// Gamestats access
	virtual void		Gamestats_ItemTransaction( int eventID, CEconItemView *item, const char *pszReason = NULL, int iQuality = 0 ) { return; }
	virtual void		Gamestats_Store( int eventID, CEconItemView* item=NULL, const char* panelName=NULL, 
		int classId=0, const cart_item_t* in_cartItem=NULL, int in_checkoutAttempts=0, const char* storeError=NULL, int in_totalPrice=0, int in_currencyCode=0 ) { return; }
	virtual void		SetExperimentValue( uint64 experimentValue ) { return; }

	// Open separate economy panels (they're not parented to the root EconUI)
	// This is here so that games can customize the implementation of these panels.
	CItemPickupPanel	*OpenItemPickupPanel( void );
	CItemDiscardPanel	*OpenItemDiscardPanel( void );
	// Store
	virtual void		CreateStorePanel( void );
	virtual CStorePanel	*OpenStorePanel( int iItemDef, bool bAddToCart );
	virtual CStorePanel	*GetStorePanel( void );

	// When the root UI is closed, send an "EconUIClosed" message to pListener.
	virtual void		AddPanelCloseListener( vgui::Panel *pListener )	{ AssertMsg( 0, "Implement me!" ); }

	// The panel at which we want back to actually close the UI - defaults to the root panel - a negative value can be passed in for class loadout panels
	virtual void		SetClosePanel( int iPanel ) { AssertMsg( 0, "Implement me!" ); }

	// Call this to set which team the class loadout should display
	virtual void		SetDefaultTeam( int iTeam ) { AssertMsg( 0, "Implement me!" ); }

protected:
	void				OpenSubPanel( EconBaseUIPanels_t nPanel );
	void				UpdateSubPanelVisibility( void );
	void				OpenTradingStartDialog( void );

private:
	bool				m_bPreventClosure;
	bool				m_bCheckForRoomOnExit;
	EconBaseUIPanels_t	m_nVisiblePanel;

	CBackpackPanel		*m_pBackpackPanel;
};

#endif // ECON_SAMPLE_ROOTUI_H
