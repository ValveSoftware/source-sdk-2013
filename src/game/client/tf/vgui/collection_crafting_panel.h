//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef COLLECTION_CRAFTING_PANEL_H
#define COLLECTION_CRAFTING_PANEL_H
#ifdef _WIN32
#pragma once
#endif

#include "backpack_panel.h"
#include "vgui_controls/ScrollableEditablePanel.h"
#include "tf_gcmessages.h"
#include "econ_gcmessages.h"
#include "tf_imagepanel.h"
#include "tf_controls.h"
#include "item_selection_panel.h"
#include "drawing_panel.h"
#include "local_steam_shared_object_listener.h"


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CCollectionCraftingSelectionPanel : public CItemCriteriaSelectionPanel
{
	DECLARE_CLASS_SIMPLE( CCollectionCraftingSelectionPanel, CItemCriteriaSelectionPanel );
public:
	CCollectionCraftingSelectionPanel( Panel *pParent ) : BaseClass( pParent, NULL ) {}

	void SetCorrespondingItems( CCopyableUtlVector< const CEconItemView* >& vecSelectedItems )
	{
		m_vecCorrespondingItems = vecSelectedItems;
	}

	void ApplySchemeSettings( vgui::IScheme *pScheme )
	{
		BaseClass::ApplySchemeSettings( pScheme );

		vgui::Label* pWeaponLabel = dynamic_cast<vgui::Label*>( FindChildByName( "ItemSlotLabel" ) );
		if ( pWeaponLabel )
		{
			pWeaponLabel->SetVisible( false );
		}
	}

	//-----------------------------------------------------------------------------
	virtual const char *GetSelectionInvalidReason( const IEconItemInterface *pTestItem, const IEconItemInterface *pSourceItem ) const
	{
		return GetCollectionCraftingInvalidReason( pTestItem, pSourceItem );
	}

	//-----------------------------------------------------------------------------
	// Purpose: 
	//-----------------------------------------------------------------------------
	const char *GetItemNotSelectableReason( const CEconItemView *pItem ) const
	{
		if ( !pItem )
			return NULL;

		const CEconItemView* pSourceItem = m_vecCorrespondingItems.Count() ? m_vecCorrespondingItems[0] : NULL;

		FOR_EACH_VEC( m_vecCorrespondingItems, i )
		{
			if ( pItem->GetItemID() == m_vecCorrespondingItems[i]->GetItemID() )
			{
				return "#TF_StrangeCount_Transfer_Self";
			}
		}

		return GetSelectionInvalidReason( pItem, pSourceItem );
	}

	virtual bool ShouldDeleteOnClose( void ) OVERRIDE{ return false; }

protected:
	const char * m_pszTitleToken;
	CUtlVector< const CEconItemView* > m_vecCorrespondingItems;
};

//-----------------------------------------------------------------------------
// A panel to let users choose 10 weapons to craft up within collections
//-----------------------------------------------------------------------------
class CCollectionCraftingPanel : public vgui::EditablePanel, public CGameEventListener, public CLocalSteamSharedObjectListener
{
public:
	DECLARE_CLASS_SIMPLE( CCollectionCraftingPanel, vgui::EditablePanel );
	CCollectionCraftingPanel( vgui::Panel *parent, CItemModelPanelToolTip* pTooltip );
	~CCollectionCraftingPanel( void );

	virtual const char *GetResFile( void ) { return "Resource/UI/econ/CollectionCraftingDialog.res"; }
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void ApplySettings( KeyValues *inResourceData ) OVERRIDE;
	virtual void PerformLayout() OVERRIDE;
	virtual void FireGameEvent( IGameEvent *event ) OVERRIDE;
	virtual void OnCommand( const char *command ) OVERRIDE;
	virtual void SetVisible( bool bVisible ) OVERRIDE;

	virtual void SOCreated( const CSteamID & steamIDOwner, const GCSDK::CSharedObject *pObject, GCSDK::ESOCacheEvent eEvent ) OVERRIDE;

	virtual void Show( CUtlVector< const CEconItemView* >& vecStartingItems );
	void SetWaitingForItem( eEconItemOrigin eOrigin );

	virtual int GetInputItemCount() { return COLLECTION_CRAFTING_ITEM_COUNT; }
	virtual int GetOutputItemCount() { return 0; }	// For Ui Display Purposes

	MESSAGE_FUNC_PTR( OnItemPanelMousePressed, "ItemPanelMousePressed", panel );
	MESSAGE_FUNC_PARAMS( OnSelectionReturned, "SelectionReturned", data );

protected:

	virtual void SetItemPanelCount( );
	virtual void CreateSelectionPanel();
	virtual void CreateItemPanels();

