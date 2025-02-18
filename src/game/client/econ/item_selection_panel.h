//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef ITEM_SELECTION_PANEL_H
#define ITEM_SELECTION_PANEL_H
#ifdef _WIN32
#pragma once
#endif

#include "vgui_controls/EditablePanel.h"
#include "econ_controls.h"
#include "vgui_controls/ScrollableEditablePanel.h"
#include "backpack_panel.h"
#include "base_loadout_panel.h"

class CItemModelPanel;

#define SELECTION_DISPLAY_SLOTS_PER_PAGE	18
#define SELECTION_DISPLAY_ROWS				3
#define SELECTION_DISPLAY_COLUMNS			(SELECTION_DISPLAY_SLOTS_PER_PAGE / SELECTION_DISPLAY_ROWS)

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
struct item_stack_type_t
{
	item_stack_type_t() : m_nDefIndex( INVALID_ITEM_DEF_INDEX ), m_nQuality( (uint8)-1 ) { }
	item_stack_type_t( item_definition_index_t nDefIndex, uint8 nQuality ) : m_nDefIndex( nDefIndex ), m_nQuality( nQuality ) { }

	bool operator<( const item_stack_type_t& other ) const { return m_nDefIndex < other.m_nDefIndex || m_nQuality < other.m_nQuality; }
	bool operator==( const item_stack_type_t& other ) const { return m_nDefIndex == other.m_nDefIndex && m_nQuality == other.m_nQuality; }

	item_definition_index_t m_nDefIndex;
	uint8 m_nQuality;
};

class CItemSelectionPanel : public CBaseLoadoutPanel
{
	DECLARE_CLASS_SIMPLE( CItemSelectionPanel, CBaseLoadoutPanel );
public:
	CItemSelectionPanel(Panel *parent);
	virtual ~CItemSelectionPanel();

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void ApplySettings( KeyValues *inResourceData );
	virtual void PerformLayout( void );
	virtual void OnThink( void );
	virtual void OnCommand( const char *command );
	virtual void OnClose( void );
	virtual void SetVisible( bool bState );
	virtual bool ShouldDeleteOnClose( void ) { return true; }
	virtual void OnKeyCodePressed( vgui::KeyCode code ) OVERRIDE;
	virtual void OnKeyCodeReleased( vgui::KeyCode code ) OVERRIDE;
	virtual void OnKeyCodeTyped( vgui::KeyCode code) OVERRIDE;
	MESSAGE_FUNC_PARAMS( OnButtonChecked, "CheckButtonChecked", pData );

	virtual int	 GetNumItemPanels( void ) { return m_bShowingEntireBackpack ? BACKPACK_SLOTS_PER_PAGE : SELECTION_DISPLAY_SLOTS_PER_PAGE; };
	virtual void PositionItemPanel( CItemModelPanel *pPanel, int iIndex );
	virtual bool AllowSelection( void ) { return true; }
	virtual bool AllowDragging( CItemModelPanel *panel ) { return false; }

	virtual int	 GetNumSlotsPerPage( void ) OVERRIDE { return m_bShowingEntireBackpack ? BACKPACK_SLOTS_PER_PAGE : SELECTION_DISPLAY_SLOTS_PER_PAGE; }
	virtual int	 GetNumColumns( void ) OVERRIDE { return m_bShowingEntireBackpack ? BACKPACK_COLUMNS : SELECTION_DISPLAY_COLUMNS; }
	virtual int	 GetNumRows( void ) OVERRIDE { return m_bShowingEntireBackpack ? BACKPACK_ROWS : SELECTION_DISPLAY_ROWS; }
	virtual int	 GetNumPages( void ) OVERRIDE;
	virtual void SetCurrentPage( int nNewPage ) OVERRIDE;

	void		 UpdateModelPanels( void );
	virtual void ApplyKVsToItemPanels( void );
	virtual void CreateItemPanels( void );

	void		 ShowDuplicateCounts( bool bShow ) { m_bShowDuplicates = bShow; }
	void		 UpdateDuplicateCounts( void );

	MESSAGE_FUNC_PTR( OnItemPanelMousePressed, "ItemPanelMousePressed", panel );
	MESSAGE_FUNC_PTR( OnItemPanelMouseReleased, "ItemPanelMouseReleased", panel );
	MESSAGE_FUNC_PARAMS( OnTextChanged, "TextChanged", data );

	// Derived panels need to override these with the custom selection behavior
	virtual const char *GetSchemeFile( void ) = 0;
	virtual bool ShouldItemPanelBeVisible( CItemModelPanel *pPanel, int iPanelIndex ) = 0;
	virtual void UpdateModelPanelsForSelection( void ) = 0;
	virtual const char *GetItemNotSelectableReason( const CEconItemView *pItem ) const = 0;

	virtual bool DisableItemSelectionFromGrayedOutPanels( void ) const { return false; }
	void NotifySelectionReturned( CItemModelPanel *pItemPanel );
	void SetCaller( Panel* pCaller ) { m_pCaller = pCaller; }

	virtual bool	DisplayOnlyAllowUniqueQualityCheckbox() const { return false; }

protected:
	void	PostMessageSelectionReturned( itemid_t ulItemID );

