//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_STORE_PAGE2_H
#define TF_STORE_PAGE2_H
#ifdef _WIN32
#pragma once
#endif

#include "store/tf_store_page_base.h"

class CNavigationPanel;
class CClassFilterTooltip;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CTFStorePage2 : public CTFStorePageBase
{
	DECLARE_CLASS_SIMPLE( CTFStorePage2, CTFStorePageBase );
public:
	CTFStorePage2( Panel *parent, const CEconStoreCategoryManager::StoreCategory_t *pPageData, const char *pPreviewItemResFile = NULL );
	~CTFStorePage2();

	virtual void OnPostCreate();

	bool HasSubcategories() const;
	int GetNumSubcategories() const	{ return m_pPageData ? m_pPageData->m_vecSubcategories.Count() : 0; }

	virtual void	PerformLayout();
	virtual void	ApplySchemeSettings( vgui::IScheme *pScheme );

	virtual const char *GetPageResFile( void );
	virtual void	OnCommand( const char *command );

	MESSAGE_FUNC( OnPageShow, "PageShow" );
	MESSAGE_FUNC_PTR( OnItemDetails, "ItemDetails", panel );
	MESSAGE_FUNC_PARAMS( OnItemDefDetails, "ItemDefDetails", pData );
	MESSAGE_FUNC_PARAMS( OnTextChanged, "TextChanged", pData );
	MESSAGE_FUNC_PARAMS( OnNavButtonSelected, "NavButtonSelected", pData );
	MESSAGE_FUNC_PARAMS( OnAddItemToCart, "AddItemToCart", data );	// Comes from preview panel
	MESSAGE_FUNC_PTR( OnItemPanelMouseDoublePressed, "ItemPanelMouseDoublePressed", panel );
	MESSAGE_FUNC_PTR( OnItemPanelMouseReleased, "ItemPanelMouseReleased", panel );	// Comes from CStoreItemControlsPanel

	virtual bool	DoesEntryFilterPassSecondaryFilter( const econ_store_entry_t *pEntry );
	bool DoesEntryFilterPassSubcategoryFilter( const econ_store_entry_t *pEntry );

	virtual void	UpdateFilteredItems( void );
	virtual void	UpdateFilterComboBox( void );
	virtual void	GetFiltersForDef( GameItemDefinition_t *pDef, CUtlVector<int> *pVecFilters );
	virtual void	OnThink( void );
	virtual bool	FindAndSelectEntry( const econ_store_entry_t *pEntry );

	void		ClearNameFilter( bool bUpdateModelPanels );

	virtual CStorePreviewItemPanel	*CreatePreviewPanel( void );
	virtual CStorePricePanel* CreatePricePanel( int iIndex );

	void ShowPreviewWindow( item_definition_index_t usDefIndex );
	int GetAllSubcategoriesIndex() const;

	vgui::TextEntry		*m_pNameFilterTextEntry;
	CExLabel			*m_pSubcategoriesFilterLabel;
	vgui::ComboBox		*m_pSubcategoriesFilterCombo;
	vgui::ComboBox		*m_pSortByCombo;
	CNavigationPanel	*m_pHomeCategoryTabs;
	CNavigationPanel	*m_pClassFilterButtons;
	CExLabel			*m_pClassFilterTooltipLabel;
	CClassFilterTooltip	*m_pClassFilterTooltip;

	int					m_iCurrentSubcategory;
	CUtlVector<wchar_t>	m_wNameFilter;
	float				m_flFilterItemTime;

	friend class CClassFilterTooltip;
};

#endif // TF_STORE_PAGE2_H