	void SelectPanel( int nPanel );
	void UpdateOKButton();
	void SetItem( const CEconItemView* pItem, int nIndex );
	virtual void OnThink() OVERRIDE;

	CItemModelPanelToolTip	*m_pMouseOverTooltip;

	DHANDLE<CCollectionCraftingSelectionPanel> m_hSelectionPanel;

	CExButton				*m_pOKButton;
	CExButton				*m_pNextItemButton;

	EditablePanel* m_pTradeUpContainer;
	CItemModelPanel* m_pSelectingItemModelPanel;
	CUtlVector< EditablePanel* > m_vecItemContainers;
	CUtlVector< ImagePanel* > m_vecImagePanels;
	CUtlVector< CItemModelPanel* > m_vecItemPanels;

	CUtlVector< EditablePanel* > m_vecOutputItemContainers;
	CUtlVector< ImagePanel* > m_vecOutputImagePanels;
	CUtlVector< CItemModelPanel* > m_vecOutputItemPanels;

	CUtlVector< CUtlString > m_vecBoxTopNames;
	CUtlVector< CUtlString > m_vecStampNames;
	CUtlVector< CUtlString > m_vecResultStrings;
	struct LocalizedPanelAction_t
	{
		CUtlString m_strPanel;
		bool m_bShowForEnglish;
	};
	CUtlVector< LocalizedPanelAction_t > m_vecLocalizedPanels;
	CBaseModelPanel *m_pModelPanel;
	ImagePanel* m_pStampPanel;
	CExButton* m_pStampButton;

	CDrawingPanel *m_pDrawingPanel;
	CTFItemInspectionPanel *m_pInspectPanel;
	CItemModelPanel* m_pCosmeticResultItemModelPanel;
	CItemModelPanel* m_pItemNamePanel;

	KeyValues* m_pKVItemPanels;
	bool	m_bWaitingForGCResponse;
	RealTimeCountdownTimer m_timerResponse;
	CUtlVector<itemid_t> m_nFoundItemID;
	bool	m_bEnvelopeReadyToSend;
	bool	m_bShowing;
	bool	m_bShowImmediately;

	eEconItemOrigin	m_eEconItemOrigin;

	CPanelAnimationVarAliasType( int, m_iButtonsStartX, "buttons_start_x", "0", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iButtonsStartY, "buttons_start_y", "0", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iButtonsStepX, "buttons_step_x", "0", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iButtonsStepY, "buttons_step_y", "0", "proportional_int" );

	CPanelAnimationVarAliasType( int, m_iOutputItemStartX, "output_start_x", "0", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iOutputItemStartY, "output_start_y", "0", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iOutputItemStepX, "output_step_x", "0", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iOutputItemStepY, "output_step_y", "0", "proportional_int" );

	CPanelAnimationVarAliasType( float, m_flSlideInTime, "slide_in_time", "1.0", "float" );
	CPanelAnimationVarAliasType( int, m_iBGContainerTargetY, "bg_target_y", "0", "proportional_int" );
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CStatClockCraftingSelectionPanel : public CCollectionCraftingSelectionPanel
{
	DECLARE_CLASS_SIMPLE( CStatClockCraftingSelectionPanel, CCollectionCraftingSelectionPanel );
public:
	CStatClockCraftingSelectionPanel( Panel *pParent ) : BaseClass( pParent ) {}

	//-----------------------------------------------------------------------------
	virtual const char *GetSelectionInvalidReason( const IEconItemInterface *pTestItem, const IEconItemInterface *pSourceItem ) const
	{
		return GetCraftCommonStatClockInvalidReason( pTestItem, pSourceItem );	// FIX ME
	}
};


//-----------------------------------------------------------------------------
// A panel to let users choose 10 weapons to craft up within collections
//-----------------------------------------------------------------------------
class CCraftCommonStatClockPanel : public CCollectionCraftingPanel
{
public:
	DECLARE_CLASS_SIMPLE( CCraftCommonStatClockPanel, CCollectionCraftingPanel );
	CCraftCommonStatClockPanel( vgui::Panel *parent, CItemModelPanelToolTip* pTooltip );
	~CCraftCommonStatClockPanel( void );

	virtual const char *GetResFile( void ) { return "Resource/UI/econ/MannCoTrade_CommonStatClock.res"; }
	virtual void OnCommand( const char *command ) OVERRIDE;

	virtual int GetInputItemCount() { return CRAFT_COMMON_STATCLOCK_ITEM_COUNT; }
	virtual int GetOutputItemCount() { return 1; }

	virtual void Show( CUtlVector< const CEconItemView* >& vecStartingItems );

protected:

	virtual void CreateSelectionPanel();

	CEconItemView m_outputItem;
};

#endif // COLLECTION_CRAFTING_PANEL_H
