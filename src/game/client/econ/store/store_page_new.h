//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef STORE_PAGE_NEW_H
#define STORE_PAGE_NEW_H
#ifdef _WIN32
#pragma once
#endif

#include <game/client/iviewport.h>
#include "vgui_controls/PropertyPage.h"
#include <vgui_controls/Button.h>
#include <vgui_controls/ComboBox.h>
#include <vgui_controls/ImagePanel.h>
#include "econ_controls.h"
#include "econ_store.h"
#include "item_model_panel.h"
#include "store_page.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CStorePricePanel_New : public CStorePricePanel
{
public:
	DECLARE_CLASS_SIMPLE( CStorePricePanel_New, CStorePricePanel );

	CStorePricePanel_New( vgui::Panel *pParent, const char *pPanelName );

	virtual const char	*GetPanelResFile()
	{
		return "Resource/UI/econ/store/v1/StorePrice_New.res";
	}

	virtual void SetItem( const econ_store_entry_t *pEntry );
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CStorePricePanel_Bundles : public CStorePricePanel
{
public:
	DECLARE_CLASS_SIMPLE( CStorePricePanel_Bundles, CStorePricePanel );

	CStorePricePanel_Bundles( vgui::Panel *pParent, const char *pPanelName );

	virtual const char	*GetPanelResFile()
	{
		return "Resource/UI/econ/store/v1/StorePrice_Bundles.res";
	}

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void PerformLayout();

	virtual void SetItem( const econ_store_entry_t *pEntry );

private:
	vgui::ImagePanel	*m_pLimitedLarge;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CStorePricePanel_Jumbo : public CStorePricePanel
{
public:
	DECLARE_CLASS_SIMPLE( CStorePricePanel_Jumbo, CStorePricePanel );

	CStorePricePanel_Jumbo( vgui::Panel *pParent, const char *pPanelName );

	virtual const char	*GetPanelResFile()
	{
		return "Resource/UI/econ/store/v1/StorePrice_Jumbo.res";
	}
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CStorePricePanel_Popular : public CStorePricePanel
{
public:
	DECLARE_CLASS_SIMPLE( CStorePricePanel_Popular, CStorePricePanel );

	CStorePricePanel_Popular( vgui::Panel *pParent, const char *pPanelName, int iPopularityRank );

	virtual const char	*GetPanelResFile()
	{
		return "Resource/UI/econ/store/v1/StorePrice_Popular.res";
	}

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void PerformLayout();

	virtual void SetItem( const econ_store_entry_t *pEntry );

private:
	int m_iPopularityRank;
	CExLabel *m_pNewLarge;
};

#endif // STORE_PAGE_NEW_H
