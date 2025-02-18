//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#ifndef ITEM_SLOT_PANEL_H
#define ITEM_SLOT_PANEL_H
#include "base_loadout_panel.h"

class CItemCriteriaSelectionPanel;

//-----------------------------------------------------------------------------
// A loadout screen that handles modifying the loadout of a specific item
//-----------------------------------------------------------------------------
class CItemSlotPanel : public CBaseLoadoutPanel
{
	DECLARE_CLASS_SIMPLE( CItemSlotPanel, CBaseLoadoutPanel );
public:
	CItemSlotPanel( vgui::Panel *parent );
	~CItemSlotPanel();

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void PerformLayout( void );

	virtual void AddNewItemPanel( int iPanelIndex ) OVERRIDE;
	virtual void UpdateModelPanels( void ) OVERRIDE;
	virtual int	 GetNumItemPanels( void ) OVERRIDE;
	virtual void OnShowPanel( bool bVisible, bool bReturningFromArmory ) OVERRIDE;

	MESSAGE_FUNC_PTR( OnItemPanelMouseReleased, "ItemPanelMouseReleased", panel );
	MESSAGE_FUNC_PARAMS( OnSelectionReturned, "SelectionReturned", data );
	MESSAGE_FUNC( OnCancelSelection, "CancelSelection" );
	virtual void OnCommand( const char *command );

	void	SetItem( CEconItem* pItem );

private:
	CEconItem	*m_pItem;

	struct ItemSlot_t
	{
		CAttribute_ItemSlotCriteria m_slotCriteriaAttribute;
		itemid_t m_ulOriginalID;
		bool m_bHasSlot;
	};
	CUtlVector< ItemSlot_t > m_itemSlots;

	int m_iCurrentSlotIndex;
	CItemSelectionCriteria m_selectionCriteria;
	CItemCriteriaSelectionPanel	*m_pSelectionPanel;
};

#endif // ITEM_SLOT_PANEL_H
