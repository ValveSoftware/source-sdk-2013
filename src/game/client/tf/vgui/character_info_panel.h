//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef CHARACTER_INFO_PANEL_H
#define CHARACTER_INFO_PANEL_H
#ifdef _WIN32
#pragma once
#endif

#include "econ_ui.h"
#include "vgui_controls/PropertyDialog.h"
#include "tf_shareddefs.h"
#include "GameEventListener.h"
#include "vgui_controls/Panel.h"
#include "vgui_controls/PHandle.h"

class CCharInfoLoadoutSubPanel;
class CArmoryPanel;
class CBackpackPanel;
class CCraftingPanel;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CServerNotConnectedToSteamDialog : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CServerNotConnectedToSteamDialog, vgui::EditablePanel );

public:
	CServerNotConnectedToSteamDialog( vgui::Panel *pParent, const char *pElementName );

	virtual void	ApplySchemeSettings( vgui::IScheme *scheme );
	virtual void	OnCommand( const char *command );
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CCheatDetectionDialog : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CCheatDetectionDialog, vgui::EditablePanel );

public:
	CCheatDetectionDialog( vgui::Panel *pParent, const char *pElementName );

	virtual void	ApplySchemeSettings( vgui::IScheme *scheme );
	virtual void	OnCommand( const char *command );
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CCharacterInfoPanel : public vgui::PropertyDialog, public IEconRootUI, public CGameEventListener
{
	DECLARE_CLASS_SIMPLE( CCharacterInfoPanel, vgui::PropertyDialog );
public:
	CCharacterInfoPanel( Panel *parent );
	virtual ~CCharacterInfoPanel();

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void PerformLayout( void );
	virtual void OnCommand( const char *command );
	virtual void ShowPanel( bool bShow );
	virtual void OnKeyCodeTyped(vgui::KeyCode code) OVERRIDE;
	virtual void OnKeyCodePressed(vgui::KeyCode code) OVERRIDE;
	virtual void OnThink();

	void		 OpenLoadoutToClass( int iClassIndex, bool bOpenClassLoadout );
	void		 OpenLoadoutToBackpack( void );
	void		 OpenLoadoutToCrafting( void );
	void		 OpenLoadoutToArmory( void );
	void		 OpenToPaintkitPreview( CEconItemView* pItem, bool bFixedItem, bool bFixedPaintkit );
	void		 SetCheckForRoomOnExit( bool bCheck ) { m_bCheckForRoomOnExit = bCheck; }

	void		 FireGameEvent( IGameEvent *event );
	
	CArmoryPanel   *GetArmoryPanel( void );

	MESSAGE_FUNC_PARAMS( OnOpenArmoryDirect, "OpenArmoryDirect", data );

	//---------------------------------------
	// IEconRootUI
	virtual IEconRootUI	*OpenEconUI( int iDirectToPage = 0, bool bCheckForInventorySpaceOnExit = false );
	virtual void		CloseEconUI( void );
	virtual bool		IsUIPanelVisible( EconBaseUIPanels_t iPanel );
	virtual void		SetPreventClosure( bool bPrevent ) OVERRIDE;

	// Sub panel access.
	// These are panels that are parented to the root EconUI.
	virtual CBackpackPanel *GetBackpackPanel( void );
	virtual CCraftingPanel *GetCraftingPanel( void );

	// Gamestats access
	virtual void		Gamestats_ItemTransaction( int eventID, CEconItemView *item, const char *pszReason = NULL, int iQuality = 0 );
	virtual void		Gamestats_Store( int eventID, CEconItemView* item=NULL, const char* panelName=NULL, 
		int classId=0, const cart_item_t* in_cartItem=NULL, int in_checkoutAttempts=0, const char* storeError=NULL, int in_totalPrice=0, int in_currencyCode=0 );
	virtual void		SetExperimentValue( uint64 experimentValue );

	// Open separate economy panels (they're not parented to the root EconUI)
	// This is here so that games can customize the implementation of these panels.
	virtual CItemPickupPanel	*OpenItemPickupPanel( void );
	virtual CItemDiscardPanel	*OpenItemDiscardPanel( void );
	virtual void				CreateStorePanel( void );
	virtual CStorePanel			*OpenStorePanel( int iItemDef, bool bAddToCart );
	virtual CStorePanel			*GetStorePanel( void );

	// When the root UI is closed, send an "EconUIClosed" message to pListener.
	virtual void		AddPanelCloseListener( vgui::Panel *pListener );

	// The panel at which we want back to actually close the UI - defaults to the root panel - a negative value can be passed in for class loadout panels
	virtual void		SetClosePanel( int iPanel );

	// Call this to set which team the class loadout should display
	virtual void		SetDefaultTeam( int iTeam );

private:
	void Close();
	void NotifyListenersOfCloseEvent();

	vgui::Panel					*m_pNotificationsPresentPanel;
	CCharInfoLoadoutSubPanel	*m_pLoadoutPanel;
	bool						m_bCheckForRoomOnExit;
	bool						m_bPreventClosure;
	int							m_iClosePanel;
	int							m_iDefaultTeam;

	CUtlVector< vgui::VPanelHandle >	m_vecOnCloseListeners;
};

CCheatDetectionDialog *OpenCheatDetectionDialog( vgui::Panel *pParent, const char *pszCheatMessage );

#endif // CHARACTER_INFO_PANEL_H
