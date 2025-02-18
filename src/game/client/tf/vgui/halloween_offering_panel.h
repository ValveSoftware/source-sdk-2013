//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef HALLOWEEN_OFFERING_PANEL_H
#define HALLOWEEN_OFFERING_PANEL_H
#ifdef _WIN32
#pragma once
#endif

#include "collection_crafting_panel.h"


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CHalloweenOfferingSelectionPanel : public CCollectionCraftingSelectionPanel
{
	DECLARE_CLASS_SIMPLE( CHalloweenOfferingSelectionPanel, CCollectionCraftingSelectionPanel );
public:
	CHalloweenOfferingSelectionPanel( Panel *pParent ) : BaseClass( pParent ) {}

	//-----------------------------------------------------------------------------
	virtual const char *GetSelectionInvalidReason( const IEconItemInterface *pTestItem, const IEconItemInterface *pSourceItem ) const
	{
		return GetHalloweenOfferingInvalidReason( pTestItem, pSourceItem );
	}
};


//-----------------------------------------------------------------------------
// A panel to let users choose 10 weapons to craft up within collections
//-----------------------------------------------------------------------------
class CHalloweenOfferingPanel : public CCollectionCraftingPanel
{
public:
	DECLARE_CLASS_SIMPLE( CHalloweenOfferingPanel, CCollectionCraftingPanel );
	CHalloweenOfferingPanel( vgui::Panel *parent, CItemModelPanelToolTip* pTooltip );
	~CHalloweenOfferingPanel( void );

	virtual const char *GetResFile( void ) { return "Resource/UI/econ/HalloweenOfferingDialog.res"; }
	virtual void OnCommand( const char *command ) OVERRIDE;

	virtual int GetInputItemCount() { return HALLOWEEN_OFFERING_ITEM_COUNT; }

protected:

	virtual void CreateSelectionPanel();
};

#endif // HALLOWEEN_OFFERING_PANEL_H
