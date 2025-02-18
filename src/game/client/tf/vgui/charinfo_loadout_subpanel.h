//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef CHARINFO_LOADOUT_SUBPANEL_H
#define CHARINFO_LOADOUT_SUBPANEL_H
#ifdef _WIN32
#pragma once
#endif

#include <game/client/iviewport.h>
#include "vgui_controls/PropertyPage.h"
#include <vgui_controls/Button.h>
#include "tf_controls.h"
#include "tf_shareddefs.h"
#include "item_pickup_panel.h"
#include "backpack_panel.h"
#include "class_loadout_panel.h"
#include "crafting_panel.h"
#include "charinfo_armory_subpanel.h"

#define NUM_CLASSES_IN_LOADOUT_PANEL		(TF_LAST_NORMAL_CLASS-1)		// We don't allow unlockables for the civilian

class CImageButton : public vgui::Button
{
private:
	DECLARE_CLASS_SIMPLE( CImageButton, vgui::Button );

public:
	CImageButton( vgui::Panel *parent, const char *panelName );

	virtual void ApplySettings( KeyValues *inResourceData );
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void OnSizeChanged( int newWide, int newTall );

	void SetActiveImage( const char *imagename );
	void SetInactiveImage( const char *imagename );
	void SetActiveImage( vgui::IImage *image );
	void SetInactiveImage( vgui::IImage *image );

public:
	virtual void Paint();

private:
	vgui::IImage *m_pActiveImage;	
	char *m_pszActiveImageName;

	vgui::IImage *m_pInactiveImage;
	char *m_pszInactiveImageName;

	bool m_bScaleImage;
	Color m_ActiveDrawColor;
	Color m_InactiveDrawColor;
};

enum charinfo_activepanels_t
{
	CHAP_LOADOUT,
	CHAP_BACKPACK,
	CHAP_CRAFTING,
	CHAP_ARMORY,
	CHAP_PAINTKIT_PREVIEW,
};

enum charinfosubbuttons_t
{
	CHSB_BACKPACK,
	CHSB_CRAFTING,
	CHSB_ARMORY,
	CHSB_TRADING,
	CHSB_PAINTKITS,

