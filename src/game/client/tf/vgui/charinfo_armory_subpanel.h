//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef CHARINFO_ARMORY_SUBPANEL_H
#define CHARINFO_ARMORY_SUBPANEL_H
#ifdef _WIN32
#pragma once
#endif

#include <game/client/iviewport.h>
#include "vgui_controls/PropertyPage.h"
#include <vgui_controls/Button.h>
#include <vgui_controls/ComboBox.h>
#include "tf_controls.h"
#include "tf_shareddefs.h"
#include "backpack_panel.h"
#include "class_loadout_panel.h"

enum armory_filters_t
{
	// These are listed in the dropdown, for players to select
	ARMFILT_ALL_ITEMS,
	ARMFILT_WEAPONS,
	ARMFILT_MISCITEMS,
	ARMFILT_ACTIONITEMS,
	ARMFILT_CRAFTITEMS,
	ARMFILT_TOOLS,
	ARMFILT_CLASS_ALL,
	ARMFILT_CLASS_SCOUT,
	ARMFILT_CLASS_SNIPER,
	ARMFILT_CLASS_SOLDIER,
	ARMFILT_CLASS_DEMOMAN,
	ARMFILT_CLASS_MEDIC,
	ARMFILT_CLASS_HEAVY,
	ARMFILT_CLASS_PYRO,
	ARMFILT_CLASS_SPY,
	ARMFILT_CLASS_ENGINEER,
	ARMFILT_DONATIONITEMS,

	ARMFILT_NUM_IN_DROPDOWN,

	ARMFILT_CUSTOM,
	ARMFILT_TOTAL,
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CArmoryPanel : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CArmoryPanel, vgui::EditablePanel );
public:
	CArmoryPanel(Panel *parent, const char *panelName);
	virtual ~CArmoryPanel();

	// Show the armory with one of the default filters set
	void		 ShowPanel( int iItemDef, armory_filters_t nFilter = ARMFILT_ALL_ITEMS );

	// Show the armory with a custom list of item definitions, and a custom filter string 
	void		 ShowPanel( const char *pszFilterString, CUtlVector<item_definition_index_t> *vecItems );

	// Select the given item and jump to the appropriate page
	void		JumpToItem( int iItemDef, armory_filters_t nFilter );

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void ApplySettings( KeyValues *inResourceData );
	virtual void OnCommand( const char *command );
	virtual void PerformLayout( void );

	bool		 ShouldShowExplanations( void ) { return true; }
	void		 UpdateItemList( void );
	void		 UpdateSelectedItem( void );
	void		 AllowGotoStore( void ) { m_bAllowGotoStore = true; }

	MESSAGE_FUNC_PTR( OnItemPanelEntered, "ItemPanelEntered", panel );
	MESSAGE_FUNC_PTR( OnItemPanelExited, "ItemPanelExited", panel );
	MESSAGE_FUNC_PTR( OnItemPanelMouseReleased, "ItemPanelMouseReleased", panel );
	MESSAGE_FUNC_PARAMS( OnTextChanged, "TextChanged", data );
	MESSAGE_FUNC_PARAMS( OnItemLinkClicked, "URLClicked", pParams );

	MESSAGE_FUNC( OnClosing, "Closing" );

private:
	void		 OnShowPanel( void );
	void		 MoveItem( int iDelta );
	void		 UpdateDataBlock( void );
	bool		 CalculateDataText( void );

	void		 SetFilterTo( int iItemDef, armory_filters_t nFilter );
	void		 SetBorderForItem( CItemModelPanel *pItemPanel, bool bMouseOver );
	bool		 DefPassesFilter( const CTFItemDefinition *pDef, armory_filters_t iFilter );

	void		 SetupComboBox( const char *pszCustomAddition );

	void		 SetSelectedItem( CEconItemView* newItem );
	void		 SetSelectedItem( int newIndex );

private:
	float							m_flStartExplanationsAt;

	CEconItemView				m_SelectedItem;
	CEconItemView				m_PreviousItem;
	CItemModelPanel					*m_pSelectedItemModelPanel;
	CItemModelPanel					*m_pSelectedItemImageModelPanel;

	// Filters
	vgui::ComboBox					*m_pFilterComboBox;
	armory_filters_t				m_CurrentFilter;
	armory_filters_t				m_OldFilter;
	int								m_iFilterPage;
	CUtlVector<item_definition_index_t>	m_FilteredItemList;
	CUtlVector<item_definition_index_t>	m_CustomFilteredList;

	// Thumbnails
	KeyValues						*m_pThumbnailModelPanelKVs;
	bool							m_bReapplyItemKVs;
	CUtlVector<CItemModelPanel*>	m_pThumbnailModelPanels;
	CItemModelPanel					*m_pMouseOverItemPanel;
	CItemModelPanelToolTip			*m_pMouseOverTooltip;

	bool							m_bEventLogging; // Handles sending entered/exited stats messages.

	// Data display
	vgui::EditablePanel				*m_pDataPanel;
	CEconItemDetailsRichText			*m_pDataTextRichText;
	CExButton						*m_pViewSetButton;

	bool							m_bAllowGotoStore;
	CExButton						*m_pStoreButton;

	CPanelAnimationVar( int, m_iThumbnailRows, "thumbnails_rows", "1" );
	CPanelAnimationVar( int, m_iThumbnailColumns, "thumbnails_columns", "1" );
	CPanelAnimationVarAliasType( int, m_iThumbnailX, "thumbnails_x", "0", "proportional_xpos" );
	CPanelAnimationVarAliasType( int, m_iThumbnailY, "thumbnails_y", "0", "proportional_ypos" );
	CPanelAnimationVarAliasType( int, m_iThumbnailDeltaX, "thumbnails_delta_x", "0", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iThumbnailDeltaY, "thumbnails_delta_y", "0", "proportional_int" );
	Color						m_colThumbnailBG;
	Color						m_colThumbnailBGMouseover;
	Color						m_colThumbnailBGSelected;

	Color						m_colSetName;
};

#endif // CHARINFO_ARMORY_SUBPANEL_H
