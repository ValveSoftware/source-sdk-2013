//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef STORE_PAGE_H
#define STORE_PAGE_H
#ifdef _WIN32
#pragma once
#endif

#include <game/client/iviewport.h>
#include "vgui_controls/PropertyPage.h"
#include <vgui_controls/Button.h>
#include <vgui_controls/ComboBox.h>
#include <vgui_controls/ImagePanel.h>
#include "econ_controls.h"
#include "econ_ui.h"
#include "econ_store.h"
#include "item_model_panel.h"
#include "econ_storecategory.h"

class CItemModelPanel;
class CItemModelPanelToolTip;
class CStorePreviewItemPanel;
class CStoreItemControlsPanel;

#define FILTER_ALL_ITEMS		0

//-----------------------------------------------------------------------------
// Purpose: Base class for the preview icons in the store's item preview panel
//-----------------------------------------------------------------------------
class CBaseStorePreviewIcon : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CBaseStorePreviewIcon, vgui::EditablePanel );
public:
	CBaseStorePreviewIcon( vgui::Panel *parent, const char *name ) : vgui::EditablePanel(parent,name)
	{
		REGISTER_COLOR_AS_OVERRIDABLE( m_colPanelBG, "panel_bgcolor" );
		REGISTER_COLOR_AS_OVERRIDABLE( m_colPanelBGMouseover, "panel_bgcolor_mouseover" );
		m_bHover = false;
		m_bSelected = false;
	}

	void SetSelected( bool bSelected )
	{
		m_bSelected = bSelected;
		UpdateBgColor();
	}

	void ApplySchemeSettings( vgui::IScheme *pScheme )
	{
		BaseClass::ApplySchemeSettings( pScheme );
		SetBgColor( m_colPanelBG );
	}

	virtual void PerformLayout( void )
	{
		BaseClass::PerformLayout();

		int iWide = GetWide() - (m_iImageIndent * 2);
		int iTall = GetTall() - (m_iImageIndent * 2);
		SetInternalImageBounds( m_iImageIndent, m_iImageIndent, iWide, iTall );
	}

	virtual void OnCursorEntered()
	{
		BaseClass::OnCursorEntered();
		m_bHover = true;
		UpdateBgColor();
	}
	virtual void OnCursorExited()
	{
		BaseClass::OnCursorExited();
		m_bHover = false;
		UpdateBgColor();
	}

	virtual void SetInternalImageBounds( int iX, int iY, int iWide, int iTall ) = 0;

private:
	Color				m_colPanelBG;
	Color				m_colPanelBGMouseover;
	CPanelAnimationVarAliasType( int, m_iImageIndent, "image_indent", "0", "proportional_int" );

	bool	m_bHover;
	bool	m_bSelected;

	void UpdateBgColor()
	{
		if ( m_bHover || m_bSelected )
		{
			SetBgColor( m_colPanelBGMouseover );
		}
		else
		{
			SetBgColor( m_colPanelBG );
		}
	}

};

//-----------------------------------------------------------------------------
// Purpose: An item preview icon in the store's item preview panel
//-----------------------------------------------------------------------------
class CStorePreviewItemIcon : public CBaseStorePreviewIcon
{
	DECLARE_CLASS_SIMPLE( CStorePreviewItemIcon, CBaseStorePreviewIcon );
public:
	CStorePreviewItemIcon( vgui::Panel *parent, const char *name ) : CBaseStorePreviewIcon(parent,name)
	{
		m_pItemPanel = new CItemModelPanel( this, "itempanel" );
		m_pItemPanel->AddActionSignalTarget( this );
		m_pItemPanel->SendPanelEnterExits( true );
		m_pItemPanel->SetActAsButton( true, true );
	}

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme )
	{
		BaseClass::ApplySchemeSettings( pScheme );
		vgui::EditablePanel *pTmp = dynamic_cast<vgui::EditablePanel*>( FindChildByName("bgblockout") );
		if ( pTmp )
		{
			pTmp->SetMouseInputEnabled( false );
		}
	}

	virtual void OnCursorEntered()
	{
		BaseClass::OnCursorEntered();
		PostActionSignal(new KeyValues("ShowItemIconMouseover", "icon", m_iIconIndex));
	}
	virtual void OnCursorExited()
	{
		BaseClass::OnCursorExited();
		PostActionSignal(new KeyValues("HideItemIconMouseover"));
	}
	virtual void OnMouseReleased(vgui::MouseCode code)
	{
		BaseClass::OnMouseReleased(code);
		PostActionSignal(new KeyValues("ItemIconSelected", "icon", m_iIconIndex));
	}

	MESSAGE_FUNC_PTR( OnItemPanelMouseReleased, "ItemPanelMouseReleased", panel );

	virtual void SetInternalImageBounds( int iX, int iY, int iWide, int iTall )
	{
		m_pItemPanel->SetBounds( iX, iY, iWide, iTall );
	}

	MESSAGE_FUNC_PTR( OnItemPanelEntered, "ItemPanelEntered", panel )
	{
		BaseClass::OnCursorEntered();
	}

	MESSAGE_FUNC_PTR( OnItemPanelExited, "ItemPanelExited", panel )
	{
		BaseClass::OnCursorExited();
	}

	void SetItem( int iIconIndex, int iItemDef )
	{
		m_iIconIndex = iIconIndex;

		CEconItemView itemData;
		itemData.Init( iItemDef, AE_UNIQUE, AE_USE_SCRIPT_VALUE, true );
		m_pItemPanel->SetItem( &itemData );
	}

	void SetItem( int iIconIndex, CEconItemView *pItem )
	{
		m_iIconIndex = iIconIndex;
		m_pItemPanel->SetItem( pItem );
	}

	CItemModelPanel *GetItemPanel( void ) { return m_pItemPanel; }

