//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_ITEM_CARD_PANEL_H
#define TF_ITEM_CARD_PANEL_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/Panel.h>
#include <vgui_controls/EditablePanel.h>
#include "item_model_panel.h"
#include "tf_controls.h"

class CEconItemView;
class CEmbeddedItemModelPanel;
namespace vgui
{
	class ScrollBar;
	class ImagePanel;
}
class CIconPanel;

using namespace vgui;


//-----------------------------------------------------------------------------
// Purpose: A simple container that contains repeating elements with common
//			and individual characteristics
//-----------------------------------------------------------------------------
class CRepeatingContainer : public EditablePanel
{
	DECLARE_CLASS_SIMPLE( CRepeatingContainer, EditablePanel );
public:
	CRepeatingContainer( Panel *pParent, const char *pszName );
	virtual ~CRepeatingContainer();

	virtual void ApplySettings( KeyValues *inResourceData ) OVERRIDE;
	virtual void PerformLayout() OVERRIDE;

	Panel* GetRepeatingChild( int nIndex ) const { return m_vecChildren[ nIndex ]; }
private:

	enum ELayoutMethod_t
	{
		METHOD_EVEN,
		METHOD_STEP,
	};

	CUtlVector< Panel* > m_vecChildren;
	ELayoutMethod_t m_eLayoutMethod;
	CPanelAnimationVarAliasType( int, m_iXStep, "x_step", "0", "proportional_xpos" );
};

//-----------------------------------------------------------------------------
// Purpose: A representation of an econ item as a collectible card
//-----------------------------------------------------------------------------
class CTFItemCardPanel : public EditablePanel
{
	DECLARE_CLASS_SIMPLE( CTFItemCardPanel, EditablePanel );
public:
	CTFItemCardPanel( Panel *parent, const char *name );
	virtual ~CTFItemCardPanel( void );

	virtual void ApplySchemeSettings( IScheme *pScheme ) OVERRIDE;
	virtual void ApplySettings( KeyValues *inResourceData ) OVERRIDE;
	virtual void PerformLayout( void ) OVERRIDE;
	virtual void SetVisible( bool bVisible ) OVERRIDE;
	virtual void SetMouseInputEnabled( bool state ) OVERRIDE;

	void SetItem( CEconItemView* pItem );
	CEconItemView* GetItem() { return m_pItem; }

	void PinCard( bool bPin );
	bool IsPinned() const { return m_bPinned; }
private:

	void UpdateDescription();
	void UpdateModelOrIcon();
	void LoadResFileForCurrentItem();

	template < class T >
	T* FindAndVerifyControl( Panel* pParent, const char* pszPanelName );

	
	CEconItemView*		m_pItem;

	ImagePanel *m_pDropShadow;
	CExImageButton *m_pCloseButton;

	ImagePanel *m_pBackground;
	ImagePanel *m_pGrime;
	ImagePanel *m_pRarityBackgroundOverlay;
	EditablePanel *m_pMainContainer;
	
	EditablePanel *m_pCardTop;
	CEmbeddedItemModelPanel *m_pItemModel;

	EditablePanel *m_pRarityContainer;
	Label *m_pItemName;
	Label *m_pRarityName;

	EditablePanel *m_pInfoContainer;
	Label *m_pClassLabel;
	CRepeatingContainer *m_pClassIconContainer;
	Label *m_pTypeLabel;
	Label *m_pTypeLabelValue;
	Label *m_pExteriorLabel;
	Label *m_pExteriorLabelValue;

	EditablePanel *m_pBottomContainer;
	CExScrollingEditablePanel *m_pBottomScrollingContainer;
	Label *m_pAttribsLabel;
	Label *m_pEquipSlotLabel;

	bool m_bAllControlsValid;
	bool m_bPinned;

	CPanelAnimationVarAliasType( int, m_iShadowOffset, "shadowoffset", "5", "proportional_int" );
};

//-----------------------------------------------------------------------------
// Purpose: Item model panel tooltip. Calls setvisible on the controlled panel
//			and positions it below/above the current panel.
//-----------------------------------------------------------------------------
class CItemCardPanelToolTip : public vgui::BaseTooltip 
{
	DECLARE_CLASS_SIMPLE( CItemCardPanelToolTip, vgui::BaseTooltip );
public:
	CItemCardPanelToolTip(vgui::Panel *parent, const char *text = NULL);

	void SetText(const char *text) { return; }
	const char *GetText() { return NULL; }

	virtual void PerformLayout();
	virtual void ShowTooltip( vgui::Panel *currentPanel );
	virtual void HideTooltip();

	void SetupPanels( vgui::Panel *pParentPanel, CTFItemCardPanel *pMouseOverItemPanel ) { m_pParentPanel = pParentPanel; m_pMouseOverItemPanel = pMouseOverItemPanel; }
	void SetPositioningStrategy( itempanel_tooltip_strategies_t iStrat ) { m_iPositioningStrategy = iStrat; }

private:
	void GetPosition( itempanel_tooltippos_t iTooltipPosition, CItemModelPanel *pItemPanel, int iItemX, int iItemY, int *iXPos, int *iYPos );
	bool ValidatePosition( CItemModelPanel *pItemPanel, int iItemX, int iItemY, int *iXPos, int *iYPos );

private:
	CTFItemCardPanel				*m_pMouseOverItemPanel;	// This is the tooltip panel we make visible. Must be a CItemModelPanel.
	vgui::Panel						*m_pParentPanel;		// This is the panel that we send item entered/exited messages to
	vgui::DHANDLE<CItemModelPanel> m_hCurrentPanel;

	itempanel_tooltip_strategies_t	m_iPositioningStrategy;
	bool							m_bHorizontalPreferLeft;
};

#endif // TF_ITEM_CARD_PANEL_H
