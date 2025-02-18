//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_STORE_PAGE_H
#define TF_STORE_PAGE_H
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
#include "store/store_page.h"
#include "tf_shareddefs.h"

class CItemModelPanel;
class CItemModelPanelToolTip;
class CTFPlayerModelPanel;
class CStorePreviewItemPanel;
class CStoreItemControlsPanel;

extern const char *g_pszTipsClassImages[];

#define FILTER_ALLCLASS_ITEMS		TF_LAST_NORMAL_CLASS
#define FILTER_UNOWNED_ITEMS		(TF_LAST_NORMAL_CLASS + 1)

//-----------------------------------------------------------------------------
// Purpose: A player class preview icon in the store's item preview panel
//-----------------------------------------------------------------------------
class CStorePreviewClassIcon : public CBaseStorePreviewIcon
{
	DECLARE_CLASS_SIMPLE( CStorePreviewClassIcon, CBaseStorePreviewIcon );
public:
	CStorePreviewClassIcon( vgui::Panel *parent, const char *name ) : CBaseStorePreviewIcon(parent,name)
	{
		m_pImagePanel = new vgui::ImagePanel( this, "classimage" );
		m_pImagePanel->SetShouldScaleImage( true );
		m_pImagePanel->SetMouseInputEnabled( false );
		m_pImagePanel->SetKeyBoardInputEnabled( false );
		m_iClass = 0;
	}

	virtual void OnCursorEntered()
	{
		BaseClass::OnCursorEntered();
		PostActionSignal(new KeyValues("ShowClassIconMouseover", "class", m_iClass));
	}
	virtual void OnCursorExited()
	{
		BaseClass::OnCursorExited();
		PostActionSignal(new KeyValues("HideClassIconMouseover"));
	}
	virtual void OnMouseReleased(vgui::MouseCode code)
	{
		BaseClass::OnMouseReleased(code);
		PostActionSignal(new KeyValues("ClassIconSelected", "class", m_iClass));
	}

	virtual void SetInternalImageBounds( int iX, int iY, int iWide, int iTall )
	{
		m_pImagePanel->SetBounds( iX, iY, iWide, iTall );
	}

	void SetClass( int iClass )
	{
		if ( iClass >= TF_FIRST_NORMAL_CLASS && iClass < TF_LAST_NORMAL_CLASS )
		{
			m_pImagePanel->SetImage( g_pszTipsClassImages[iClass] );
		}
		else
		{
			m_pImagePanel->SetImage( "class_portraits/all_class" );
		}
		m_iClass = iClass;
	}

	int GetClass( void ) { return m_iClass; }

private:
	vgui::ImagePanel	*m_pImagePanel;
	int					m_iClass;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CTFStorePageBase : public CStorePage
{
	DECLARE_CLASS_SIMPLE( CTFStorePageBase, CStorePage );
protected:
	// CTFStorePageBase should not be instantiated directly
	CTFStorePageBase( Panel *parent, const CEconStoreCategoryManager::StoreCategory_t *pPageData, const char *pPreviewItemResFile = NULL );

public:

	virtual void	OnCommand( const char *command );
	virtual void	ShowPreview( int iClass, const econ_store_entry_t* pEntry );

	MESSAGE_FUNC( OnPageShow, "PageShow" );
	MESSAGE_FUNC_PTR( OnItemDetails, "ItemDetails", panel );

	virtual void	UpdateFilterComboBox( void );
	virtual void	GetFiltersForDef( GameItemDefinition_t *pDef, CUtlVector<int> *pVecFilters );
	virtual void	OnTick( void );
	virtual int		GetNumPrimaryFilters( void ) { return FILTER_UNOWNED_ITEMS+1; }

protected:
	float		m_flStartExplanationsAt;
};

#endif // TF_STORE_PAGE_H