private:
	CItemModelPanel		*m_pItemPanel;
	int					m_iIconIndex;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CStoreItemControlsPanel;
class CStoreItemControlsPanel : public vgui::EditablePanel
{
public:
	DECLARE_CLASS_SIMPLE( CStoreItemControlsPanel, vgui::EditablePanel );

	CStoreItemControlsPanel( vgui::Panel *pParent, const char *pPanelName, CItemModelPanel *pItemModelPanel );
	virtual ~CStoreItemControlsPanel() {}

	void SetMouseHoverHandler( Panel *pHandler );
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );

	const econ_store_entry_t *GetItem() const;
	void SetItem( const econ_store_entry_t *pEntry );
	void SetButtonsVisible( bool bVisible );

	virtual void OnCursorEntered();
	virtual void OnCursorExited();

	void OnItemPanelEntered();
	void OnItemPanelExited();

	virtual void OnThink();
	virtual void OnCommand( const char *command );

	CItemModelPanel *GetItemModelPanel() { return m_pItemModelPanel; }

protected:
	CItemModelPanel *m_pItemModelPanel;
	const econ_store_entry_t *m_pEntry;
	bool m_bButtonsVisible;
	bool m_bItemPanelEntered;
	vgui::DHANDLE< Panel > m_pMouseHoverHandler;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CStorePricePanel : public vgui::EditablePanel
{
public:
	DECLARE_CLASS_SIMPLE( CStorePricePanel, vgui::EditablePanel );

	CStorePricePanel( vgui::Panel *pParent, const char *pPanelName );
	virtual ~CStorePricePanel();

	virtual const char* GetPanelResFile();
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void PerformLayout();

	void SetPriceText( int iPrice, const char *pVariable, const econ_store_entry_t *pEntry );
	virtual void SetItem( const econ_store_entry_t *pEntry );

	MESSAGE_FUNC_PARAMS( OnStoreItemControlsPanelHover, "StoreItemControlsPanelHover", data );

protected:
	bool			m_bOldDiscountVisibility;
	CExLabel		*m_pPrice;
	CExLabel		*m_pDiscount;
	CExLabel		*m_pNew;
	CExLabel		*m_pHighlighted;
	CExLabel		*m_pSale;
	EditablePanel	*m_pSaleBorder;
	CExLabel		*m_pOGPrice;
	Panel			*m_pCrossout;
	Panel			*m_pLimited;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CStorePage : public vgui::PropertyPage, public CGameEventListener
{
	DECLARE_CLASS_SIMPLE( CStorePage, vgui::PropertyPage );
public:
	CStorePage( Panel *parent, const CEconStoreCategoryManager::StoreCategory_t *pPageData, const char *pPreviewItemResFile = NULL );
	virtual ~CStorePage();

	virtual void OnPostCreate();

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void ApplySettings( KeyValues *inResourceData );
	virtual void OnCommand( const char *command );
	virtual void PerformLayout( void );
	virtual void FireGameEvent( IGameEvent *event );
	virtual void OnMouseWheeled( int delta );

	virtual CStorePricePanel* CreatePricePanel( int iIndex );

	void SetBorderForItem( CItemModelPanel *pItemPanel, bool bMouseOver );
	void CalculateItemButtonPos( CItemModelPanel *pItemPanel, int x, int y, int *iXPos, int *iYPos );
	int  AssignItemToPanel( CItemModelPanel *pPanel, int iIndex );
	void PositionItemPanel( CItemModelPanel *pPanel, int iIndex );

	void UpdateModelPanels( void );
	virtual void UpdateSelectionInfoPanel( void );
	void UpdateCart( void );
	void AddSelectionToCart( void );
	void PreviewSelectionItem( void );
	void DoPreviewItem( item_definition_index_t usItemDef );
	const econ_store_entry_t *GetSelectedEntry( void );

	int	GetNumItemPanels( void ) { return m_iItemPanels; }
	int GetNumColumns( void ) { return m_iItemColumns; }
	int GetNumPages( void );
	virtual void ShowPreview( int iClass, const econ_store_entry_t* pEntry );
	void SetDetailsVisible( bool bVisible );

	const char* GetPageName( void ) { return m_pPageData ? m_pPageData->m_pchName : NULL; }

	virtual bool FindAndSelectEntry( const econ_store_entry_t *pEntry );

	CItemModelPanelToolTip *GetItemTooltip( void ) { return m_pMouseOverTooltip; }

	MESSAGE_FUNC( OnPageShow, "PageShow" );
	MESSAGE_FUNC_PTR( OnItemPanelMouseReleased, "ItemPanelMouseReleased", panel );	// Comes from CStoreItemControlsPanel
	MESSAGE_FUNC_PTR( OnItemPanelMouseDoublePressed, "ItemPanelMouseDoublePressed", panel );
	MESSAGE_FUNC_PTR( OnItemPanelEntered, "ItemPanelEntered", panel );
	MESSAGE_FUNC_PTR( OnItemPanelExited, "ItemPanelExited", panel );
	MESSAGE_FUNC_PTR( OnItemAddToCart, "ItemAddToCart", panel );
	MESSAGE_FUNC_PARAMS( OnTextChanged, "TextChanged", data );
	MESSAGE_FUNC_PARAMS( OnPreviewItem, "PreviewItem", data );

	virtual const char	*GetPageResFile();
	virtual CStorePreviewItemPanel *CreatePreviewPanel( void );

protected:
	// Filtering
	virtual bool	DoesEntryFilterPassSecondaryFilter( const econ_store_entry_t *pEntry ) { return true; }	// Allow derived classes to add an additional
	virtual void	UpdateFilteredItems( void );
	virtual int		GetNumPrimaryFilters( void ) { return 1; }		// All Items
	void			SetFilter( int iFilter );
	virtual void	UpdateFilterComboBox( void );
	virtual void	GetFiltersForDef( GameItemDefinition_t *pDef, CUtlVector<int> *pVecFilters ) { pVecFilters->AddToTail( FILTER_ALL_ITEMS ); }

	static int		ItemDisplayOrderSort_UseSortOverride( const econ_store_entry_t *const *ppA, const econ_store_entry_t *const *ppB );
	virtual void	OrderItemsForDisplay( CUtlVector<const econ_store_entry_t *>& vecItems ) const;

protected:
	void	CreateItemPanels( void );
	void	DeSelectAllItemPanels( void );
	void	ToggleSelectItemPanel( CItemModelPanel *pPanel );
	void	SelectItemPanel( CItemModelPanel *pPanel );
	void	UpdateBackpackLabel( void );
	bool	IsHomePage( void ) { return m_pPageData && m_pPageData->m_bIsHome; }

protected:
	const CEconStoreCategoryManager::StoreCategory_t	*m_pPageData;
	CStorePreviewItemPanel		*m_pPreviewPanel;
	const char					*m_pPreviewItemResFile;
	vgui::EditablePanel			*m_pItemDetailsButtonPanel = NULL;
	vgui::EditablePanel			*m_pItemPreviewButtonPanel = NULL;

	// Filtering
	CUtlVector< const econ_store_entry_t* >	m_FilteredEntries;
	vgui::ComboBox				*m_pFilterComboBox;
	int							m_iCurrentFilter;

	// Selection info panel
	int							m_iSelectedItemDef;
	int							m_iOldSelectedItemDef;
	int							m_iSelectDefOnPageShow;
	int							m_iSelectPageOnPageShow;
	CItemModelPanel				*m_pSelectedPanel;
	CItemModelPanel				*m_pFeaturedItemPanel;
	Color						m_colBackpackOrg;

	// Item model panels
	struct item_panel
	{
		CItemModelPanel* m_pItemModelPanel;
		CStorePricePanel* m_pStorePricePanel;
		CStoreItemControlsPanel* m_pItemControlsPanel;
	};
	CUtlVector<item_panel>		m_vecItemPanels;
	CUtlVector<int>				m_EntryIndices; // Easy lookup for which model panel is mapped to which entry index			
	CItemModelPanel				*m_pMouseOverItemPanel;
	CItemModelPanelToolTip		*m_pMouseOverTooltip;
	KeyValues					*m_pItemModelPanelKVs;
	KeyValues					*m_pModelPanelLabelsKVs;
	bool						m_bReapplyItemKVs;

	// Cart display
	CExButton					*m_pCartButton;
	CUtlVector<CItemModelPanel*> m_pCartModelPanels;
	KeyValues					*m_pCartModelPanelKVs;
	CUtlVector<CExLabel*>		 m_pCartQuantityLabels;
	KeyValues					*m_pCartQuantityLabelKVs;
	vgui::ImagePanel			*m_pCartFeaturedItemImage;

	// Pages
	int							m_iCurrentPage;
	vgui::Label					*m_pCurPageLabel;
	CExButton					*m_pNextPageButton;
	CExButton					*m_pPrevPageButton;

	CExButton					*m_pCheckoutButton;
	CExButton					*m_pPreviewItemButton;
	vgui::EditablePanel			*m_pAddToCartButtonPanel;
	vgui::Label					*m_pBackpackLabel;
	vgui::EditablePanel			*m_pItemBackdropPanel;

	bool						m_bShouldDeletePreviewPanel;	// Set to true by derived classes if the preview panel's panel should be deleted, which is necessary if its parent is NULL

	CPanelAnimationVarAliasType( int, m_iItemOffcenterX, "item_offcenter_x", "0", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iItemXDelta, "item_xdelta", "0", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iItemYDelta, "item_ydelta", "0", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iItemXPos, "item_xpos", "0", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iItemYPos, "item_ypos", "0", "proportional_int" );
	CPanelAnimationVar( int, m_iItemPanels, "item_panels", "35" );
	CPanelAnimationVar( int, m_iItemColumns, "item_columns", "7" );
	CPanelAnimationVar( bool, m_bShowItemBgPanel, "show_item_backdrop", "0" );
	CPanelAnimationVarAliasType( int, m_iItemBackdropLeftMargin, "item_backdrop_left_margin", "20", "proportional_xpos" );
	CPanelAnimationVarAliasType( int, m_iItemBackdropRightMargin, "item_backdrop_right_margin", "20", "proportional_xpos" );
	CPanelAnimationVarAliasType( int, m_iItemBackdropTopMargin, "item_backdrop_top_margin", "20", "proportional_ypos" );
	CPanelAnimationVarAliasType( int, m_iItemBackdropBottomMargin, "item_backdrop_bottom_margin", "20", "proportional_ypos" );
	CPanelAnimationVar( int, m_iItemBackdropPaintBackgroundType, "item_backdrop_paintbackgroundtype", "50" );
	CPanelAnimationVar( int, m_iItemBackdropZPos, "item_backdrop_zpos", "0" );
	CPanelAnimationVarAliasType( int, m_iItemControlsXOffset, "item_controls_xoffset", "5", "proportional_xpos" );
	CPanelAnimationVarAliasType( int, m_iItemControlsYOffset, "item_controls_yoffset", "5", "proportional_xpos" );
	CPanelAnimationVar( int, m_iMaxCartModelPanels, "max_cart_model_panels", "10" );

	Color						m_colItemPanelBG;
	Color						m_colItemPanelBGMouseover;
	Color						m_colItemPanelBGSelected;
	Color						m_colItemBackdropPanel;

	// The number of items in each filter options
	CUtlVector<int>				m_vecFilterCounts;

	bool						m_bFilterDirty;
};

void AddItemToCartHelper( const char *pszContext, const econ_store_entry_t *pEntry, ECartItemType eSelectedCartItemType );
void AddItemToCartHelper( const char *pszContext, item_definition_index_t unItemDef, ECartItemType eSelectedCartItemType );

#endif // STORE_PAGE_H
