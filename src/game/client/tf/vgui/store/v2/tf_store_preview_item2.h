//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_STORE_PREVIEW_ITEM2_H
#define TF_STORE_PREVIEW_ITEM2_H
#ifdef _WIN32
#pragma once
#endif

#include "store/tf_store_preview_item_base.h"

namespace vgui
{
	class ScrollBar;
};
class CNavigationPanel;
class CExLabel;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CFullscreenStorePreviewItem : public EditablePanel
{
	DECLARE_CLASS_SIMPLE( CFullscreenStorePreviewItem, EditablePanel );
public:
	CFullscreenStorePreviewItem( vgui::Panel *pParent, EditablePanel *pOwner );

	void SetItemDef( itemid_t iItemDef );

	void GoFullscreen( CTFPlayerModelPanel *pPlayerModelPanel );
	void ExitFullscreen();
	bool IsFullscreenMode();

private:
	MESSAGE_FUNC_PARAMS( OnNavButtonSelected, "NavButtonSelected", pData );

	virtual void OnThink();
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void OnCommand( const char *command );

	itemid_t		m_iItemDef;

	CExLabel		*m_pCycleTextLabel;
	CNavigationPanel *m_pTeamNavPanel;
	CExButton		*m_pPreviewButton;

	struct ModelState_t
	{
		int				m_aPlayerModelPanelBounds[4];
		Vector			m_vecPlayerPos;
		bool			m_bZoomed;
	}
	m_OldModelState;

	struct Stats_t
	{
		Stats_t() { Clear(); }
		void Clear() { V_memset( this, 0, sizeof( Stats_t ) ); }
		
		float			m_flRotationTime;
	}
	m_Stats;

	float			m_flGoFullscreenStartTime;
	bool			m_bIsHalloweenOrFullmoonOnlyItem;
	vgui::DHANDLE< CTFPlayerModelPanel > m_pPlayerModelPanel;

	CExButton		*m_pZoomButton;
	CExButton		*m_pRotLeftButton;
	CExButton		*m_pRotRightButton;

	EditablePanel	*m_pOverlayPanel;

	PHandle			m_hOwner;

	int				m_nLastMouseX;
	int				m_nLastMouseY;
	float			m_flLastMouseMoveTime;

