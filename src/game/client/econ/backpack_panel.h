//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef BACKPACK_PANEL_H
#define BACKPACK_PANEL_H
#ifdef _WIN32
#pragma once
#endif

#include "base_loadout_panel.h"
#include "tf_item_inspection_panel.h"

#define BACKPACK_SLOTS_PER_PAGE		50
#define BACKPACK_ROWS				5
#define BACKPACK_COLUMNS			(BACKPACK_SLOTS_PER_PAGE / BACKPACK_ROWS)
#define BACKPACK_MAX_PAGES			(MAX_NUM_BACKPACK_SLOTS / BACKPACK_SLOTS_PER_PAGE)

class CDynamicRecipePanel;
class CItemSlotPanel;
class CStrangeCountTransferPanel;
class CCollectionCraftingPanel;
class CHalloweenOfferingPanel;
class CCraftCommonStatClockPanel;
class CTFStorePreviewItemPanel2;

//-----------------------------------------------------------------------------
// An inventory screen that handles displaying the backpack
//-----------------------------------------------------------------------------
class CBackpackPanel : public CBaseLoadoutPanel
{
	DECLARE_CLASS_SIMPLE( CBackpackPanel, CBaseLoadoutPanel );
public:
	CBackpackPanel( vgui::Panel *parent, const char *panelName );
	virtual ~CBackpackPanel();

	virtual const char *GetResFile( void ) { return "Resource/UI/econ/BackpackPanel.res"; }
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme ) OVERRIDE;
	virtual void ApplySettings(  KeyValues *inResourceData ) OVERRIDE;
	virtual void PerformLayout( void ) OVERRIDE;
	virtual void FireGameEvent( IGameEvent *event ) OVERRIDE;

	virtual void UpdateModelPanels( void );
	virtual int	 GetNumItemPanels( void ) { return BACKPACK_SLOTS_PER_PAGE; };
	virtual void OnShowPanel( bool bVisible, bool bReturningFromArmory );
	virtual void PostShowPanel( bool bVisible );
	virtual bool UsesRarityControls( void ) { return true; }
	virtual bool AllowSelection( void ) { return true; }
	virtual bool AllowDragging( CItemModelPanel *panel ) { return true; }

	virtual int	 GetNumSlotsPerPage( void ) OVERRIDE { return BACKPACK_SLOTS_PER_PAGE; }
	virtual int	 GetNumColumns( void )  OVERRIDE { return BACKPACK_COLUMNS; }
	virtual int	 GetNumRows( void ) OVERRIDE { return BACKPACK_ROWS; }
	virtual int	 GetNumPages( void ) OVERRIDE;
	virtual void SetCurrentPage( int nNewPage ) OVERRIDE;

	virtual void AssignItemToPanel( CItemModelPanel *pPanel, int iIndex );

	virtual void OnItemPanelEntered( vgui::Panel *panel ) OVERRIDE;
	virtual void OpenContextMenu();
	MESSAGE_FUNC_PTR( OnItemPanelMousePressed, "ItemPanelMousePressed", panel );
	MESSAGE_FUNC_PTR( OnItemPanelMouseReleased, "ItemPanelMouseReleased", panel );
	MESSAGE_FUNC_PTR( OnItemPanelMouseRightRelease, "ItemPanelMouseRightRelease", panel );
	MESSAGE_FUNC_INT_INT( OnCursorMoved, "OnCursorMoved", x, y );
	MESSAGE_FUNC_INT_INT( OnItemPanelCursorMoved, "ItemPanelCursorMoved", x, y );
	MESSAGE_FUNC_PARAMS( OnConfirmDelete, "ConfirmDlgResult", data );
	MESSAGE_FUNC_PARAMS( OnTextChanged, "TextChanged", data );
	MESSAGE_FUNC_PARAMS( OnButtonChecked, "CheckButtonChecked", pData );
	MESSAGE_FUNC( OnCancelSelection, "CancelSelection" );
	MESSAGE_FUNC( DoTradeToPlayer, "DoTradeToPlayer" );
	MESSAGE_FUNC( DoSellMarketplace, "DoSellMarketplace" );
	MESSAGE_FUNC( DoDescription, "DoDescription" );
	MESSAGE_FUNC( DoRename, "DoRename" );
	MESSAGE_FUNC( DoDelete, "DoDelete" );
	MESSAGE_FUNC( DoApplyOnItem, "Context_ApplyOnItem" );
	MESSAGE_FUNC( DoUseConsumableItem, "Context_UseConsumableItem" );
	MESSAGE_FUNC( DoUnwrapItem, "Context_UnwrapItem" );
	MESSAGE_FUNC( DoDeliverItem, "Context_DeliverItem" );
	MESSAGE_FUNC( DoApplyByItem, "Context_ApplyByItem" );
	MESSAGE_FUNC( DoShuffle, "Context_Shuffle" );
	MESSAGE_FUNC( DoEditSlot, "Context_EditSlot" );
	MESSAGE_FUNC( DoRefurbishItem, "Context_RefurbishItem" );
	MESSAGE_FUNC( DoGetItemFromStore, "Context_GetItemFromStore" );
	MESSAGE_FUNC( DoOpenDuckLeaderboards, "Context_OpenDuckLeaderboards" );
	MESSAGE_FUNC( DoInspectModel, "Context_InspectModel" );
	MESSAGE_FUNC( DoPreviewPaintkitsOnItem, "Context_PreviewPaintkitsOnItem" );
	MESSAGE_FUNC( DoPreviewItemsWithPaintkit, "Context_PreviewItemsWithPaintkit" );
	MESSAGE_FUNC( DoBuyKeyAndOpenCrate, "Context_BuyKeyAndOpenCrate" );
	MESSAGE_FUNC( DoOpenCrateWithKey, "Context_OpenCrateWithKey" );
	MESSAGE_FUNC( DoStrangeCountTransfer, "Context_OpenStrangeCountTransfer" );
	MESSAGE_FUNC( DoCraftUpCollection, "Context_CraftUpCollection" );
	MESSAGE_FUNC( DoHalloweenOffering, "Context_HalloweenOffering" );
	MESSAGE_FUNC( DoCraftCommonStatClock, "Context_CraftCommonStatClock" );
	MESSAGE_FUNC( DoOpenConTracker, "Context_OpenConTracker" );
	void DoEquipForClass( int nClass );
	void DoPaint( int nPaintItemIndex, bool bUseStore, bool bUseMarket );
	void DoStrangePart( int nStrangePartIndex, bool bUseMarket );
	enum ESelection
	{
		SELECT_FIRST,
		SELECT_ALL
	};
	bool AttemptToUseItem( item_definition_index_t iItemDefIndex );
	void AttemptToShowItemInStore( item_definition_index_t iItemDefIndex );
	void AttemptToShowItemInMarket( item_definition_index_t iItemDefIndex );
	void GetSelectedPanels( ESelection eSelection, CUtlVector< CItemModelPanel* >& m_vecSelected ) const;
	virtual void OnCommand( const char *command );
	virtual void OnTick( void );
	virtual void OnThink( void );
	virtual void OnKeyCodePressed( vgui::KeyCode code ) OVERRIDE;
	virtual void OnKeyCodeReleased( vgui::KeyCode code ) OVERRIDE;
	virtual void OnKeyCodeTyped(vgui::KeyCode code) OVERRIDE;

	virtual void OnMouseReleased(vgui::MouseCode code) OVERRIDE;
	virtual void OnMouseMismatchedRelease( vgui::MouseCode code, Panel* pPressedPanel ) OVERRIDE;
	virtual void OnMouseCaptureLost() OVERRIDE;

	void OnItemContentsChanged( CEconItemView *pEconItemView );

	virtual void OpenArmory( CEconItemView* item );

	void		 ToggleSelectBackpackItemPanel( CItemModelPanel *pPanel );
	void		 DeSelectAllBackpackItemPanels( void );

	CEconItemView*	GetComboBoxOverlayUISeletionItem() { return &m_ComboBoxOverlaySelectionItem; }
	void			SetComboBoxOverlaySelectionItem( const CEconItemView *pEconItemView ) { m_ComboBoxOverlaySelectionItem = *pEconItemView; }

	void		SetCurrentTransactionID( uint64 nTxnID );
	void		CheckForQuickOpenKey();

	void		MarkItemIDDirty( itemid_t itemID );

	void		OpenInspectModelPanelAndCopyItem( CEconItemView *pItemView );
	CCollectionCraftingPanel	*GetCollectionCraftPanel();

