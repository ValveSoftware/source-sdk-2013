//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_ITEM_INSPECTION_PANEL_H
#define TF_ITEM_INSPECTION_PANEL_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/Panel.h>
#include <vgui_controls/EditablePanel.h>
#include "tf_controls.h"

using namespace vgui;

class CEconItemView;
class CEmbeddedItemModelPanel;
class CNavigationPanel;
class CItemModelPanel;
class CPaintKitDefinition;

class CTFItemInspectionPanel : public EditablePanel
{
	DECLARE_CLASS_SIMPLE( CTFItemInspectionPanel, EditablePanel )
public:

	CTFItemInspectionPanel( Panel* pPanel, const char *pszName );

	virtual void ApplySchemeSettings( IScheme *pScheme ) OVERRIDE;
	virtual void PerformLayout() OVERRIDE;
	virtual void OnCommand( const char *command ) OVERRIDE;
	virtual void OnThink() OVERRIDE;

	void SetItemCopy( CEconItemView *pItem, bool bReset = true );
	void SetOptions( bool bFixedItem, bool bFixedPaintkit, bool bConsumptionMode );
	void Reset();

private:
	// we always want to copy item with SetItemCopy to make sure that we use high res skin
	void SetItem( CEconItemView *pItem, bool bReset );
	void RecompositeItem();

	bool m_bFixedItem = false;
	bool m_bConsumeMode = false;
	float m_flLastManipulatedTime = 0.f;
	float m_flLastThink = 0.f;
	CPanelAnimationVar( float, m_flSpinVel, "spin_vel", "0" );

	MESSAGE_FUNC_PARAMS( OnNavButtonSelected, "NavButtonSelected", pData );
	MESSAGE_FUNC_PTR( OnRadioButtonChecked, "RadioButtonChecked", panel );


	CEmbeddedItemModelPanel *m_pModelInspectPanel;
	CItemModelPanel *m_pItemNamePanel;

	CEconItemView *m_pItemViewData;
	CEconItem *m_pSOEconItemData;
};

#endif // TF_ITEM_INSPECTION_PANEL_H