	CPanelAnimationVar( float, m_flFullscreenFadeToBlackDuration, "fullscreen_fade_to_black_duration", "1.0" );
	CPanelAnimationVar( float, m_flModelPanelOriginX, "fullscreen_modelpanel_origin_x", "170" );
	CPanelAnimationVar( float, m_flModelPanelOriginY, "fullscreen_modelpanel_origin_y", "0" );
	CPanelAnimationVar( float, m_flModelPanelOriginZ, "fullscreen_modelpanel_origin_z", "-36" );
	CPanelAnimationVar( float, m_flUiFadeoutTime, "ui_fadeout_time", "5.0" );
	CPanelAnimationVar( float, m_flUiFadeoutDuration, "ui_fadeout_duration", "1.0" );
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CTFStorePreviewItemPanel2 : public CTFStorePreviewItemPanelBase
{
	DECLARE_CLASS_SIMPLE( CTFStorePreviewItemPanel2, CTFStorePreviewItemPanelBase );
public:
	CTFStorePreviewItemPanel2( vgui::Panel *pParent, const char *pResFile, const char *pPanelName, CStorePage *pOwner );

	virtual void	PreviewItem( int iClass, CEconItemView *pItem, const econ_store_entry_t* pEntry=NULL ) OVERRIDE;
	void			PreviewItemCopy( int iClass, CEconItemView *pItem, const econ_store_entry_t* pEntry=NULL );
	virtual void	SetState( preview_state_t iState );
	
	MESSAGE_FUNC_PARAMS( OnClassIconSelected, "ClassIconSelected", data );
	MESSAGE_FUNC( OnHideClassIconMouseover, "HideClassIconMouseover" );
	MESSAGE_FUNC_PARAMS( OnShowClassIconMouseover, "ShowClassIconMouseover", data );

protected:
	virtual void	OnThink();
	virtual void	ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void	OnCommand( const char *command );
	virtual void	PerformLayout( void );
	virtual void	OnTick( void );
	virtual void	OnMouseWheeled( int delta );

	int				PlaceControl( Panel *pParent, const char *pControlNameA, const char *pControlNameB, int nOffset, bool bVertical,
								  bool bSizeAToContents = true, bool bUseContentSize = true );
	void			DoClose();
	void			Clear();
	void			UpdateScrollableChild();

	virtual void	SetPlayerModelVisible( bool bVisible );
	virtual void	UpdateIcons( void );
	virtual void	UpdatePlayerModelButtons( void );
	virtual void	SetCycleLabelText( vgui::Label *pTargetLabel, const char *pCycleText );

	MESSAGE_FUNC_PARAMS( OnNavButtonSelected, "NavButtonSelected", pData );
	MESSAGE_FUNC_PARAMS( OnExitFullscreen, "ExitFullscreen", pData );

	Label			*m_pLastNewLineControl;
	EditablePanel	*m_pDialogFrame;	/// The background border
	EditablePanel	*m_pPreviewViewportBg;
	CExLabel		*m_pItemNameLabel;
	CExLabel		*m_pAttributesLabel;
	vgui::EditablePanel *m_pItemCollectionHighlight;
	CExLabel		*m_pCycleTextLabel;
	int				m_nNumAttribLinesAdded;
	bool			m_bArmoryTextAdded;
	EditablePanel	*m_pDetailsView;
	EditablePanel	*m_pDetailsViewChild;
	CExButton		*m_pAddRentalToCartButtons[3];
	EditablePanel	*m_pScrollableChild;
	ScrollBar		*m_pScrollBar;
	int				m_iSliderPos;
	bool			m_bCloseOnUp;
	bool			m_bMouseWasDown;
	int				m_aClickPos[2];
	CExButton		*m_pItemWikiPageButton;
	CNavigationPanel	*m_pTeamNavPanel;
	CExButton		*m_pPreviewButton;
	CExImageButton	*m_pGoFullscreenButton;
	int				m_nViewMaxHeight;

	CFullscreenStorePreviewItem	*m_pFullscreenPanel;
	bool			m_bIsHalloweenOrFullmoonOnlyItem;

	CEconItemView *m_pItemViewData;
	CEconItem *m_pSOEconItemData;

	// mouse over reference item tooltip
	CItemModelPanel				*m_pMouseOverItemPanel;
	CItemModelPanelToolTip		*m_pMouseOverTooltip;
	CUtlVector< CItemModelPanel* > m_vecReferenceItemPanels;

	CPanelAnimationVarAliasType( int, m_iSmallVerticalBreakSize, "small_vertical_break_size", "0", "proportional_ypos" );
	CPanelAnimationVarAliasType( int, m_iMediumVerticalBreakSize, "medium_vertical_break_size", "0", "proportional_ypos" );
	CPanelAnimationVarAliasType( int, m_iBigVerticalBreakSize, "big_vertical_break_size", "0", "proportional_ypos" );
	CPanelAnimationVarAliasType( int, m_iHorizontalBreakSize, "horizontal_break_size", "0", "proportional_xpos" );
	CPanelAnimationVarAliasType( int, m_iControlButtonWidth, "control_button_width", "0", "proportional_xpos" );
	CPanelAnimationVarAliasType( int, m_iControlButtonHeight, "control_button_height", "0", "proportional_ypos" );
	CPanelAnimationVarAliasType( int, m_iControlButtonY, "control_button_y", "0", "proportional_ypos" );

	MESSAGE_FUNC_INT( OnSliderMoved, "ScrollBarSliderMoved", position );
};

#endif // TF_STORE_PREVIEW_ITEM2_H