protected:
	virtual void StartDrag( int x, int y );
	virtual void StopDrag( bool bSucceeded );
	virtual bool CanDragTo( CItemModelPanel *pItemPanel, int iPanelIndex ) { return true; }
	virtual void HandleDragTo( CItemModelPanel *pItemPanel, int iPanelIndex );
	virtual int GetBackpackPosForPanelIndex( int iPanelIndex ) { return iPanelIndex + 1 + (GetCurrentPage() * GetNumSlotsPerPage()); }
	virtual bool NeedsDerivedTickSignal( void ) { return false; }
	
	int			GetBackpackPositionForPanel( CItemModelPanel *pItemPanel );
	virtual const char *GetGreyOutItemPanelReason( CItemModelPanel *pItemPanel );
	virtual void	SetBorderForItem( CItemModelPanel *pItemPanel, bool bMouseOver );
	virtual bool	IsIgnoringItemPanelEnters( void ) { return m_bDragging; }
	virtual void	AddNewItemPanel( int iPanelIndex );
	virtual CItemModelPanel *GetItemPanelAtPos( int x, int y );
	virtual void	PositionItemPanel( CItemModelPanel *pPanel, int iIndex );

	void		CancelToolSelection( void );
	void		SetShowBaseItems( bool bShow );

	virtual ConVar		*GetExplanationConVar( void );
	bool				ShouldShowExplanations( void ) { return (!m_bItemsOnly && !InToolSelectionMode()); }

	bool 		InToolSelectionMode() const { return m_eSelectionMode != StandardSelection; }
	void		SetupToolSelectionItem();
	void		HandleToolItemSelection( CEconItemView *pItem );

	void		ClearNameFilter( bool bUpdateModelPanels );
	bool		HasNameFilter() const { return m_wNameFilter.Count() > 0; }
	const wchar_t* GetNameFilter() const { return HasNameFilter() ? m_wNameFilter.Base() : NULL; }
	void		UpdateFilteringItems();

	int			GetItemQualityForBorder( CItemModelPanel* pItemPanel ) const;

	int			GetNumMaxPages() const { return BACKPACK_MAX_PAGES; }
	int			GetPageButtonIndexAtPos( int x, int y );
	void		SetPageButtonTextColorBasedOnContents();

	void		AddPaintToContextMenu( Menu *pPaintSubMenu, item_definition_index_t iPaintDef, bool bAddCommerce );
	void		AddCommerceToContextMenu( Menu *pMenu, const char* pszActionFmt, item_definition_index_t iItemDefIndex, bool bAddMarket, bool bAddStore );
	void		AddCommerceSubmenus( Menu *pSubMenu, item_definition_index_t iItemDef, const char* pszActionFmt );
	void		DoGiftToPlayer( );