	bool							m_bShowingEntireBackpack;

	KeyValues						*m_pSelectionItemModelPanelKVs;
	KeyValues						*m_pDuplicateLabelKVs;
	vgui::CheckButton				*m_pOnlyAllowUniqueQuality;
	CExButton						*m_pShowBackpack;
	CExButton						*m_pShowSelection;
	bool							m_bForceBackpack;

	CExButton						*m_pNextPageButton;
	CExButton						*m_pPrevPageButton;
	vgui::Label						*m_pCurPageLabel;
	vgui::Label						*m_pNoItemsInSelectionLabel;

	int								m_iItemsInSelection;

	bool							m_bShowDuplicates;
	CUtlVector<CExLabel*>			m_pDuplicateCountLabels;

	typedef CUtlMap< item_stack_type_t, int >	DuplicateCountsMap_t;
	DuplicateCountsMap_t			m_DuplicateCounts;		// A map of item def indices to item counts. Derived classes should fill this out.

	bool							m_bGotMousePressed;

	Panel				*m_pCaller;
	vgui::TextEntry		*m_pNameFilterTextEntry;
	CUtlVector<wchar_t>	m_wNameFilter;
	float				m_flFilterItemTime;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CEquipSlotItemSelectionPanel : public CItemSelectionPanel
{
public:
	DECLARE_CLASS_SIMPLE( CEquipSlotItemSelectionPanel, CItemSelectionPanel );
public:
	CEquipSlotItemSelectionPanel(Panel *parent, int iClass, int iSlot);

	virtual void	ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void	PerformLayout( void );
	
	virtual const char *GetSchemeFile( void ) { return "Resource/UI/ItemSelectionPanel.res"; }
	virtual bool	ShouldItemPanelBeVisible( CItemModelPanel *pPanel, int iPanelIndex );
	virtual void	UpdateModelPanelsForSelection( void );
	virtual const char *GetItemNotSelectableReason( const CEconItemView *pItem ) const;

	virtual bool	DisableItemSelectionFromGrayedOutPanels( void ) const { return true; }

	void			OnBackPressed();

protected:
	int								m_iClass;		// Class of the player we're selecting an item for
	int								m_iSlot;		// Slot on the player that we're selecting an item for

	itemid_t						m_iCurrentItemID;

	vgui::Label						*m_pWeaponLabel;
};

//-----------------------------------------------------------------------------
// Purpose: Selection panel that uses an Item Criteria block to do selection
//-----------------------------------------------------------------------------
class CItemCriteriaSelectionPanel : public CItemSelectionPanel
{
	DECLARE_CLASS_SIMPLE( CItemCriteriaSelectionPanel, CItemSelectionPanel );
public:
	CItemCriteriaSelectionPanel(Panel *parent, const CItemSelectionCriteria *pCriteria, itemid_t pExceptions[] = NULL, int iNumExceptions = 0 );

	virtual void	ApplySchemeSettings( vgui::IScheme *pScheme );
	void			UpdateExceptions( itemid_t pExceptions[], int iNumExceptions );

	virtual const char *GetSchemeFile( void ) { return "Resource/UI/ItemSelectionPanel.res"; }
	virtual bool	ShouldItemPanelBeVisible( CItemModelPanel *pPanel, int iPanelIndex );
	virtual void	UpdateModelPanelsForSelection( void );
	virtual const char *GetItemNotSelectableReason( const CEconItemView *pItem ) const;

protected:
	const CItemSelectionCriteria	*m_pCriteria;
	CUtlVector<itemid_t>			m_Exceptions;
};

//-----------------------------------------------------------------------------
// Purpose: Selection panel for crafting
//-----------------------------------------------------------------------------
class CCraftingItemSelectionPanel : public CItemCriteriaSelectionPanel
{
	DECLARE_CLASS_SIMPLE( CCraftingItemSelectionPanel, CItemCriteriaSelectionPanel );
public:
	CCraftingItemSelectionPanel(Panel *parent );

	virtual const char *GetItemNotSelectableReason( const CEconItemView *pItem ) const;
	virtual bool	ShouldDeleteOnClose( void ) { return false; }

	void			UpdateOnShow( const CItemSelectionCriteria *pCriteria, bool bForceBackpack, itemid_t pExceptions[] = NULL, int iNumExceptions = 0 );

	virtual bool	DisplayOnlyAllowUniqueQualityCheckbox() const { return true; }
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CAccountSlotItemSelectionPanel : public CEquipSlotItemSelectionPanel
{
	DECLARE_CLASS_SIMPLE( CAccountSlotItemSelectionPanel, CEquipSlotItemSelectionPanel );
public:
	CAccountSlotItemSelectionPanel( Panel *pParent, int iSlot, const char *pszTitleToken );

	virtual void	ApplySchemeSettings( vgui::IScheme *pScheme ) OVERRIDE;

	virtual const char *GetItemNotSelectableReason( const CEconItemView *pItem ) const OVERRIDE;

protected:
	const char * m_pszTitleToken;
};

#endif // ITEM_SELECTION_PANEL_H