	CHSB_NUM_BUTTONS
};


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CCharInfoLoadoutSubPanel : public vgui::PropertyPage
{
	DECLARE_CLASS_SIMPLE( CCharInfoLoadoutSubPanel, vgui::PropertyPage );
public:
	CCharInfoLoadoutSubPanel(Panel *parent);
	virtual ~CCharInfoLoadoutSubPanel();

	virtual void	ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void	OnCommand( const char *command );
	virtual void	PerformLayout( void );
	virtual void	OnCursorMoved( int x, int y );

	void			SetClassIndex( int iClassIndex, bool bOpenClassLoadout );
	void			SetTeamIndex( int iTeamIndex );
	void			OpenToBackpack( void ) { OpenSubPanel( CHAP_BACKPACK ); }
	void			OpenToCrafting( void ) { OpenSubPanel( CHAP_CRAFTING ); }
	void			OpenToArmory( int iItemDef = 0 ) { m_iArmoryItemDef = iItemDef; OpenSubPanel( CHAP_ARMORY ); }
	void			OpenToPaintkitPreview( CEconItemView* pItem, bool bFixedItem, bool bFixedPaintkit );
	void			OpenSubPanel( charinfo_activepanels_t iPanel );
	void			UpdateModelPanels( bool bOpenClassLoadout = true );

	CClassLoadoutPanel	*GetClassLoadoutPanel( void ) { return m_pClassLoadoutPanel; }
	CBackpackPanel	*GetBackpackPanel( void ) { return m_pBackpackPanel; }
	CCraftingPanel	*GetCraftingPanel( void ) { return m_pCraftingPanel; }
	CArmoryPanel	*GetArmoryPanel( void ) { return m_pArmoryPanel; }
	CTFItemInspectionPanel* GetInspectionPanel( void ) { return m_pInspectPanel; }

	void UpdateLabelFromClass( int nClass );
	void UpdateLabelFromSubButton( int nButton );
	virtual void OnTick( void );
	void	RecalculateTargetClassLayout( void );
	void	RecalculateTargetClassLayoutAtPos( int x, int y );
	void MoveCharacterSelection( int nDirection );
	void	OnKeyCodeTyped( vgui::KeyCode code );
	void	OnKeyCodePressed( vgui::KeyCode code );

	bool		 ShouldShowExplanations( void ) { return (m_iShowingPanel == CHAP_LOADOUT && m_iCurrentClassIndex == TF_CLASS_UNDEFINED); }

	charinfo_activepanels_t	GetShowingPanel() const { return m_iShowingPanel; }
	int GetCurrentClassIndex() const	{ return m_iCurrentClassIndex; }

	MESSAGE_FUNC( OnPageShow, "PageShow" );
	MESSAGE_FUNC( OnSelectionStarted, "SelectionStarted" );
	MESSAGE_FUNC( OnSelectionEnded, "SelectionEnded" );
	MESSAGE_FUNC( OnCancelSelection, "CancelSelection" );
	MESSAGE_FUNC( OnOpenCrafting, "OpenCrafting" );
	MESSAGE_FUNC( OnCraftingClosed, "CraftingClosed" );
	MESSAGE_FUNC( OnArmoryClosed, "ArmoryClosed" );
	MESSAGE_FUNC( OnCharInfoClosing, "CharInfoClosing" );

private:
	void		RequestInventoryRefresh();

	CImageButton		*m_pClassButtons[NUM_CLASSES_IN_LOADOUT_PANEL+1];
	CImageButton		*m_pSubButtons[CHSB_NUM_BUTTONS];
	CExLabel			*m_pButtonLabels[CHSB_NUM_BUTTONS];
	int					m_iOverSubButton;
	int					m_iClassLayout[NUM_CLASSES_IN_LOADOUT_PANEL+1][4];
	bool				m_bClassLayoutDirty;
	bool				m_bSnapClassLayout;
	bool				m_bRequestingInventoryRefresh;
	int					m_iCurrentClassIndex;
	int					m_iCurrentTeamIndex;
	charinfo_activepanels_t	m_iShowingPanel;
	charinfo_activepanels_t	m_iPrevShowingPanel;
	CClassLoadoutPanel	*m_pClassLoadoutPanel;
	CBackpackPanel		*m_pBackpackPanel;
	CCraftingPanel		*m_pCraftingPanel;
	CArmoryPanel		*m_pArmoryPanel;
	CTFItemInspectionPanel *m_pInspectPanel;
	vgui::Label			*m_pSelectLabel;
	vgui::Label			*m_pLoadoutChangesLabel;
	vgui::Label			*m_pNoSteamLabel;
	vgui::Label			*m_pNoGCLabel;
	vgui::Label			*m_pClassLabel;
	CExLabel			*m_pItemsLabel;
	int					m_iMouseXPos;
	int					m_iMouseYPos;
	int					m_iLabelSetToClass;
	int					m_iClassLabelYPos;
	int					m_iItemLabelYPos;
	Color				m_ItemColorNone;
	Color				m_ItemColor;
	float				m_flStartExplanationsAt;
	int					m_iArmoryItemDef;

	CPanelAnimationVarAliasType( int, m_iSelectLabelY, "selectlabely_default", "0", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iSelectLabelOnChangesY, "selectlabely_onchanges", "0", "proportional_int" );

	CPanelAnimationVarAliasType( int, m_iClassYPos,	"class_ypos", "0", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iClassXDelta,  "class_xdelta", "0", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iClassWideMin, "class_wide_min", "0", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iClassWideMax, "class_wide_max", "0", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iClassTallMin, "class_tall_min", "0", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iClassTallMax, "class_tall_max", "0", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iClassDistanceMin,	"class_distance_min", "0", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iClassDistanceMax,	"class_distance_max", "0", "proportional_int" );
};

#endif // CHARINFO_LOADOUT_SUBPANEL_H