protected:
	vgui::TextEntry		*m_pNameFilterTextEntry;
	CUtlVector<wchar_t>	m_wNameFilter;
	float				m_flFilterItemTime;
	CUtlMap< int, CEconItemView*, int > m_mapFilteringItems;
	CUtlMap< itemid_t, char > m_mapSeenItems;
	bool				m_bInitializedSeenItems;
	CUtlVector< itemid_t > m_vecDirtyItems;

	CExButton			*m_pNextPageButton;
	CExButton			*m_pPrevPageButton;
	CExButton			*m_pShowExplanationsButton;
	vgui::Label			*m_pCurPageLabel;
	vgui::ComboBox		*m_pSortByComboBox;
	vgui::ComboBox		*m_pShowRarityComboBox;
	vgui::CheckButton	*m_pShowBaseItemsCheckbox;
	CExButton			*m_pDragToNextPageButton;
	CExButton			*m_pDragToPrevPageButton;
	float				m_flPreventDragPageSwitchUntil;
	float				m_flStartExplanationsAt;

	// Dragging support
	float				m_flMouseDownTime;
	int					m_iMouseDownX;
	int					m_iMouseDownY;
	CItemModelPanel		*m_pItemDraggedFromPanel;
	int					m_iDraggedFromPage;
	bool				m_bMouseDownOnItemPanel;
	bool				m_bDragging;
	CItemModelPanel		*m_pMouseDragItemPanel;
	int					m_iDragOffsetX;
	int					m_iDragOffsetY;
	CItemModelPanel		*m_pPrevDragOverItemPanel;

	// Deletion
	vgui::EditablePanel *m_pConfirmDeleteDialog;

	// Tool support
	enum SelectionMode_t
	{
		StandardSelection,
		ToolSelection,
	};
	SelectionMode_t		m_eSelectionMode;
	int					m_nLastToolPage;
	CEconItemView		m_ToolSelectionItem;
	CExButton			*m_pCancelToolButton;
	vgui::ScalableImagePanel *m_pToolIcon;

	CEconItemView		m_ComboBoxOverlaySelectionItem;

	CExButton			*m_pCraftButton;

	// base items or backpack items
	bool				m_bShowBaseItems;

	// positions of all our item panels, so we can handle drag & drop
	struct backpackitempos_t
	{
		int x,y;
	};
	CUtlVector<backpackitempos_t>	m_ItemModelPanelPos;

	KeyValues			*m_pPageButtonKVs;
	int					m_nNumActivePages;
	CUtlVector< EditablePanel* >	m_Pages;
	CUtlVector<backpackitempos_t>	m_PageButtonPos;

	CDynamicRecipePanel* m_pDynamicRecipePanel;
	CItemSlotPanel* m_pItemSlotPanel;
	CUtlVector< item_definition_index_t > m_vecPaintCans;
	CUtlVector< item_definition_index_t > m_vecStrangeParts;

	DHANDLE<CStrangeCountTransferPanel> m_pStrangeToolPanel;
	DHANDLE<CCollectionCraftingPanel> m_pCollectionCraftPanel;
	DHANDLE<CHalloweenOfferingPanel> m_pHalloweenOfferingPanel;
	DHANDLE<CCraftCommonStatClockPanel> m_pMannCoTradePanel;			// Make this Panel Generic
	CTFStorePreviewItemPanel2 *m_pInspectCosmeticPanel;
	vgui::Menu *m_pContextMenu;
	CEconItemViewHandle m_hQuickOpenCrate;
	uint64 m_nQuickOpenTxn;

	CPanelAnimationVarAliasType( int, m_iPageButtonYPos, "page_button_y", "0", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iPageButtonXDelta, "page_button_x_delta", "0", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iPageButtonYDelta, "page_button_y_delta", "0", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iPageButtonPerRow, "page_button_per_row", "20", "int" );
	CPanelAnimationVarAliasType( int, m_iPageButtonHeight, "page_button_height", "0", "proportional_int" );
};

#endif // BACKPACK_PANEL_H
